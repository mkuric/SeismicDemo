

#include "csFFTTools.h"
#include "csException.h"
#include "geolib_math.h"
#include "geolib_defines.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>

using namespace cseis_geolib;

csFFTTools::csFFTTools( int numSamples ) {
  myNumSamplesIn  = numSamples;
  myNumSamplesOut = numSamples;
  myOrder = 0;
  myCutOffFreqHz = 0;
  init();
}

csFFTTools::csFFTTools( int numSamples, float sampleInt ) {
  myNumSamplesIn  = numSamples;
  mySampleIntIn     = sampleInt;
  myNumSamplesOut = myNumSamplesIn;
  mySampleIntOut    = mySampleIntIn;

  myOrder = 2;
  float freqNyquist = (float)(500.0/mySampleIntIn);
  myCutOffFreqHz = freqNyquist;

  init();
}

csFFTTools::csFFTTools( int theNumSamplesIn, int theNumSamplesOut, float theSampleIntIn, float theSampleIntOut ) {
  myNumSamplesIn  = theNumSamplesIn;
  mySampleIntIn     = theSampleIntIn;
  myNumSamplesOut = theNumSamplesOut;
  mySampleIntOut    = theSampleIntOut;

  myOrder  = 10;
  float freqNyquist = (float)(500.0/mySampleIntIn);
  double ratio = (double)mySampleIntOut / (double)mySampleIntIn;
  myCutOffFreqHz = (float)( freqNyquist / ratio ) * 0.8f;

  init();
}
//--------------------------------------------------------------------------------
//
//
void csFFTTools::init() {
  myNumFFTSamplesIn  = myNumSamplesIn;
  myNumFFTSamplesOut = myNumSamplesOut;

  myFilterWavelet = NULL;
  myLengthFilterWavelet = 0;
  myIsFilterWavelet     = false;

  int two_power_m;
  csFFTTools::Powerof2( myNumFFTSamplesIn, &myTwoPowerIn, &two_power_m );
  if( two_power_m != myNumFFTSamplesIn ) {
    myNumFFTSamplesIn = two_power_m * 2;
    myTwoPowerIn += 1;
  }

  myNumFFTSamplesOut = myNumSamplesOut;
  csFFTTools::Powerof2( myNumFFTSamplesOut, &myTwoPowerOut, &two_power_m );
  if( two_power_m != myNumFFTSamplesOut ) {
    myNumFFTSamplesOut = two_power_m * 2;
    myTwoPowerOut += 1;
  }
  myBufferReal = new double[myNumFFTSamplesIn];
  myBufferImag = new double[myNumFFTSamplesIn];

  myOutputImpulseResponse = false;
}

//--------------------------------------------------------------------------------
//
//
csFFTTools::~csFFTTools() {
  if( myBufferReal != NULL ) {
    delete [] myBufferReal;
    myBufferReal = NULL;
  }
  if( myBufferImag != NULL ) {
    delete [] myBufferImag;
    myBufferImag = NULL;
  } 
  if( myFilterWavelet != NULL ) {
    delete [] myFilterWavelet;
    myFilterWavelet = NULL;
  }
}
//--------------------------------------------------------------------------------
//
//
void csFFTTools::setFilter( int order, float cutOffFreqHz, bool outputImpulseResponse ) {
  myOrder = order;
  myCutOffFreqHz = cutOffFreqHz;
  myOutputImpulseResponse = outputImpulseResponse;
}
void csFFTTools::setFilterWavelet( int length ) {
  myIsFilterWavelet = true;
  if( myFilterWavelet != NULL ) {
    delete [] myFilterWavelet;
  }
  myLengthFilterWavelet = length;
  myFilterWavelet = new double[myLengthFilterWavelet];
}
//--------------------------------------------------------------------------------
bool csFFTTools::fft_forward( float const* samples, bool doNormalisation ) {
  setBuffer( samples );
  return csFFTTools::fft( csFFTTools::FORWARD, myTwoPowerIn, myBufferReal, myBufferImag, doNormalisation );
}
//--------------------------------------------------------------------------------
bool csFFTTools::fft_forward( float const* samples, float* ampSpec, bool doNormalisation ) {
  return fft_forward( samples, ampSpec, NULL, doNormalisation );
}
bool csFFTTools::fft_forward( float const* samples, float* ampSpec, float* phaseSpec, bool doNormalisation ) {
  setBuffer( samples );
  bool success = csFFTTools::fft( csFFTTools::FORWARD, myTwoPowerIn, myBufferReal, myBufferImag, doNormalisation );
  if( !success ) return false;
  
  convertToAmpPhase( ampSpec, phaseSpec );
  return true;
}
//--------------------------------------------------------------------------------
// Convert real/imaginary spectrum to to amp/phase
//
void csFFTTools::convertToAmpPhase( float* ampSpec, float* phaseSpec ) {
  for( int i = 0; i <= myNumFFTSamplesIn/2; i++ ) {
    ampSpec[i] = (float)( 2*sqrt(myBufferReal[i]*myBufferReal[i] + myBufferImag[i]*myBufferImag[i]) );
  }
  if( phaseSpec != NULL ) {
    for( int i = 0; i <= myNumFFTSamplesIn/2; i++ ) {
      phaseSpec[i] = (float)atan2( -myBufferImag[i], myBufferReal[i] );
    }
  }
}
//--------------------------------------------------------------------------------
// Convert amp/phase spectrum to to real/imaginary
//
void csFFTTools::convertFromAmpPhase( float const* ampSpec, float const* phaseSpec ) {
  for( int i = 0; i <= myNumFFTSamplesIn/2; i++ ) {
    myBufferReal[i] = 0.5*ampSpec[i] * cos(phaseSpec[i]);
    myBufferImag[i] = -0.5*ampSpec[i] * sin(phaseSpec[i]);
  }
  for( int i = 1; i < myNumFFTSamplesIn/2; i++ ) {
    myBufferReal[myNumFFTSamplesIn-i] = myBufferReal[i];
    myBufferImag[myNumFFTSamplesIn-i] = -myBufferImag[i];
  }

  // Phase at 0Hz and numFFTSamplesNyquist is always set to zero. If not, time series will have imaginary part.
  myBufferImag[0] = 0.0;
  myBufferImag[myNumFFTSamplesIn/2] = 0.0;
}

//--------------------------------------------------------------------------------
//
bool csFFTTools::fft_inverse( float const* samples, int fftDataType, bool doNormalisation ) {
  if( fftDataType == FX_REAL_IMAG ) {
    for( int i = 0; i < myNumFFTSamplesIn; i++ ) {
      myBufferReal[i] = samples[i];
      myBufferImag[i] = samples[i+myNumFFTSamplesIn];
    }
  }
  else if( fftDataType == FX_AMP_PHASE ) {
    // Input float array contains both amplitude and phase spectrum
    convertFromAmpPhase( &samples[0], &samples[myNumFFTSamplesIn/2+1] );
  }
  else {
    // ...just use currently stored real/imag buffers for inverse FFT
//    return false;
  }
  return csFFTTools::fft( csFFTTools::INVERSE, myTwoPowerIn, myBufferReal, myBufferImag, doNormalisation );
}

bool csFFTTools::fft_inverse( float const* ampSpec, float const* phaseSpec, bool doNormalisation ) {
  convertFromAmpPhase( ampSpec, phaseSpec );
  return csFFTTools::fft( csFFTTools::INVERSE, myTwoPowerIn, myBufferReal, myBufferImag, doNormalisation );
}
//--------------------------------------------------------------------------------
//
//
void csFFTTools::lowPass( float* samples, int order, float cutOffFreqHz, bool outputImpulseResponse ) {
  myCutOffFreqHz = cutOffFreqHz;
  myOrder = order;
  myOutputImpulseResponse = outputImpulseResponse;
  filter( samples, LOWPASS );
}
//--------------------------------------------------------------------------------
//
//
void csFFTTools::highPass( float* samples, int order, float cutOffFreqHz, bool outputImpulseResponse ) {
  myCutOffFreqHz = cutOffFreqHz;
  myOrder = order;
  myOutputImpulseResponse = outputImpulseResponse;
  filter( samples, HIGHPASS );
}
//--------------------------------------------------------------------------------
//
//
void csFFTTools::resample( float* samples ) {
  resample( samples, myOrder, myCutOffFreqHz );
}
void csFFTTools::resample( float* samples, int order, float cutOffFreqHz ) {
  myCutOffFreqHz = cutOffFreqHz;
  myOrder = order;
  myOutputImpulseResponse = false;

  //  filter( samples, RESAMPLE );

  setBuffer( samples );

  float rmsIn  = compute_rms( samples, myNumSamplesIn );

  if( !csFFTTools::fft( csFFTTools::FORWARD, myTwoPowerIn, myBufferReal, myBufferImag, false ) ) {
    delete [] myBufferReal;
    delete [] myBufferImag;
    throw( csException("csFFTTools::filter(): Unknown error occurred during forward FFT transform.") );
  }

  //  fprintf(stderr,"IN: FFT numsamplesIN: %d, myNumSamplesOut: %d, in/out: %d %d\n", myNumFFTSamplesIn, myNumFFTSamplesOut, myNumSamplesIn, myNumSamplesOut );
  double G0 = 1.0;
  double power = myOrder * 2.0;
  double df = 1.0 / ( (double)myNumFFTSamplesIn*mySampleIntIn/1000.0 );

  myBufferReal[myNumFFTSamplesOut/2] = 0.0; //myBufferReal[numFFTSamples/2];
  myBufferImag[myNumFFTSamplesOut/2] = 0.0; //myBufferImag[numFFTSamples/2];

  for( int is = 0; is < myNumFFTSamplesOut/2; is++ ) {
    double freq = (double)is * df;
    double dampG = G0 / (1.0 + pow(freq/myCutOffFreqHz,power) );
    //    printf("    Freq  %d  %f  %f\n", is, freq, dampG );
    myBufferReal[is] *= dampG;
    myBufferImag[is] *= dampG;
  }
  for( int is = 1; is < myNumFFTSamplesOut/2; is++ ) {
    int indexOut  = myNumFFTSamplesOut/2 + is;
    int indexIn = 3*myNumFFTSamplesIn/4 + is;
    double freq = (double)(indexIn-myNumFFTSamplesIn)*df;
    double dampG = G0 / (1.0 + pow(freq/myCutOffFreqHz,power) );
    //    printf("    Freq2  %d %d  %f  %f\n", indexIn, indexOut, freq, dampG );
    myBufferReal[indexOut] = myBufferReal[indexIn] * dampG;
    myBufferImag[indexOut] = myBufferImag[indexIn] * dampG;
  }
  
  if( !csFFTTools::fft( csFFTTools::INVERSE, myTwoPowerOut, myBufferReal, myBufferImag, true ) ) {
    delete [] myBufferReal;
    delete [] myBufferImag;
    throw( csException("filter: Unknown error occurred during inverse FFT transform.") );
  }
  for( int i = 0; i < myNumSamplesOut; i++ ) {
    samples[i] = (float)myBufferReal[i];
  }

  float rmsOut = compute_rms( samples, myNumSamplesOut );
  if( rmsOut != 0.0 ) {
    float ratio = rmsIn / rmsOut;
    //    fprintf(stderr,"RMS ratio: %f\n", ratio);
    for( int i = 0; i < myNumSamplesOut; i++ ) {
      samples[i] *= ratio;
    }
  }
  //  fprintf(stderr,"Num samples in/out: %d %d, order: %d %f\n", myNumSamplesIn, myNumSamplesOut, myOrder, myCutOffFreqHz );
}
//--------------------------------------------------------------------------------
//
//
void csFFTTools::filter( float* samples, int filterType ) {
  setBuffer( samples );

  if( !csFFTTools::fft( csFFTTools::FORWARD, myTwoPowerIn, myBufferReal, myBufferImag, false ) ) {
    delete [] myBufferReal;
    delete [] myBufferImag;
    throw( csException("csFFTTools::filter(): Unknown error occurred during forward FFT transform.") );
  }

  double G0 = 1.0;
  double power = myOrder * 2.0;
  double df = 1.0 / ( (double)myNumFFTSamplesIn*mySampleIntIn/1000.0 );

  if( filterType == LOWPASS ) {
    for( int is = 0; is <= myNumFFTSamplesIn/2; is++ ) {
      double freq = (double)is * df;
      double dampG = G0 / (1.0 + pow(freq/myCutOffFreqHz,power) );
      myBufferReal[is] *= dampG;
      myBufferImag[is] *= dampG;
    }
    for( int is = myNumFFTSamplesIn/2+1; is < myNumFFTSamplesIn; is++ ) {
      double freq = (double)(is-myNumFFTSamplesIn)*df;
      double dampG = G0 / (1.0 + pow(freq/myCutOffFreqHz,power) );
      myBufferReal[is] *= dampG;
      myBufferImag[is] *= dampG;
    }
  }
  else if( filterType == HIGHPASS ) {
    for( int is = 0; is <= myNumFFTSamplesIn/2; is++ ) {
      double freq = (double)is * df;
      double dampG;
      if( freq == 0 ) {
        dampG = 0;
      }
      else {
        dampG = G0 / (1.0 + pow(myCutOffFreqHz/freq,power) );
      }
      myBufferReal[is] *= dampG;
      myBufferImag[is] *= dampG;
    }
    for( int is = myNumFFTSamplesIn/2+1; is < myNumFFTSamplesIn; is++ ) {
      double freq = (double)(is-myNumFFTSamplesIn)*df;
      double dampG;
      if( freq == 0 ) {
        dampG = 0;
      }
      else {
        dampG = G0 / (1.0 + pow(myCutOffFreqHz/freq,power) );
      }
      myBufferReal[is] *= dampG;
      myBufferImag[is] *= dampG;
    }
  }
  else {
    //
  }

  if( !csFFTTools::fft( csFFTTools::INVERSE, myTwoPowerOut, myBufferReal, myBufferImag, true ) ) {
    delete [] myBufferReal;
    delete [] myBufferImag;
    myBufferReal = NULL;
    myBufferImag = NULL;
    throw( csException("filter: Unknown error occurred during inverse FFT transform.") );
  }
  for( int i = 0; i < myNumSamplesOut; i++ ) {
    samples[i] = (float)myBufferReal[i];
  }
}
//--------------------------------------------------------------------------------
// Set FFT coefficients
//
void csFFTTools::setBuffer( float const* samples ) {
  if( !myOutputImpulseResponse ) {
    for( int i = 0; i < myNumSamplesIn; i++ ) {
      myBufferReal[i] = samples[i];
      myBufferImag[i] = 0;
    }
    for( int i = myNumSamplesIn; i < myNumFFTSamplesIn; i++ ) {
      myBufferReal[i] = 0;
      myBufferImag[i] = 0;
    }
  }
  //----------------------------------------------------------
  // Create filter impulse response
  else {
    double amplitude = 1.0;
    for( int i = 0; i < myNumFFTSamplesIn; i++ ) {
      myBufferReal[i] = 0;
      myBufferImag[i] = 0;
    }
    myBufferReal[myNumFFTSamplesIn/2] = amplitude;
  }
}


bool csFFTTools::Powerof2( int numFFTSamplesX, int* m, int* twopm ) {
  int value = numFFTSamplesX;
  *m = 0;
  while( (value = (int)(value / 2)) > 0 ) {
    *m += 1;
  }
  *twopm = (int)pow( 2.0, *m );
  return( *twopm == numFFTSamplesX );
}


/*-------------------------------------------------------------------------
   This computes an in-place complex-to-complex FFT
   x and y are the real and imaginary arrays of 2^m points.
   dir =  1 gives forward transform
   dir = -1 gives reverse transform

     Formula: forward
                  N-1
                  ---
              1   \          - j k 2 pi n / N
      X(n) = ---   >   x(k) e                    = forward transform
              N   /                                n=0..N-1
                  ---
                  k=0

      Formula: reverse
                  N-1
                  ---
                  \          j k 2 pi n / N
      X(n) =       >   x(k) e                    = forward transform
                  /                                n=0..N-1
                  ---
                  k=0
 * This fft implementation is pulled somewhere from the Internet, I forgot where...
 */
//bool csFFTTools::fft( int dir, int power_of_two, double *realValues, double *imagValues ) {
//  return csFFTTools::fft( dir, power_of_two, realValues, imagValues, false );
//}
bool csFFTTools::fft( int dir, int power_of_two, double *realValues, double *imagValues, bool doNormalisation )
{
   long nn,i,i1,j,k,i2,l,l1,l2;
   double c1,c2,tx,ty,t1,t2,u1,u2,z;

   /* Calculate the number of points */
   nn = 1;
   for (i=0;i<power_of_two;i++)
      nn *= 2;

   /* Do the bit reversal */
   i2 = nn >> 1;
   j = 0;
   for (i=0;i<nn-1;i++) {
      if (i < j) {
         tx = realValues[i];
         ty = imagValues[i];
         realValues[i] = realValues[j];
         imagValues[i] = imagValues[j];
         realValues[j] = tx;
         imagValues[j] = ty;
      }
      k = i2;
      while (k <= j) {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   /* Compute the FFT */
   c1 = -1.0;
   c2 = 0.0;
   l2 = 1;
   for (l=0;l<power_of_two;l++) {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0;
      u2 = 0.0;
      for (j=0;j<l1;j++) {
         for (i=j;i<nn;i+=l2) {
            i1 = i + l1;
            t1 = u1 * realValues[i1] - u2 * imagValues[i1];
            t2 = u1 * imagValues[i1] + u2 * realValues[i1];
            realValues[i1] = realValues[i] - t1;
            imagValues[i1] = imagValues[i] - t2;
            realValues[i] += t1;
            imagValues[i] += t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) / 2.0);
      if (dir == 1)
         c2 = -c2;
      c1 = sqrt((1.0 + c1) / 2.0);
   }

   // Normalisation should be done for reverse transform
   if( doNormalisation ) {
      for (i=0;i<nn;i++) {
         realValues[i] /= (double)nn;
         imagValues[i] /= (double)nn;
      }
   }

   return(true);
}

// Not tested yet...
bool csFFTTools::fft_2d( int dir, double** realValues, double** imagValues, int numFFTSamplesX, int numFFTSamplesY )
{
  int mx,my,twopm;

  // Transform the rows
  int max_numFFTSamples = std::max(numFFTSamplesX,numFFTSamplesY);
  double* real = new double[max_numFFTSamples];
  double* imag = new double[max_numFFTSamples];

  if( real == NULL || imag == NULL ) return false;
  if( !csFFTTools::Powerof2( numFFTSamplesX, &mx, &twopm ) || twopm != numFFTSamplesX ) {
    return false;
  }
  if( !csFFTTools::Powerof2( numFFTSamplesY, &my, &twopm ) || twopm != numFFTSamplesY ) {
    return false;
  }
  for( int j = 0; j < numFFTSamplesY; j++ ) {
    for( int i = 0; i < numFFTSamplesX; i++ ) {
      real[i] = realValues[i][j];
      imag[i] = imagValues[i][j];
    }
    csFFTTools::fft(dir,mx,real,imag,false);
    for( int i = 0; i < numFFTSamplesX; i++ ) {
      realValues[i][j] = real[i];
      imagValues[i][j] = imag[i];
    }
  }

   // Transform the columns
  for( int i = 0; i < numFFTSamplesX; i++) {
//    memcpy( real, &realValues[i][0], numFFTSamplesY*sizeof(double) );
//    memcpy( imag, &imagValues[i][0], numFFTSamplesY*sizeof(double) );
    for( int j = 0; j < numFFTSamplesY; j++ ) {
      real[j] = realValues[i][j];
      imag[j] = imagValues[i][j];
    }
    csFFTTools::fft(dir,my,real,imag,false);
//    memcpy( &realValues[i][0], real, numFFTSamplesY*sizeof(double) );
//    memcpy( &imagValues[i][0], imag, numFFTSamplesY*sizeof(double) );
    for( int j = 0; j < numFFTSamplesY; j++ ) {
      realValues[i][j] = real[j];
      imagValues[i][j] = imag[j];
    }
  }
  delete [] real;
  delete [] imag;
  
  return true;
}


