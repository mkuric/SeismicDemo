

#include <cmath>
#include <cstring>
#include "csNMOCorrection.h"
#include "csInterpolation.h"
#include "csTimeFunction.h"
#include <iostream>

using namespace cseis_geolib;

namespace cseis_geolib {
  float getQuadAmplitudeAtSample( float const* traceData, double sample, int numSamples );
}

csNMOCorrection::csNMOCorrection( double sampleInt_ms, int numSamples, int method_nmo ) {
  myNMOMethod = method_nmo;
  mySampleInt_sec      = sampleInt_ms/1000.0;
  myNumSamples          = numSamples;
  myTimeOfLastSample    = (myNumSamples-1)*mySampleInt_sec;

  myBuffer = new float[myNumSamples];
  myModeOfApplication = csNMOCorrection::NMO_APPLY;
  myOffsetApex = 4000;
  myIsHorizonBasedNMO = false;
  myVelocityTrace = NULL;
  myTimeTrace     = NULL;
  myTimeTraceInverse = NULL;
  myInterpol = NULL;

  allocateVelocity();
}
//--------------------------------------------------------------------------------
//
csNMOCorrection::~csNMOCorrection() {
  if( myBuffer != NULL ) {
    delete [] myBuffer;
    myBuffer = NULL;
  }
  if( myVelocityTrace != NULL ) {
    delete [] myVelocityTrace;
    myVelocityTrace = NULL;
  }
  if( myTimeTrace != NULL ) {
    delete [] myTimeTrace;
    myTimeTrace = NULL;
  }
  if( myTimeTraceInverse != NULL ) {
    delete [] myTimeTraceInverse;
    myTimeTraceInverse = NULL;
  }
  if( myInterpol != NULL ) {
    delete myInterpol;
    myInterpol = NULL;
  }
}
//--------------------------------------------------------------------------------
//
void csNMOCorrection::setHorizonBasedNMO( bool setHorizonBased, int method_interpolation ) {
  if( myIsHorizonBasedNMO == setHorizonBased ) return;
  myIsHorizonBasedNMO = setHorizonBased;
  myHorInterpolationMethod = method_interpolation;
  allocateVelocity();
}
//--------------------------------------------------------------------------------
//
void csNMOCorrection::allocateVelocity() {
  if( myVelocityTrace != NULL ) {
    delete [] myVelocityTrace;
    myVelocityTrace = NULL;
  }
  if( myTimeTrace != NULL ) {
    delete [] myTimeTrace;
    myTimeTrace = NULL;
  }
  if( myIsHorizonBasedNMO ) {
    myVelocityTrace = NULL;
  }
  else {
    myVelocityTrace = new float[myNumSamples];
    myTimeTrace     = new float[myNumSamples];
    for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
      myTimeTrace[isamp] = (float)( mySampleInt_sec * isamp );
    }
    myInterpol = new csInterpolation( myNumSamples, mySampleInt_sec );
  }
}
//--------------------------------------------------------------------------------
//
void csNMOCorrection::setModeOfApplication( int mode ) {
  myModeOfApplication = mode;
}
//--------------------------------------------------------------------------------
//
void csNMOCorrection::setEmpiricalNMO( double offsetApex_m, double zeroOffsetDamping ) {
  myNMOMethod  = csNMOCorrection::EMPIRICAL_NMO;
  myOffsetApex = offsetApex_m;
  myZeroOffsetDamping = zeroOffsetDamping;
}
//--------------------------------------------------------------------------------
//
void csNMOCorrection::perform_nmo( float const* samplesIn, int numVelocities_in, float const* time_in, float const* vel_rms_in, double offset, float* samplesOut ) {
  memcpy(samplesOut, samplesIn, myNumSamples*sizeof(float) );
  perform_nmo( numVelocities_in, time_in, vel_rms_in, offset, samplesOut );
}

void csNMOCorrection::perform_nmo( int numVelocities_in, float const* time_in, float const* vel_rms_in, double offset, float* samplesOut ) {
  if( myIsHorizonBasedNMO ) {
    return( perform_nmo_horizonBased_prepare( numVelocities_in, time_in, vel_rms_in, offset, samplesOut ) );
  }
  else {
    return( perform_nmo_internal( numVelocities_in, time_in, vel_rms_in, offset, samplesOut ) );
  }
}
//--------------------------------------------------------------------
//
//
void csNMOCorrection::perform_nmo( csTimeFunction<double> const* velTimeFunc, double offset, float* samplesOut ) {
  int numVelocities_in = velTimeFunc->numValues();
  // Horizon based: Add one vel location at top and bottom of trace (temp fix to make sure that interpolation works)
  int numVelocities    = myIsHorizonBasedNMO ? numVelocities_in+2 : numVelocities_in;

  float* t0      = new float[numVelocities];
  float* vel_rms = new float[numVelocities];

  if( myIsHorizonBasedNMO ) {
    t0[0]      = 0;
    vel_rms[0] = velTimeFunc->valueAtIndex(0);
    for( int i = 0; i < numVelocities_in; i++ ) {
      t0[i+1]      = velTimeFunc->timeAtIndex(i)/1000.0;  // Convert to seconds
      vel_rms[i+1] = velTimeFunc->valueAtIndex(i);
    }
    t0[numVelocities-1]      = myTimeOfLastSample;
    vel_rms[numVelocities-1] = vel_rms[numVelocities-2];
    
    perform_nmo_horizonBased( numVelocities, t0, vel_rms, offset, samplesOut );
  }
  else {
    for( int i = 0; i < numVelocities_in; i++ ) {
      t0[i]      = velTimeFunc->timeAtIndex(i)/1000.0;  // Convert to seconds
      vel_rms[i] = velTimeFunc->valueAtIndex(i);
    }

    perform_nmo_internal( numVelocities_in, t0, vel_rms, offset, samplesOut );    
  }
  delete [] t0;
  delete [] vel_rms;
}
//--------------------------------------------------------------------------------
//
//void csNMOCorrection::perform_nmo( int numVelocities_in, float const* time_in, float const* vel_rms_in, double offset, float* samplesOut ) {
//}
void csNMOCorrection::perform_nmo_internal( int numVels, float const* times, float const* vel_rms, double offset, float* samplesOut ) {

  // Linearly interpolate input velocities
  csInterpolation::linearInterpolation( numVels, times, vel_rms, myNumSamples, mySampleInt_sec, myVelocityTrace );

  memcpy( myBuffer, samplesOut, myNumSamples*sizeof(float) );
  double offset_sq = offset*offset;

  for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
    double timeOut = (double)isamp*mySampleInt_sec;
    double timeOut_sq = timeOut * timeOut;
    double vel        = myVelocityTrace[isamp];
    switch( myNMOMethod ) {
    case csNMOCorrection::PP_NMO:
      myTimeTrace[isamp] = sqrt( timeOut_sq + offset_sq / (vel*vel) );
      break;
    case csNMOCorrection::PS_NMO:
      myTimeTrace[isamp] = 0.5 * sqrt( timeOut_sq ) + 0.5 * sqrt( timeOut_sq + 2.0 * offset_sq / (vel*vel) );
      break;
    case csNMOCorrection::EMPIRICAL_NMO:
      myTimeTrace[isamp] = timeOut + ( ( vel * ( pow( (offset-myOffsetApex)/1000,2) - pow(myOffsetApex/1000,2) ) -  (vel/(vel+0.1)) * myZeroOffsetDamping * exp(-0.5*pow(offset/1000,2)) ) / 1000);
      break;
    }
//      fprintf(stdout,"%f %f %f\n", timeOut, myTimeTrace[isamp], myVelocityTrace[isamp]);
  }

  if( myModeOfApplication == csNMOCorrection::NMO_APPLY ) {
    myInterpol->process( mySampleInt_sec, 0.0, myBuffer, myTimeTrace, samplesOut );
  }
  else { // if( myModeOfApplication == csNMOCorrection::NMO_REMOVE ) {
    if( myTimeTraceInverse == NULL ) {
      myTimeTraceInverse  = new float[myNumSamples];
    }
    csInterpolation::xy2yxInterpolation( myTimeTrace, myTimeTraceInverse, myNumSamples, mySampleInt_sec );
    myInterpol->process( mySampleInt_sec, 0.0, myBuffer, myTimeTraceInverse, samplesOut );
  /*
    for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
      myTimeTrace[isamp] /= mySampleInt_sec; // Convert from time to sample index
    }
    csInterpolation::xy2yxInterpolation( myTimeTrace, myTimeTraceInverse, myNumSamples );
    myInterpol->process( 1.0, 0.0, myBuffer, myTimeTraceInverse, samplesOut );
*/
  }
}

//--------------------------------------------------------------------------------
//
void csNMOCorrection::perform_nmo_horizonBased_prepare( int numVelocities_in, float const* t0_in, float const* vel_rms_in, double offset, float* samplesOut ) {
  // Set fields. Add t0 & vel_rms at top and bottom if not there yet
  float* t0      = new float[numVelocities_in+2];
  float* vel_rms = new float[numVelocities_in+2];
  int numVelocities = numVelocities_in;
  if( t0_in[0] > 0 ) {
    t0[0] = 0;
    vel_rms[0] = vel_rms_in[0];
    memcpy( &(t0[1]), t0_in, numVelocities_in*sizeof(float) );
    memcpy( &(vel_rms[1]), vel_rms_in, numVelocities_in*sizeof(float) );
    numVelocities += 1;
  }
  else {
    memcpy( t0, t0_in, numVelocities_in*sizeof(float) );
    memcpy( vel_rms, vel_rms_in, numVelocities_in*sizeof(float) );
  }
  if( t0_in[numVelocities_in-1] < myTimeOfLastSample ) {
    numVelocities += 1;
    t0[numVelocities-1]      = myTimeOfLastSample;
    vel_rms[numVelocities-1] = vel_rms_in[numVelocities_in-1];
  }

  //  fprintf(stderr,"Velocities: %d  time: %f %f, vel: %f %f %f   num: %d\n", numVelocities, t0[0], t0[1], vel_rms[0], vel_rms[1], offset, numVelocities_in);

  perform_nmo_horizonBased( numVelocities, t0, vel_rms, offset, samplesOut );
}
//--------------------------------------------------------------------------------
//
void csNMOCorrection::perform_nmo_horizonBased( int numVelocities, float const* times, float const* vel_rms, double offset, float* samplesInOut ) {
  double offset_sq = offset*offset;

  int currentVelIndex = 0;
  double t0Top    = 0;
  double t0Bot    = times[currentVelIndex];
  double velTop_sq = vel_rms[currentVelIndex]*vel_rms[currentVelIndex];
  double velBot_sq = velTop_sq;
  double time_sqDifference = t0Bot*t0Bot - t0Top*t0Top;
  //  double time_difference   = t0Bot - t0Top;

  double nmo_sign = ( myModeOfApplication == csNMOCorrection::NMO_APPLY ) ? 1.0 : -1.0;

  // Go through all output samples, retrieve sample value at NMO time corresponding to zero-offset t0
  for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
    double timeOut = (double)isamp*mySampleInt_sec;
    while( currentVelIndex < numVelocities && t0Bot <= timeOut ) {
      t0Top      = t0Bot;
      velTop_sq  = velBot_sq;

      currentVelIndex += 1;
      t0Bot      = times[currentVelIndex];
      velBot_sq  = vel_rms[currentVelIndex] * vel_rms[currentVelIndex];
      //      time_difference   = t0Bot - t0Top;
      time_sqDifference = t0Bot*t0Bot - t0Top*t0Top;
    }

    double timeOut_sq = timeOut * timeOut;
    double weight;
    double v_sq;
    if( myHorInterpolationMethod == csNMOCorrection::HORIZON_METHOD_LINEAR ) {
      weight = (t0Bot - timeOut) / (t0Bot - t0Top);
      v_sq   = weight * (velTop_sq - velBot_sq) + velBot_sq;
    }
    else {
      weight = (t0Bot*t0Bot - timeOut_sq)/time_sqDifference;
      v_sq   = weight * (velTop_sq - velBot_sq) + velBot_sq;
    }
    double vel = sqrt( v_sq );

    double timeIn = 0;
    switch( myNMOMethod ) {
    case csNMOCorrection::PP_NMO:
      timeIn = sqrt( timeOut_sq + nmo_sign * offset_sq / v_sq );
      break;
    case csNMOCorrection::PS_NMO:
      timeIn = 0.5 * sqrt( timeOut_sq ) + 0.5 * sqrt( timeOut_sq + nmo_sign *  2.0 * offset_sq / v_sq );
      break;
    case csNMOCorrection::EMPIRICAL_NMO:
      timeIn = timeOut + ( vel * ( pow( (offset-myOffsetApex)/1000,2) - pow(myOffsetApex/1000,2) ) -  (vel/(vel+0.1)) * myZeroOffsetDamping * exp(-0.5*pow(offset/1000,2)) ) / 1000;
      break;
    }
    double sampleIndex = timeIn / mySampleInt_sec;

    myBuffer[isamp] = getQuadAmplitudeAtSample( samplesInOut, sampleIndex, myNumSamples );
    //    fprintf(stderr,"%d  %9.5f %9.5f   %7.2f %7.2f %7.2f %7.3f %8.3f\n", isamp, timeOut, timeIn, sqrt(velTop_sq), sqrt(velBot_sq), sqrt(v_sq), sampleIndex, samplesInOut[isamp] );
    //    fprintf(stderr,"%f %f %f\n", timeOut, vel, timeIn);
  }

  memcpy( samplesInOut, myBuffer, myNumSamples*sizeof(float) );
}




