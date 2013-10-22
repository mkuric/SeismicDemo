

#include "geolib_math.h"
#include "geolib_methods.h"
#include <cmath>
#include <cstdio>

using namespace cseis_geolib;

namespace cseis_geolib {

//---------------------------------------------
  int compute_correlation_length( int maxlag ) {
    return( 2*maxlag+1 );
  }

  /*
   *
   * auto[f(i)] = a[tau] = a[dt * i] = 1 / (m - i + 1) * SUM_j=0-m[ f(j) * f(i+j) ]
   */
  void compute_onesided_auto_correlation( float const* samples, int nSampIn, float* autocorr ) {
    compute_onesided_auto_correlation( samples, nSampIn, autocorr, nSampIn-1 );
  }

  void compute_onesided_auto_correlation( float const* samples, int nSampIn, float* autocorr, int maxlag_in_num_samples ) {
    for( int ilag = 0; ilag <= maxlag_in_num_samples; ilag++ ) {
      float sum = 0;
      int sampEnd = nSampIn-ilag;
      for( int isamp = 0; isamp < sampEnd; isamp++ ) {
        sum += samples[isamp]*samples[isamp+ilag];
      }
      autocorr[ilag] = sum; // (float)sampEnd; BUGFIX 100304: This normalisation was not a good idea. However, no norm at all also seem wrong..
    }
  }

  
  void compute_twosided_correlation( float const* samplesLeft, float const* samplesRight,
                                     int nSampIn, float* corr ) {
    compute_twosided_correlation( samplesLeft, samplesRight, nSampIn, corr, nSampIn-1 );
  }

  void compute_twosided_correlation( float const* samplesLeft, float const* samplesRight,
                                   int nSampIn, float* corr, int maxlag_in_num_samples ) {

    int sampStart;
    int sampEnd;
    
    //---------------------------------------
    // Compute negative lags
    //
    sampEnd    = nSampIn;
    for( int ilag = -maxlag_in_num_samples; ilag < 0; ilag++ ) {
      sampStart  = -ilag;
      float sum = 0;
      for( int isamp = sampStart; isamp < sampEnd; isamp++ ) {
        sum += samplesLeft[isamp]*samplesRight[isamp+ilag];
      }
      int nSamp = sampEnd-sampStart;
      corr[ilag+maxlag_in_num_samples] = sum/(float)nSamp;
    }
    
    //---------------------------------------
    // Compute positive lags
    //
    sampStart  = 0;
    for( int ilag = 0; ilag <= maxlag_in_num_samples; ilag++ ) {
      sampEnd    = nSampIn-ilag;
      float sum = 0;
      for( int isamp = sampStart; isamp < sampEnd; isamp++ ) {
        sum += samplesLeft[isamp]*samplesRight[isamp+ilag];
      }
      int nSamp = sampEnd-sampStart;
      corr[ilag+maxlag_in_num_samples] = sum/(float)nSamp;
    }
  }

  void compute_twosided_correlation( float const* samplesLeft, float const* samplesRight,
                                   int nSampIn, float* corr, int maxlag_in_num_samples, float* sampleIndex_maxAmp, float* maxAmp ) {

    compute_twosided_correlation( samplesLeft, samplesRight, nSampIn, corr, maxlag_in_num_samples );

    // Determine maximum cross-correlation lag time & amplitude
    int nSampCorr = maxlag_in_num_samples*2 + 1;
    int sampleIndex_int = 0;
    *maxAmp = corr[sampleIndex_int];
    for( int isamp = 0; isamp < nSampCorr; isamp++ ) {
      if( corr[isamp] > *maxAmp ) {
        *maxAmp = corr[isamp];
        sampleIndex_int = isamp;
      }
    }
    *sampleIndex_maxAmp = getQuadMaxSample( corr, sampleIndex_int, nSampCorr, maxAmp );
  }

  float compute_rms( float const* samples, int nSamples ) {
    if( nSamples > 0 ) {
      double sum_sqr = 0.0;
      for( int isamp = 0; isamp < nSamples; isamp++ ) {
        sum_sqr += (double)samples[isamp]*(double)samples[isamp];
      }
      return( sqrt( (float)sum_sqr/(float)nSamples ) );
    }
    else {
      return 0.0;
    }
  }

}


