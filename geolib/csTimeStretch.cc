

#include <cmath>
#include <cstring>
#include "csTimeStretch.h"
#include "csInterpolation.h"
#include "geolib_methods.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>

using namespace cseis_geolib;

csTimeStretch::csTimeStretch( double sampleInt_ms, int numSamples ) {
  init( sampleInt_ms, numSamples, csTimeStretch::SAMPLE_INTERPOL_SINC );
}
csTimeStretch::csTimeStretch( double sampleInt_ms, int numSamples, int methodSampleInterpolation ) {
  init( sampleInt_ms, numSamples, methodSampleInterpolation );
}

void csTimeStretch::init( double sampleInt_ms, int numSamples, int methodSampleInterpolation ) {
  myIsBottomStretch = false;
  myLayerInterpolationMethod = csTimeStretch::LAYER_INTERPOL_LIN;
  mySampleInt           = sampleInt_ms;
  myNumSamples          = numSamples;
  myTimeOfLastSample    = (myNumSamples-1)*mySampleInt;
  mySampleInterpolationMethod = methodSampleInterpolation;
  if( mySampleInterpolationMethod == csTimeStretch::SAMPLE_INTERPOL_SINC ) {
    mySampleInterpolation = new csInterpolation( numSamples, (float)sampleInt_ms );
  }
  else {
    mySampleInterpolation = NULL;
  }
}

csTimeStretch::~csTimeStretch() {
  if( mySampleInterpolation != NULL ) {
    delete mySampleInterpolation;
    mySampleInterpolation = NULL;
  }
}

void csTimeStretch::setLayerInterpolationMethod( int method ) {
  myLayerInterpolationMethod = method;
}
//--------------------------------------------------------------------
//
//
void csTimeStretch::applyStretchFunction( float const* samplesIn, float const* tIn_ms, float const* stretch_ms, int numTimes, float* samplesOut )
{
  int numLayers = numTimes - 1;
  float* tOut_ms = new float[numTimes];
  tOut_ms[0] = tIn_ms[0];
  double stretchSumDbl = 0.0;
  for( int ilay = 0; ilay < numLayers; ilay++ ) {
    stretchSumDbl += (double)stretch_ms[ilay];
    tOut_ms[ilay+1] = tIn_ms[ilay+1] + (float)stretchSumDbl;
  }
  //  for( int ilay = 0; ilay < numLayers+1; ilay++ ) {
  //   fprintf(stderr,"STRETCH   %.4f %.4f\n", tIn_ms[ilay], tOut_ms[ilay]);
  //  }
  applyTimeInterval( samplesIn, tIn_ms, tOut_ms, numTimes, samplesOut );
  delete [] tOut_ms;
}

void csTimeStretch::applyTimeInterval( float const* samplesIn, float const* tIn, float const* tOut, int numTimes, float* samplesOut )
{
  int numLayers = numTimes - 1;
  int ilay = 0;
  // Traces always start at time = 0.0. First layer to stretch may start further down. If that is the case, don't stretch the top, just copy.
  float tTopIn  = 0.0;
  float tTopOut = 0.0;
  float tBotIn  = tIn[ilay];
  float tBotOut = tOut[ilay];
  float dtIn  = tBotIn  -  tTopIn;  // Input layer thickness in [ms]
  float dtOut = tBotOut - tTopOut; // Output layer thickness in [ms]
  float timeLast = (float)((myNumSamples-1) * mySampleInt);

  //  fprintf(stdout,"TIMEIO  START %f %f   %f %f  %d\n", tBotIn, tBotOut,dtIn,dtOut,ilay);
  for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
    float timeOut = (float)(isamp * mySampleInt);
    float timeIn  = timeOut;
    while( ilay <= numLayers && timeOut > tBotOut ) {
      ilay += 1;
      tTopIn = tBotIn;
      tTopOut = tBotOut;
      if( ilay <= numLayers ) {
        tBotIn  = tIn[ilay];
        tBotOut = tOut[ilay];
        dtIn  = tBotIn  -  tTopIn;  // Input layer thickness in [ms]
        dtOut = tBotOut - tTopOut; // Output layer thickness in [ms]
      }
      else {
        // For bottom of data beyond bottom of last layer: Make stretch ratio = dtIN/dtOut = 1.0 = no stretching, just static shift
        if( !myIsBottomStretch ) {
          dtOut = dtIn;
        }
        // ..otherwise, keep stretch factor from last specified layer
      }
    }
    if( dtOut != 0.0 ) {
      timeIn = tTopIn + (dtIn/dtOut) * ( timeOut - tTopOut );
      if( timeIn < 0.0 ) timeIn = 0.0;
      else if( timeIn > timeLast ) timeIn = timeLast;
    }
    //    fprintf(stdout,"TIMEIO  %f %f   %f %f  %d   %f %f\n", timeIn, timeOut,dtIn,dtOut,ilay,tBotIn,tBotOut);
    if( mySampleInterpolationMethod == csTimeStretch::SAMPLE_INTERPOL_SINC ) {
      samplesOut[isamp] = mySampleInterpolation->valueAt( timeIn, samplesIn );
    }
    else if( mySampleInterpolationMethod == csTimeStretch::SAMPLE_INTERPOL_QUAD ) {
      samplesOut[isamp] = getQuadAmplitudeAtSample( samplesIn, timeIn/mySampleInt, myNumSamples );
    }
    else {
      samplesOut[isamp] = getLinAmplitudeAtSample( samplesIn, timeIn/mySampleInt, myNumSamples );
    }
  }
}


/*

// Old methods. After proper testing these methods did not seem to work as expected. Remove in future releases

void csTimeStretch::process( float const* samplesIn, float const* times_ms, float const* stretch_ms, int numTimes, float* samplesOut )
{
  int numLayers = numTimes - 1;
  int ilay = 0;
  float t1 = times_ms[ilay];
  float dt = times_ms[ilay+1] - t1;
  float stretch = stretch_ms[ilay];
  float timeNew;
  float timeOld;

  // (1) Top data: Just copy from input trace to output trace
  int isamp = (int)( t1 / mySampleInt );
  if( isamp > 0 ) {
    memcpy( samplesOut, samplesIn, isamp*sizeof(float) );
  }
  
  // (2) Layers that shall be squeezed/stretched
  while( isamp < myNumSamples ) {
    timeNew = isamp * mySampleInt;
    timeOld = (dt*timeNew + stretch*t1)/(dt + stretch);
    if( timeOld > times_ms[ilay+1] ) {
      ilay += 1;
      if( ilay == numLayers ) {
        break;
      }
      t1 = times_ms[ilay];
      dt = times_ms[ilay+1] - t1;
      stretch = stretch_ms[ilay];
    }
    else {
      if( mySampleInterpolationMethod == csTimeStretch::SAMPLE_INTERPOL_SINC ) {
        samplesOut[isamp] = mySampleInterpolation->valueAt( timeOld, samplesIn );
      }
      else if( mySampleInterpolationMethod == csTimeStretch::SAMPLE_INTERPOL_QUAD ) {
        samplesOut[isamp] = getQuadAmplitudeAtSample( samplesIn, timeOld/mySampleInt, myNumSamples );
      }
      else {
        samplesOut[isamp] = getLinAmplitudeAtSample( samplesIn, timeOld/mySampleInt, myNumSamples );
      }
      isamp += 1;
    }
  }

  // (3) Bottom data: Move data (interpolate if necessary)
  while( isamp < myNumSamples ) {
    timeNew = isamp * mySampleInt;
   // printf(" ***Time new, old: %d  %f %f    %f\n", isamp, timeNew, timeOld, stretch );
    if( mySampleInterpolationMethod == csTimeStretch::SAMPLE_INTERPOL_SINC ) {
      samplesOut[isamp] = mySampleInterpolation->valueAt( timeOld, samplesIn );
    }
    else {
      samplesOut[isamp] = getQuadAmplitudeAtSample( samplesIn, timeOld/mySampleInt, myNumSamples );
    }
    isamp   += 1;
    timeOld += mySampleInt;
  }
}

void csTimeStretch::process2( float const* samplesIn, float const* tIn, float const* tOut, int numTimes, float* samplesOut )
{
  int numLayers = numTimes - 1;
  int ilay = 0;
  float tTopIn  = tIn[ilay];
  float tBotIn  = tIn[ilay+1];
  float tTopOut = tOut[ilay];
  float tBotOut = tOut[ilay+1];
  float dtIn  = tBotIn  -  tTopIn;  // Input layer thickness in [ms]
  float dtOut = tBotOut - tTopOut; // Output layer thickness in [ms]
  float timeLast = (myNumSamples-1) * mySampleInt;

  for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
    float timeOut = isamp * mySampleInt;
    while( ilay < numLayers && timeOut > tBotOut ) {
      ilay += 1;
      tTopIn = tBotIn;
      tTopOut = tBotOut;
      if( ilay < numLayers ) {
        tBotIn  = tIn[ilay+1];
        tBotOut = tOut[ilay+1];
      }
      else {
        tBotIn  = timeLast; //myNumSamples * mySampleInt;
        tBotOut = timeLast; //myNumSamples * mySampleInt;
      }
      dtIn  = tBotIn  -  tTopIn;  // Input layer thickness in [ms]
      dtOut = tBotOut - tTopOut; // Output layer thickness in [ms]
    }
    float timeIn = timeOut;
    if( dtOut != 0.0 ) {
      timeIn = tTopIn + (dtIn/dtOut) * ( timeOut - tTopOut );
      if( timeIn < 0.0 ) timeIn = 0.0;
      else if( timeIn > timeLast ) timeIn = timeLast;
    }
    if( mySampleInterpolationMethod == csTimeStretch::SAMPLE_INTERPOL_SINC ) {
      samplesOut[isamp] = mySampleInterpolation->valueAt( timeIn, samplesIn );
    }
    else if( mySampleInterpolationMethod == csTimeStretch::SAMPLE_INTERPOL_QUAD ) {
      samplesOut[isamp] = getQuadAmplitudeAtSample( samplesIn, timeIn/mySampleInt, myNumSamples );
    }
    else {
      samplesOut[isamp] = getLinAmplitudeAtSample( samplesIn, timeIn/mySampleInt, myNumSamples );
    }
  }
}
*/


