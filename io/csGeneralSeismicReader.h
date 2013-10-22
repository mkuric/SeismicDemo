/* Copyright (c) Colorado School of Mines, 2013.*/
/* All rights reserved.                       */



#ifndef CS_GENERAL_SEISMIC_READER_H
#define CS_GENERAL_SEISMIC_READER_H

#include <cstdio>
#include <string>
#include "geolib/geolib_defines.h"
#include "geolib/csIReader.h"

namespace cseis_geolib {
  class csFlexHeader;
}

namespace cseis_system {
  class csSuperHeader;
  class csTraceHeaderDef;
}  

namespace cseis_io {
  class csSeismicReader_ver;
  class csSeismicIOConfig;

/**
 * General seismic file reader, Cseis format
 *
 * @author Bjorn Olofsson
 * @date 2006
 */
class csGeneralSeismicReader {
public:
  csGeneralSeismicReader( std::string filename, bool enableRandomAccess, int numTracesToBuffer );
  ~csGeneralSeismicReader();

  bool readFileHeader();
  bool moveToTrace( int traceIndex, int numTracesToRead );
  bool moveToTrace( int traceIndex );
  float const* readTraceReturnPointer();
  bool readTrace( float* samples );
   
  void closeFile();

  int hdrIntValue( int hdrIndex ) const;
  float hdrFloatValue( int hdrIndex ) const;
  double hdrDoubleValue( int hdrIndex ) const;
  csInt64_t hdrInt64Value( int hdrIndex ) const;
  std::string hdrStringValue( int hdrIndex ) const;

  int numTraceHeaders() const;
  int headerIndex( std::string const& headerName ) const;
  std::string headerName( int hdrIndex ) const;
  std::string headerDesc( int hdrIndex ) const;
  cseis_geolib::type_t headerType( int hdrIndex ) const;
  int headerElements( int hdrIndex ) const;
  csSeismicIOConfig const* getSeismicConfig() const { return myConfig; } 
 
  int numTraces();
 
  bool setHeaderToPeek( std::string const& headerName );

  bool peekHeaderValue( cseis_geolib::csFlexHeader* hdrValue, int traceIndex = -1 );
private:
  void setByteLocation();

  csSeismicReader_ver* myReader;
  csSeismicIOConfig* myConfig;

  bool    myIsFileHeaderRead;
  int*    myByteLocation;

  float*  myTraceBuffer;
  
  char*   myHdrValueBlock;          // byte header buffer
  int*    myIntPtr;                 // header buffer as int
  float*  myFloatPtr;               // header buffer as float
  
  /// Header to check. 
  int  myHdrCheckByteOffset;
  cseis_geolib::type_t myHdrCheckType;
  int   myHdrCheckByteSize;
  char* myHdrCheckBuffer;
};

} // end namespace
#endif


