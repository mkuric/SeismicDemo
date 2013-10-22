/* Copyright (c) Colorado School of Mines, 2013.*/
/* All rights reserved.                       */



#ifndef CS_SEISMIC_READER_VER03_H
#define CS_SEISMIC_READER_VER03_H

#include <cstdio>
#include <string>
#include <fstream>
#include "geolib/geolib_defines.h"
#include "csSeismicReader_ver.h"

namespace cseis_io {

/**
 * Seismic file Reader, Cseis format
 *
 * Version 0.3
 * 
 * @author Bjorn Olofsson
 * @date 2012
 */
class csSeismicReader_ver03 : public csSeismicReader_ver {
 public:
  static int const VERSION_SEISMIC_READER   = 03;

 public:
  csSeismicReader_ver03( std::string filename, bool enableRandomAccess, int numTracesBuffer = 0 );
  virtual ~csSeismicReader_ver03();
  virtual bool readFileHeader( csSeismicIOConfig* config );
};

} // end namespace
#endif


