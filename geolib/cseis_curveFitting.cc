

#include "cseis_curveFitting.h"
#include <cmath>
#include <cstdio>
#include "geolib_math.h"
#include "geolib_methods.h"

namespace cseis_geolib {

  void computeXcorCos(
                      double const* angles,
                      double const* values,
                      int     nAngles,
                      int     periodicity,
                      bool    computeStddev,
                      double& result,
                      double& stddev,
                      double& amplitude )
  {
    double maxValue = -9999999.0;
    double phiMaxValue = 0;
    double phi_rad;
    double sum;
    double periodicityFloat = (float)periodicity;

    int maxTestAngle = 360 / periodicity;  // one degree steps

    //for( int i = 0; i < nAngles; i++ ) {
    //  printf("%d  %f %f\n", i, RAD2DEG(angles[i]), values[i]);
    // }

    // 1) Find best fit angle (phi_rad)
    for( int iPhi = 0; iPhi < maxTestAngle; iPhi++ ) {
      phi_rad = double(iPhi) * M_PI / 180.0;
      sum = 0.0;
      for( int i = 0; i < nAngles; i++ ) {
        sum += values[i] * cos( periodicityFloat*( angles[i] - phi_rad) );
      }
      if( sum > maxValue ) {
        maxValue    = sum;
        phiMaxValue = iPhi;
      }
    }
    result = phiMaxValue;
    /// Do quadratic interpolation to get closer to real maximum
    //  float getQuadAmplitudeAtSample( float const* values, phiMaxValue, int numSamples )

    // 2) Find amplitude of cosine function, by least-square's solution for f(A,a) = A*cos(a - phi_rad)
    //    a(i): Specified angles
    //    y(i): Specified amplitude
    //    A   : Amplitude of best-fit cosine function
    //    phi : Phase shift of best-fit cosine function
    //
    //    g(a)  = SUM{ y - A * cos(a-phi) }^2 = MINIMUM
    // -> dg/dA = SUM{ 2*( y - A * cos(a-phi) ) * -cos(a-phi) } = 0
    // ->         2 * SUM{ -y * cos(a-phi) } + 2*A * SUM{ cos^2(a-phi) } = 0
    // ->     A = SUM{ y * cos(a-phi) } / SUM{ cos^2(a-phi) }
    //
    sum = 0.0;
    phi_rad = double(phiMaxValue) * M_PI / 180.0;
    for( int i = 0; i < nAngles; i++ ) {
      double tmp_cos = cos( periodicityFloat*( angles[i] - phi_rad) );
      sum += tmp_cos*tmp_cos;
    }
    if( sum != 0.0 ) {
      amplitude = maxValue/sum;
    }
    else {
      amplitude = 0.0;
    }

    if( computeStddev ) {
      sum = 0.0;
      for( int i = 0; i < nAngles; i++) {
        sum += CS_SQR( values[i] - amplitude*cos( periodicityFloat*( angles[i] - phi_rad) ) );
        //printf("SUM %d %f %f   %f  (%f)\n", i, RAD2DEG(angles[i]), values[i], sum, amplitude*cos( periodicityFloat*( angles[i] - phi_rad) ) );
      }
      stddev = sqrt( sum/(double)nAngles );
    }
    //    fprintf(stderr,"Resultant value: %f %f %f %f\n", phiMaxValue, maxValue, amplitude, stddev );
  }

  //================================================================================
  //

bool polynom_fit( double* xValues, double* yValues, int nValues, int order, double* coefficients ) {
  int nUnknowns = order + 1;

  if( nUnknowns > nValues ) {
    //    fprintf(stderr,"Fewer input values (%d) than unknowns (%d)\n", nValues, nUnknowns );
    return false;
  }
  
  double** mat_v  = NULL;
  double** mat_u  = NULL;
  double*  vec_w  = new double[nValues];
  double*  vec_work = new double[nValues];

  mat_u = new double*[nValues];
  mat_v = new double*[nUnknowns];
  for( int irow = 0; irow < nValues; irow++ ) {
    mat_u[irow] = new double[nUnknowns];
  }
  for( int icol = 0; icol < nUnknowns; icol++ ) {
    mat_v[icol] = new double[nUnknowns];
  }
  
  
  for( int irow = 0; irow < nValues; irow++ ) {
    double xval = xValues[irow];
    mat_u[irow][0] = 1.0;
    for( int icol = 1; icol < nUnknowns; icol++ ) {
      mat_u[irow][icol] = pow( xval, icol );
    }
  }
  int ret = svd_decomposition( mat_u, nValues, nUnknowns, vec_w, mat_v, vec_work );
  if( ret == ERROR ) {
    fprintf(stderr,"Error occurred in SVD\n");
    return false;
  }

  svd_linsolve( mat_u, vec_w, mat_v, nValues, nUnknowns, yValues, coefficients, vec_work );

  for( int irow = 0; irow < nValues; irow++ ) {
    delete [] mat_u[irow];
  }
  for( int icol = 0; icol < nUnknowns; icol++ ) {
    delete [] mat_v[icol];
  }
  delete [] mat_u;
  delete [] mat_v;
  delete [] vec_w;
  delete [] vec_work;

  return true;
}

} // end namespace


