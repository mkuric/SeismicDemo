/* Copyright (c) Colorado School of Mines, 2013.*/
/* All rights reserved.                       */



struct PosParam {
public:
PosParam() : x(0), y(0), z(0), t(0), v(0) {}
  void dump() {
    fprintf(stdout,"%d %f %f %f %f %f\n",station,x,y,z,t,v);
  }
  PosParam( PosParam const& pos ) {
    station = pos.station;
    x = pos.x;
    y = pos.y;
    z = pos.z;
    t = pos.t;
    v = pos.v;
  }
public:
  int station;
  double x;
  double y;
  double z;
  double t;
  double v;
};

/**
 * Position source or receivers in OBS survey
 */

class csPositioning {
 public:
  static int const SOLVE_XYZ   = 1;
  static int const SOLVE_XYZT  = 2;
  static int const SOLVE_XYZV  = 3;
  static int const SOLVE_XYZTV = 4;
  static int const SOLVE_XZ    = 5;
  static int const SOLVE_XZT   = 6;
  static int const SOLVE_XZV   = 7;
  static int const SOLVE_XZTV  = 8;

 public:
  csPositioning( int inversionType, int maxObs );
  ~csPositioning();

// Provide initial velocity in variable x1.v
// Provide first break times in variable x2.t
  /**
   * Process one location
   * @param x1        Approximate position of station to be solved. Provide initial velocity in field x1.v
   * @param x2        Positions of all stations of other domain. Provide estimated travel times in fields x2[].t
   * @param tdelay    Time delay [s]
   * @param maxOffset Maximum source-receiver offset [m]  Exclude all source-receiver pairs with higher offset
   * @param solution  Computed position
   */
  void process( int numObs,
                PosParam const& x1,
                PosParam const* x2,
                double tdelay,
                double maxOffset,
                PosParam& solution );
  void dump( FILE* stream = stdout ) const;
 private:
  int myMaxObs;
  int myNumUnknowns;
  int myInversionType;
  int myIdX;
  int myIdY;
  int myIdZ;
  int myIdV;
  int myIdT;

  double** AA;
  double** mat_v;
  double* vec_work;
  double* vec_b;
  double* vec_w;
  double* vec_x2;

  void reallocate( int numObs );
};

}  // end namespace

#endif


