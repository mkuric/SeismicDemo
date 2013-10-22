
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include "csPositioning.h"
#include "geolib_methods.h"
#include "csException.h"

using namespace std;
using namespace cseis_geolib;

csPositioning::csPositioning( int inversionType, int maxObs ) {
  myInversionType = inversionType;
  myMaxObs = 0;
  AA       = NULL;
  vec_w    = NULL;
  vec_b    = NULL;
  vec_work = NULL;
  vec_x2   = NULL;

  myNumUnknowns = 0;
  myIdX = -1;
  myIdY = -1;
  myIdZ = -1;
  myIdV = -1;
  myIdT = -1;

  switch( myInversionType ) {
  case SOLVE_XYZ:
    myNumUnknowns = 3;
    myIdX = 0;
    myIdY = 1;
    myIdZ = 2;
    break;
  case SOLVE_XYZT:
    myNumUnknowns = 4;
    myIdX = 0;
    myIdY = 1;
    myIdZ = 2;
    myIdT = 3;
    break;
  case SOLVE_XYZV:
    myNumUnknowns = 4;
    myIdX = 0;
    myIdY = 1;
    myIdZ = 2;
    myIdV = 3;
    break;
  case SOLVE_XYZTV:
    myNumUnknowns = 5;
    myIdX = 0;
    myIdY = 1;
    myIdZ = 2;
    myIdT = 3;
    myIdV = 4;
    break;
  case SOLVE_XZ:
    myIdX = 0;
    myIdZ = 1;
    myNumUnknowns = 2;
    break;
  case SOLVE_XZT:
    myIdX = 0;
    myIdZ = 1;
    myIdT = 2;
    myNumUnknowns = 3;
    break;
  case SOLVE_XZV:
    myIdX = 0;
    myIdZ = 1;
    myIdV = 2;
    myNumUnknowns = 3;
    break;
  case SOLVE_XZTV:
    myIdX = 0;
    myIdZ = 1;
    myIdT = 2;
    myIdV = 3;
    myNumUnknowns = 4;
    break;
  }

  mat_v = new double*[myNumUnknowns];
  for( int k = 0; k < myNumUnknowns; k++ ) {
    mat_v[k] = new double[myNumUnknowns];
  }
  vec_w    = new double[myNumUnknowns];
  vec_x2   = new double[myNumUnknowns];

  reallocate( maxObs );
}

csPositioning::~csPositioning() {
  if( vec_w != NULL ) {
    delete [] vec_w;
    vec_w = NULL;
  }
  if( vec_work != NULL ) {
    delete [] vec_work;
    vec_work = NULL;
  }
  if( vec_x2 != NULL ) {
    delete [] vec_x2;
    vec_x2 = NULL;
  }
  if( vec_b != NULL ) {
    delete [] vec_b;
    vec_b = NULL;
  }
  if( AA != NULL ) {
    for( int i = 0; i < myMaxObs; i++ ) {
      delete [] AA[i];
    }
    delete [] AA;
  }
  if( mat_v != NULL ) {
    for( int k = 0; k < myNumUnknowns; k++ ) {
      delete [] mat_v[k];
    }
    delete [] mat_v;
  }
}

void csPositioning::reallocate( int numObs ) {
  if( AA != NULL ) {
    for( int i = 0; i < myMaxObs; i++ ) {
      delete [] AA[i];
    }
    delete [] AA;
  }
  if( vec_work != NULL ) {
    delete [] vec_work;
  }
  if( vec_b != NULL ) {
    delete [] vec_b;
  }
  myMaxObs = numObs;
  AA = new double*[myMaxObs];
  for( int i = 0; i < myMaxObs; i++ ) {
    AA[i] = new double[myNumUnknowns];
  }
  vec_work = new double[myMaxObs];
  vec_b    = new double[myMaxObs];
}

// Provide initial velocity in variable x1.v
// Provide first break times in variable x2.t
void csPositioning::process( int numObs,
                             PosParam const& x1,
                             PosParam const* x2,
                             double tdelay,
                             double maxOffset,
                             PosParam& solution )
{
  if( numObs > myMaxObs ) reallocate( numObs );

  /*
  fprintf(stderr,"MaxObs: %d, numUnknowns: %d\n", myMaxObs, myNumUnknowns);
  for( int i = 0; i < numObs; i++ ) {
    fprintf(stdout,"%f %f %f   %f %f %f  %f  -- %f %f\n", x1.x, x1.y, x1.z, x2[i].x, x2[i].y, x2[i].z, x2[i].t, tdelay, x1.v);
  }
  */

  int counter = 0;
  if( myInversionType == SOLVE_XYZ || myInversionType == SOLVE_XYZT || myInversionType == SOLVE_XYZV || myInversionType == SOLVE_XYZTV ) {
    for( int i = 0; i < numObs; i++ ) {
      double dx = x1.x - x2[i].x;
      double dy = x1.y - x2[i].y;
      double dz = x1.z - x2[i].z;
      double ds = sqrt( dx*dx + dy*dy + dz*dz );
      if( ds > maxOffset ) continue;

      vec_b[counter] = x2[i].t - (ds/x2[i].v + tdelay);

      AA[counter][0] = dx/(ds*x1.v);
      AA[counter][1] = dy/(ds*x1.v);
      AA[counter][2] = dz/(ds*x1.v);
      if( myInversionType == SOLVE_XYZ ) {
        //nothing
      }
      else if( myInversionType == SOLVE_XYZTV ) {
        AA[counter][3] = 1.0;
        AA[counter][4] = -ds/(x1.v*x1.v);
      }
      else if( myInversionType == SOLVE_XYZT ) {
        AA[counter][3] = 1.0;
      }
      else if( myInversionType == SOLVE_XYZV ) {
        AA[counter][3] = -ds/(x1.v*x1.v);
      }
      counter += 1;
    }
  }
  else {
    for( int i = 0; i < numObs; i++ ) {
      double dx = x1.x - x2[i].x;
      double dy = x1.y - x2[i].y;
      double dz = x1.z - x2[i].z;
      double ds = sqrt( dx*dx + dy*dy + dz*dz );
      if( ds > maxOffset ) continue;

      vec_b[counter] = x2[i].t - (ds/x2[i].v + tdelay);
      
      AA[counter][0] = dx/(ds*x1.v);
      AA[counter][1] = dz/(ds*x1.v);
      if( myInversionType == SOLVE_XZ ) {
        //nothing
      }
      else if( myInversionType == SOLVE_XZTV ) {
        AA[counter][2] = 1.0;
        AA[counter][3] = -ds/(x1.v*x1.v);
      }
      else if( myInversionType == SOLVE_XZT ) {
        AA[counter][2] = 1.0;
      }
      else if( myInversionType == SOLVE_XZV ) {
        AA[counter][2] = -ds/(x1.v*x1.v);
      }
      counter += 1;
    }
  }
  numObs = counter;

  int ret = svd_decomposition( AA, numObs, myNumUnknowns, vec_w, mat_v, vec_work );
  if( !ret ) {
    throw( csException("SVD decomposition failed for unknown reasons. Check input data.") );
  }
  svd_linsolve( AA, vec_w, mat_v, numObs, myNumUnknowns, vec_b, vec_x2, vec_work );

  if( myIdX >= 0 ) solution.x = vec_x2[myIdX];
  if( myIdY >= 0 ) solution.y = vec_x2[myIdY];
  if( myIdZ >= 0 ) solution.z = vec_x2[myIdZ];
  if( myIdT >= 0 ) solution.t = vec_x2[myIdT];
  if( myIdV >= 0 ) solution.v = vec_x2[myIdV];
}

void csPositioning::dump( FILE* stream ) const {
  //  for( int k = 0; k < myNumUnknowns; k++ ) {
  //  fprintf(stream,"%d %12.4f %12.4f %12.4f  --  %f\n", k, x0[k], vec_x2[k], x0[k]+vec_x2[k], stddev[k]);
  // }
}



