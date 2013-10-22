/* Copyright (c) Colorado School of Mines, 2013.*/
/* All rights reserved.                       */


#ifndef CS_RSF_READER_H
#define CS_RSF_READER_H

#include <cstdio>
#include <fstream>
#include <string>
#include <geolib/geolib_defines.h>

namespace cseis_geolib {
  class csFlexHeader;
}

namespace cseis_io {

  class csRSFHeader;

class csRSFReader {
//---------------------------------------------------------------------------------------
public:
  csRSFReader( std::string filename, int nTracesBuffer, bool reverseByteOrder );
  ~csRSFReader();
  //
  void initialize( csRSFHeader* );

  /// @return number of bytes per sample
  inline int sampleByteSize() const { return mySampleByteSize; }
  inline int totalTraceSize() const { return myTraceByteSize; }
  inline char const* filename() const { return myFilenameRSF.c_str(); }
  void closeFile();
  inline int numSamples() const { return myNumSamples; }
  inline float sampleInt() const { return mySampleInt; }
  inline int numTraces(void) const { return myTotalNumTraces; };

  bool moveToTrace( int firstTraceIndex );
  bool moveToTrace( int firstTraceIndex, int numTracesToRead );
  bool moveToComputedTrace( double val_dim2, double val_dim3 );

  int computeTrace( double val_dim2, double val_dim3 ) const;
  double computeDim2( int traceIndex ) const;
  double computeDim3( int traceIndex ) const;

  bool setHeaderToPeek( std::string const& headerName );
  bool peekHeaderValue( cseis_geolib::csFlexHeader* hdrValue, int traceIndex = -1 );   

  bool getNextTrace( cseis_geolib::byte_t* buffer, int numSamplesToRead );
  float const* getNextTracePointer();
  void dump( FILE* stream );


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

  /// @return CSEIS domain code defining data 'domain', for example time or depth..
  int getCSEISDomain() const;
private:

  bool seekg_relative( csInt64_t bytePosRelative );
  void convert2standardUnit();
  
  csRSFHeader* myHdr;
  int myPeekHdr;
  float* myCopyBuffer;

  int   mySampleByteSize;
  int   myTraceByteSize;
  /// File size in bytes
  csInt64_t myFileSize;

  bool  myIsAtEOF;
  char* myDataBuffer;
  /// Index pointer to current trace in buffer
  int myBufferCurrentTrace;
  /// Capacity in number of traces of big buffer = Maximum number of traces that can be buffered at once
  int myBufferCapacityNumTraces;
  /// Total number of traces that are currently stored in the buffer
  int myBufferNumTraces;

  /// Total trace counter of all traces accessed via the getNextTrace() method
  int myTotalTraceCounter;
  /// Current file pointer: Points to trace with given index in input file
  /// This is the trace that the file pointer points at, not the trace which has just been retrieved with getNextTrace()
  int myCurrentFileTraceIndex;
  /// The current trace that has most recently been retrieved by getNextTrace(), ro which is about to be retrieved after a moveTo operation
  int myCurrentTraceIndex;
  /// Total number of traces in file
  int myTotalNumTraces;
  /// true if endian swapping shall be performed.
  bool myDoSwapEndian;

  std::ifstream*  myFileBin;
  std::string myFilenameRSF;

  float mySampleInt;
  int   myNumSamples;

  bool myHasBeenInitialized;
//-----------------------------------------------------------------------------------------
//
private:
  void openBinFile();
  void readRSFHdr();
  bool readDataBuffer();
  int currentTraceIndex() const;

  csRSFReader();
  csRSFReader( csRSFReader const& obj );
  csRSFReader& operator=( csRSFReader const& obj );
};

} // namespace

#endif


