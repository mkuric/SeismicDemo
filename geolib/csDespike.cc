

#include "csDespike.h"
#include "csVector.h"
#include "geolib_math.h"

using namespace cseis_geolib;

csDespike::csDespike( int numSamples, double sampleInt, DespikeConfig& config ) {
  init( numSamples, sampleInt );
  set( config );
}
void csDespike::init( int numSamples, double sampleInt ) {
  mySampleInt     = sampleInt;
  myNumSamples    = numSamples;
  myRatios = NULL;
}
csDespike::~csDespike() {
  if( myRatios != NULL ) {
    delete [] myRatios;
    myRatios = NULL;
  }
}
//***************************************************************************************
//
//
void csDespike::set( DespikeConfig const& config ) {
  if( myRatios != NULL ) {
    delete [] myRatios;
    myRatios = NULL;
  }
  myPerformDebias = config.performDebias;
  myMaxRatio = config.maxRatio;
  myMethod   = config.method;

  if( config.incWin <= mySampleInt ) {
    myIncWin = 1;
  }
  else {
    throw( csException("Increments other than one sample interval are currently not supported.") );
    myIncWin = (int)( config.incWin / mySampleInt );
  }

  myWidthRefWin    = 2*(int)((int)( config.widthRefWin / mySampleInt + 0.5 )/2) + 1;  // Make sure this becomes an odd number
  myWidthSpikeWin  = (int)( config.minWidthSpikeWin / mySampleInt + 0.5 );
  myStartSample    = (int)( config.start / mySampleInt + 0.5 );
  if( config.stop > 0 ) {
    myStopSample     = (int)( config.stop / mySampleInt + 0.5 );
  }
  else {
    myStopSample     = myNumSamples - 1;
  }
  myIncWin         = 1;
  myWidthMeanWin   = config.advanced.widthMeanWin_inSamples;
  myWindowAmplifier= config.advanced.windowAmplifier;
  myRatioAmplifier = config.advanced.ratioAmplifier;

  if( myStartSample < 0 ) myStartSample = 0;
  if( myStopSample >= myNumSamples ) myStopSample = myNumSamples-1;
  int numSamplesToProcess = myStopSample - myStartSample + 1;
  myNumWindows = ( numSamplesToProcess - myWidthRefWin ) / myIncWin + 1;

  myRatios = new float[myNumWindows];
}

//***************************************************************************************
//
//
void csDespike::apply( float* samples, int numSamples, int& numSpikesFound, int& numSamplesAffected ) {

  // Compute amplitude ratios between defined spike window and background
  if( !computeRatios( samples, numSamples ) ) {
    return;
  }

  numSpikesFound      = 0;
  numSamplesAffected  = 0;
  int countSampleLast = 0;

  if( myMethod == COSINE_TAPER ) {
    for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
      samples[isamp] = 1.0;
    }
  }
  int widthRefWinHalf = myWidthRefWin / 2;
  int winIndex = 0;
  while( winIndex < myNumWindows ) {
    // If computed ratio exceeds the defined maximum, a spike has been detected
    if( myRatios[winIndex] > myMaxRatio ) {
      int win2 = winIndex + 1;
      double maxRatio = myRatios[winIndex];
      while( win2 < myNumWindows && myRatios[win2] > myMaxRatio ) {
        if( myRatios[win2] > maxRatio ) {
          maxRatio = myRatios[win2];
        }
        win2 += 1;
      }
      maxRatio *= myRatioAmplifier;
      int width = myWindowAmplifier*(win2 - winIndex);
      if( width < myWidthSpikeWin/2 ) width = myWidthSpikeWin/2;
      int winMid = ( win2 + winIndex ) / 2;
      int sampleMid   = winMid * myIncWin + myStartSample + widthRefWinHalf;
      int sampleFirst = std::max( 0, sampleMid - width );
      int sampleLast  = MIN( sampleMid + width, myNumSamples-1 );
      if( myMethod == COSINE_TAPER ) {
        for( int isamp = sampleFirst; isamp <= sampleLast; isamp++ ) {
          double phase = ( ( (double)(isamp-sampleFirst) / (double)width ) - 1.0 ) * M_PI;
          double scalar = 0.5 * ( cos(phase) * (-1.0 + 1.0/maxRatio) + (1.0 + 1.0/maxRatio) );
          samples[isamp] *= (float)scalar;
        }
      }
      else if( myMethod == LINEAR_INTERPOLATION ) {
        float val1 = samples[sampleFirst];
        float val2 = samples[sampleLast];
        float length = sampleLast - sampleFirst;
        for( int isamp = sampleFirst+1; isamp < sampleLast; isamp++ ) {
          float valCurrent = val1 + (float)(isamp-sampleFirst)/length * ( val2 - val1 );
          samples[isamp] = valCurrent;
        }
      }
      else {
        for( int isamp = sampleFirst; isamp <= sampleLast; isamp++ ) {
          samples[isamp] = 0;
        }
      }
      // Count identified spikes, and number of affected samples
      numSpikesFound     += 1;
      numSamplesAffected += sampleLast - std::max( sampleFirst, countSampleLast ) + 1;
      countSampleLast    = sampleLast;

      winIndex = win2;
    }
    winIndex += 1;
  }

}
//***************************************************************************************
//
//
bool csDespike::computeRatios( float* samples, int numSamples ) {
  if( myWidthRefWin >= myNumSamples ) return false;
  if( numSamples != myNumSamples ) return false;

  csVector<DespikePoint*> sortedList(myWidthRefWin);
  int widthRefWinHalf  = myWidthRefWin / 2;
  int widthMeanWinHalf = myWidthMeanWin / 2;

//---------------------------------------------------------------------
// Set first window. Omit last sample so that the following loop also works for first window
//
  double currentSumRefWin = 0.0;
  for( int i = 0; i < myWidthRefWin-1; i++ ) {
    int sampleIndex = i + myStartSample;
    float value = fabs(samples[sampleIndex]);
    DespikePoint* p = new DespikePoint( sampleIndex, value );
    int is = 0;
    for( ; is < sortedList.size(); is++ ){
      DespikePoint* ptmp = sortedList.at(is);
      if( p->value < ptmp->value ) {
        sortedList.insert( p, is );
        break;
      }
    }
    if( is == sortedList.size() ) {
      sortedList.insert( p, sortedList.size() );
    }
    if( myPerformDebias ) {
      currentSumRefWin += samples[sampleIndex];
    }
  }

  double currentSumMeanWin = 0.0;
  for( int i = 0; i < myWidthMeanWin-1; i++ ){
    int sampleIndex = i + widthRefWinHalf + myStartSample - widthMeanWinHalf;
    //fprintf(stderr,"Sum add %d %f\n", sampleIndex, samples[sampleIndex] );
    currentSumMeanWin += fabs( samples[sampleIndex] );
  }

//----------------------------------------------------------------------
// Main loop
//
  for( int iwin = 0; iwin < myNumWindows; iwin++ ) {
// (1) Determine median value in reference window (sort all values, pick middle one)
    int sampleFirst = iwin * myIncWin + myStartSample;
    int sampleLast  = sampleFirst + myWidthRefWin - 1;
    int sampleMid   = widthRefWinHalf + sampleFirst;
    float value     = fabs(samples[sampleLast]);

    int sampleFirstMean = sampleMid - widthMeanWinHalf;
    int sampleLastMean  = sampleMid + widthMeanWinHalf;
// Remove previous point
    DespikePoint* p = NULL;
    if( iwin > 0 ) {
      int sample_del = sampleFirst - 1;
      int length = sortedList.size();
      for( int is = 0; is < length; is++ ) {   // Currently assumes that window increment is 1
        p = sortedList.at(is);
        if( p->index == sample_del ) {
          sortedList.remove(is);
          break;
        }
      }
      //fprintf(stderr,"-Sum min %d %f\n", sampleFirstMean-1, samples[sampleFirstMean-1] );
      currentSumMeanWin -= fabs((double)samples[sampleFirstMean-1]);
    }
    else {
      p = new DespikePoint( 0, 0 );
    }
    //fprintf(stderr,"+Sum min %d %f\n", sampleLastMean, samples[sampleLastMean] );
    if( myPerformDebias ) {
      if( iwin > 0 ) currentSumRefWin -= (double)samples[sampleFirst - 1];
      currentSumRefWin += (double)samples[sampleLast];
      value -= (float)(currentSumRefWin / (double)myWidthRefWin);
    }
    p->index = sampleLast;
    p->value = value;
    insertNewValue( p, &sortedList );

    double medianValue = sortedList.at( widthRefWinHalf )->value;
    currentSumMeanWin += fabs((double)samples[sampleLastMean]);
    myRatios[iwin] = (float)( currentSumMeanWin / (medianValue * myWidthMeanWin ) );  // Compute ratio from mean value
  }
  // Free memory
  for( int is = 0; is < sortedList.size(); is++ ) {
    delete sortedList.at( is );
  }
  return true;
}
//***************************************************************************************
//
//
void csDespike::insertNewValue( DespikePoint* p, csVector<DespikePoint*>* sortedList ) {
  int id1 = 0;
  int id2 = sortedList->size() - 1;
  float value = p->value;

  while( true ) {
    int id0 = (id1+id2)/2;
    float val0 = sortedList->at(id0)->value;
    if( value > val0 ) {
      if( id2-id1 <= 1 ) {
        if( value > sortedList->at(id2)->value ) {
          sortedList->insert( p, id2+1 );
        }
        else {
          sortedList->insert( p, id1+1 );
        }
        break;
      }
      id1 = id0;
    }
    else if( value < val0 ) {
      if( id2-id1 <= 1 ) {
        sortedList->insert( p, id1 );
        break;
      }
      id2 = id0;
    }
    else {
      sortedList->insert( p, id0 );
      break;
    }
  }
}
void csDespike::getDefaultFrequencySpikeConfig( DespikeConfig& config ) {
  config.widthRefWin = 1.0;
  config.incWin      = 0;
  config.start       = 0;
  config.stop        = 0;
  config.minWidthSpikeWin = 0.1;
  config.advanced.widthMeanWin_inSamples = 1;
  config.advanced.ratioAmplifier  = 1;
  config.advanced.windowAmplifier = 1;
  config.performDebias = true;
  config.maxRatio      = 3.0;
  config.method        = csDespike::LINEAR_INTERPOLATION;
}
void csDespike::getDefaultTimeNoiseBurstConfig( DespikeConfig& config ) {
  config.widthRefWin = 3000;
  config.incWin      = 0;
  config.start       = 0;
  config.stop        = 0;
  config.minWidthSpikeWin = 300;
  config.advanced.widthMeanWin_inSamples = 11;
  config.advanced.ratioAmplifier  = 2;
  config.advanced.windowAmplifier = 4;
  config.performDebias = false;
  config.maxRatio   = 5.0;
  config.method     = csDespike::COSINE_TAPER;
}


