/* Copyright (c) Colorado School of Mines, 2013.*/
/* All rights reserved.                       */



#ifndef SEGD_FUNCTIONS_HH
#define SEGD_FUNCTIONS_HH

#include <string>
#include <cstdio>
#include <ostream>
#include "csSegdDefines.h"

namespace cseis_segd {

  /// 
  int bcd( byte const* bytes, int startNibble, int numNibbles );
  int string2int( byte const* str, int length );
  long string2long( byte const* str, int length );
  double string2double( byte const* str, int length );
  float string2float( byte const* str, int length );

  /// Size in bits of one seismic sample
  int sampleBitSize( int segdFormatCode );
  /// Return true if format code is supported
  bool isFormatCodeSupported( int segdFormatCode );
  /// Return true if manufacturer is supported
  bool isManufacturerSupported( int manufactCode );
  /// Return true if SEGD revision is supported
  bool isRevisionSupported( int rev1, int rev2 );
  /// Get Manufacturer name
  void getManufacturerName( int manufactCode, std::string* text );
  /// Return Manufacturer recording system ID
  int manufacturerRecordingSystem( int manufactCode );

  /// Return external header identifier
  int manufacturerRecordingSystem( int manufactCode );

  void dumpRawHex( std::ostream& os, byte const* buffer, int numBytes );
  void dumpRawHex( FILE* file, byte const* buffer, int numBytes );
  void dumpRawASCII( std::ostream& os, byte const* buffer, int numBytes );

  void dumpCommonHeaders( commonTraceHeaderStruct& comTrcHdr );

} // end namespace

#endif


