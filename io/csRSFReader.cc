
#include "csRSFReader.h"
#include "csRSFHeader.h"
#include "geolib/csException.h"
#include "geolib/csVector.h"
#include "geolib/csFileUtils.h"
#include "geolib/csFlexHeader.h"
#include "geolib/geolib_endian.h"
#include "geolib/geolib_string_utils.h"
#include "geolib/csStandardHeaders.h"
#include <string>
#include <cstring>
#include <cmath>
#include <limits>

using namespace cseis_io;
using namespace cseis_geolib;


csRSFReader::csRSFReader( std::string filenameRSF, int numTracesBuffer, bool reverseByteOrder ) {
  myDataBuffer    = NULL;
  myHasBeenInitialized = false;
  myIsAtEOF = false;
  myHdr          = NULL;
  myPeekHdr = csRSFHeader::HDR_NONE;
  myCopyBuffer = NULL;

  myNumSamples = 0;
  mySampleInt  = 0;
  if( numTracesBuffer <= 0 ) {
    myBufferCapacityNumTraces = 20;
  }
  else {
    myBufferCapacityNumTraces = numTracesBuffer;
  }

  myFilenameRSF  = filenameRSF;
  myIsAtEOF      = false;
  myDoSwapEndian = reverseByteOrder;

  mySampleByteSize = 0;
  myTraceByteSize = 0;
  myFileSize = cseis_geolib::csFileUtils::FILESIZE_UNKNOWN;

  myBufferNumTraces = 0;
  // Index pointer to current trace in buffer
  myBufferCurrentTrace = 0;
  // Total trace counter of all traces accessed via the getNextTrace() method
  myTotalTraceCounter = 0;
  // Total number of traces in input file
  myTotalNumTraces    = 0;
  // File pointer to current trace in input file
  myCurrentFileTraceIndex = 0;
  myCurrentTraceIndex = 0;

  myFileBin = NULL;
}
//-----------------------------------------------------------------------------------------
csRSFReader::~csRSFReader() {
  closeFile();
  if( myCopyBuffer != NULL ) {
    delete [] myCopyBuffer;
    myCopyBuffer = NULL;
  }
  if( myDataBuffer ) {
    delete [] myDataBuffer;
    myDataBuffer = NULL;
  }
  if( myFileBin ) {
    myFileBin->close();
    delete myFileBin;
    myFileBin = NULL;
  }
  if( myHdr ) {
    delete myHdr;
    myHdr = NULL;
  }
}

void csRSFReader::convert2standardUnit() {
  // Convert unit of sample interval to [ms] or [m] if necessary
  if( myHdr->unit1 == csRSFHeader::SAMPLE_UNIT_S ) {
    myHdr->unit1 = csRSFHeader::SAMPLE_UNIT_MS;
    myHdr->d1   *= 1000.0f;
  }
  else if( myHdr->unit1 == csRSFHeader::SAMPLE_UNIT_KM ) {
    myHdr->unit1 = csRSFHeader::SAMPLE_UNIT_M;
    myHdr->d1   *= 1000.0f;
  }

  if( myHdr->unit2 == csRSFHeader::SAMPLE_UNIT_S ) {
    myHdr->unit2 = csRSFHeader::SAMPLE_UNIT_MS;
    myHdr->d1   *= 1000.0f;
  }
  else if( myHdr->unit2 == csRSFHeader::SAMPLE_UNIT_KM ) {
    myHdr->unit2 = csRSFHeader::SAMPLE_UNIT_M;
    myHdr->d1   *= 1000.0f;
  }

  if( myHdr->unit3 == csRSFHeader::SAMPLE_UNIT_S ) {
    myHdr->unit3 = csRSFHeader::SAMPLE_UNIT_MS;
    myHdr->d1   *= 1000.0f;
  }
  else if( myHdr->unit3 == csRSFHeader::SAMPLE_UNIT_KM ) {
    myHdr->unit3 = csRSFHeader::SAMPLE_UNIT_M;
    myHdr->d1   *= 1000.0f;
  }
}

//-----------------------------------------------------------------------------------------
void csRSFReader::initialize( csRSFHeader* hdr ) {
  readRSFHdr();
  convert2standardUnit();
  hdr->set( *myHdr );

  myNumSamples = myHdr->n1;
  mySampleInt  = myHdr->d1;
  if( myHdr->n2 == 0 ) throw( csException("No traces in second dimension. n2=%d", myHdr->n2) );
  if( myHdr->n3 != 0 ) {
    myTotalNumTraces = myHdr->n2 * myHdr->n3;
  }
  else {
    myTotalNumTraces = myHdr->n2;
  }

  mySampleByteSize = 4;   // assume 4 byte floating point
  myTraceByteSize = myNumSamples*mySampleByteSize;

  myBufferCapacityNumTraces = std::min( myTotalNumTraces, myBufferCapacityNumTraces );
  myDataBuffer = new char[ myBufferCapacityNumTraces * myTraceByteSize ];
  if( !myDataBuffer ) {
    throw( csException("Not enough memory. Too many buffered traces requested: %d.", myBufferCapacityNumTraces) );
  }
  memset( myDataBuffer, 0, myBufferCapacityNumTraces * myTraceByteSize );

  myTotalTraceCounter    = 0;
  myBufferCurrentTrace   = 0;

  openBinFile();

  myHasBeenInitialized = true;
}

//-----------------------------------------------------------------------------------------
void csRSFReader::openBinFile() {
  myFileBin = new std::ifstream();
  myFileBin->open( myHdr->filename_bin_full_path.c_str(), std::ios::in | std::ios::binary );
  if( myFileBin->fail() ) {
    throw csException("Cannot open RSF binary file for reading: %s", myHdr->filename_bin_full_path.c_str());
  }
  try {
    myFileSize = cseis_geolib::csFileUtils::retrieveFileSize( myHdr->filename_bin_full_path );
  }
  catch( cseis_geolib::csException& e ) {
    throw cseis_geolib::csException("Error occurred while determining file size. System message: %s\n", e.getMessage() );
  }
}
void csRSFReader::closeFile() {
  if( myFileBin != NULL ) {
    myFileBin->close();
    delete myFileBin;
    myFileBin = NULL;
  }
}

//*******************************************************************
//
// Read RSF header file
//
//*******************************************************************
void csRSFReader::readRSFHdr() {
  if( myHdr != NULL ) delete myHdr;
  myHdr = new csRSFHeader();

  FILE* fin_rsf = fopen(myFilenameRSF.c_str(),"r");
  if( fin_rsf == NULL ) {
    throw csException("Cannot open rsf header file for reading: %s", myFilenameRSF.c_str());
  }

  char buffer[256];
  while( fgets(buffer,256,fin_rsf) != NULL ) {
    if( strlen(buffer) < 2 ) continue;
    std::string bufferStr = trim(buffer);
    if( bufferStr.length() == 0 ) continue;
    char* cPtr = strtok(buffer,"=");
    if( cPtr != NULL ) {
      string name = trim(cPtr);
      cPtr = strtok(NULL,"=");
      if( name.length() > 0 && cPtr != NULL ) {
        string value = trim(cPtr);
        if( value.length() > 0 ) {
          myHdr->setField( name.c_str(), value.c_str() );
        }
      }
    }
  }
  //  myHdr->dump(stderr);
  if( fabs(myHdr->e2-(myHdr->d2*myHdr->n2+myHdr->o2)) > 0.01 ) {
    myHdr->e2 = (myHdr->d2*myHdr->n2+myHdr->o2);
  }
  if( fabs(myHdr->e3-(myHdr->d3*myHdr->n3+myHdr->o3)) > 0.01 ) {
    myHdr->e3 = (myHdr->d3*myHdr->n3+myHdr->o3);
  }
  if( myHdr->filename_bin.length() == 0 ) {
    throw( csException("Cannot extract filename for binary input file (in=... in rsf header file)") );
  }
  if( myHdr->filename_bin.at(0) == '/' ) {
    myHdr->filename_bin_full_path = myHdr->filename_bin;
  }
  else {
    // If binary file name in rsf file does not contain full path,
    // set binary file name with full path
    int counter = myFilenameRSF.length()-1;
    while( counter > 0 ) {
      if( myFilenameRSF.at(counter) == '/' ) {
	break;
      }
      counter -= 1;
    }
    if( counter == 0 ) {
      counter = -1;
    }
    int sizeFile = myHdr->filename_bin.length();
    int sizePath = counter + 1;
    char* tmpStr = new char[sizePath + sizeFile + 1];
    memcpy(tmpStr,myFilenameRSF.c_str(),sizePath);
    memcpy(&tmpStr[sizePath],myHdr->filename_bin.c_str(),sizeFile);
    tmpStr[sizePath+sizeFile] = '\0';
    myHdr->filename_bin_full_path = tmpStr;
    delete [] tmpStr;
  }

  fclose(fin_rsf);
}

//*******************************************************************
//
// Reading one trace...
//
//*******************************************************************
//
bool csRSFReader::getNextTrace( byte_t* theBuffer, int numSamplesToRead ) {
  if( !myHasBeenInitialized ) {
    throw( csException("Accessing method to read first trace before initializing RSF Reader. This is a program bug in the calling method") );
  }
  //  fprintf(stdout,"getNextTrace. bufferNumTrace/total: %d/%d, traceIndex: %d\n",myBufferCurrentTrace, myBufferNumTraces, myCurrentFileTraceIndex);

  if( myBufferCurrentTrace == myBufferNumTraces ) {
    if( myFileBin->eof() ) return false;
    bool success = readDataBuffer();
    if( !success ) return false;
  }

  int minNumSamples = std::min( myNumSamples, numSamplesToRead );
  memcpy( theBuffer, &myDataBuffer[myBufferCurrentTrace*myTraceByteSize], minNumSamples*mySampleByteSize );

  if( numSamplesToRead > myNumSamples ) {
    // Zero out rest of output buffer if necessary:
    memset( &theBuffer[minNumSamples*mySampleByteSize], 0, (numSamplesToRead-minNumSamples)*mySampleByteSize );
  }

  myCurrentTraceIndex = myCurrentFileTraceIndex - (myBufferNumTraces-myBufferCurrentTrace);
  myBufferCurrentTrace += 1;

  return true;
}

//--------------------------------------------------------
bool csRSFReader::readDataBuffer() {
  if( myCurrentFileTraceIndex == myTotalNumTraces ) return false;

  // Set myBufferNumTraces: Number of traces to be read into buffer
  // Make sure only as many traces are read in as necessary:
  myBufferNumTraces = std::min( myBufferCapacityNumTraces, myTotalNumTraces - myCurrentFileTraceIndex );
  //  fprintf(stdout,"readDataBuffer, capacity: %d, totNumTraces: %d, bufferNumTraces: %d, total num bytes: %d\n", myBufferCapacityNumTraces, myTotalNumTraces, myBufferNumTraces, myTraceByteSize*myBufferNumTraces );

  myFileBin->clear(); // Clear all flags
  myFileBin->read( myDataBuffer, myTraceByteSize*myBufferNumTraces );
  if( myFileBin->fail() ) {
    closeFile();
    throw( cseis_geolib::csException("csRSFReader::readDataBuffer: Unexpected error (1) occurred when reading in data from input file '%s'", myFilenameRSF.c_str()) );
  }
  else if( myFileBin->eof() ) {
    return false;
  }
  if( myDoSwapEndian ) {
    for( int itrc = 0; itrc < myBufferNumTraces; itrc++ ) {
      swapEndian4( myDataBuffer+myTraceByteSize*itrc, myTraceByteSize );
    }
  }

  myBufferCurrentTrace = 0;
  myCurrentFileTraceIndex += myBufferNumTraces;
  
  return true;
}
void csRSFReader::dump( FILE* stream ) {
  myHdr->dump( stream );
}
int csRSFReader::computeTrace( double val_dim2, double val_dim3 ) const {
  int traceIndex = (int)round( val_dim3 / myHdr->d3 )*myHdr->n2 + (int)round( val_dim2 / myHdr->d2 );
  return traceIndex;
}
double csRSFReader::computeDim2( int traceIndex ) const {
  if( myHdr == NULL ) throw( cseis_geolib::csException("csRSFReader::computeDim2:") );
  int steps = (int)( traceIndex / myHdr->n2 );
  int remainder = traceIndex - ( steps * myHdr->n2 );
  double value = myHdr->o2 + myHdr->d2 * (double)remainder;
  return value;
}
double csRSFReader::computeDim3( int traceIndex ) const {
  if( myHdr == NULL ) throw( cseis_geolib::csException("csRSFReader::computeDim3:") );
  int steps = (int)( traceIndex / myHdr->n2 );
  double value = myHdr->o3 + myHdr->d3 * (double)(steps);
  return value;
}
bool csRSFReader::moveToComputedTrace( double val_dim2, double val_dim3 ) {
  int traceIndex = (int)round( val_dim3 / myHdr->d3 )*myHdr->n2 + (int)round( val_dim2 / myHdr->d2 );
  return moveToTrace( traceIndex );
}
bool csRSFReader::moveToTrace( int traceIndex ) {
  return moveToTrace( traceIndex, myTotalNumTraces-traceIndex );
}
bool csRSFReader::moveToTrace( int traceIndex, int numTracesToRead ) {
  if( !myHasBeenInitialized ) {
    throw( cseis_geolib::csException("csRSFReader::moveToTrace: Input file has not been initialized yet. This is a program bug in the calling function") );
  }
  else if( myFileSize == cseis_geolib::csFileUtils::FILESIZE_UNKNOWN ) {
    throw( cseis_geolib::csException("csRSFReader::moveToTrace: File size unknown. This may be due to a compatibility problem of this compiled version of the program on the current platform." ) );
  }
  else if( traceIndex < 0 || traceIndex >= myTotalNumTraces ) {
    throw( cseis_geolib::csException("csRSFReader::moveToTrace: Incorrect trace index: %d (number of traces in input file: %d). This is a program bug in the calling method", traceIndex, myTotalNumTraces) );
  }

  if( myCurrentFileTraceIndex != traceIndex ) {
    csInt64_t bytePosRelative = (csInt64_t)(traceIndex-myCurrentFileTraceIndex) * (csInt64_t)myTraceByteSize;
    if( !seekg_relative( bytePosRelative ) ) return false;
    myBufferNumTraces    = 0;
    myBufferCurrentTrace = 0;
  }
  myCurrentFileTraceIndex  = traceIndex;
  myCurrentTraceIndex  = traceIndex;

  // Index of last trace that will be read in one go by consecutive calls to getNextTrace()
  // myLastTraceIndex = std::min( traceIndex + numTracesToRead - 1, myTotalNumTraces-1 );

  return true;
}

bool csRSFReader::setHeaderToPeek( std::string const& headerName ) {
  myPeekHdr = headerIndex( headerName );
  return( myPeekHdr != csRSFHeader::HDR_NONE ); 
}

//--------------------------------------------------------------------

bool csRSFReader::peekHeaderValue( cseis_geolib::csFlexHeader* hdrValue, int traceIndex ) {
  if( !myHasBeenInitialized ) {
    throw( cseis_geolib::csException("csRSFReader::peek: Reader object has not been initialized. This is a program bug in the calling function") );
  }
  else if( myFileSize == cseis_geolib::csFileUtils::FILESIZE_UNKNOWN ) {
    throw( cseis_geolib::csException("csRSFReader::peek: File size unknown. This may be due to a compatibility problem of this compiled version of the program on the current platform." ) );
  }
  else if( myPeekHdr == csRSFHeader::HDR_NONE ) {
    throw(cseis_geolib::csException("csRSFReader::peekHeaderValue: No header has been set for checking. This is a program bug in the calling function"));
  }

  if( traceIndex < 0 || traceIndex >= myTotalNumTraces ) {
    return false;  // Trace beyond end of file. Cannot peek.
  }

  if( myPeekHdr == csRSFHeader::HDR_DIM2 ) {
    hdrValue->setDoubleValue( computeDim2(traceIndex) );
  }
  else if( myPeekHdr == csRSFHeader::HDR_DIM3 ) {
    hdrValue->setDoubleValue( computeDim3(traceIndex) );
  }
  else if( myPeekHdr == csRSFHeader::HDR_TRCNO ) {
    hdrValue->setIntValue( traceIndex+1 );
  }
  return true;;
}

bool csRSFReader::seekg_relative( csInt64_t bytePosRelative ) {
  // Complex algorithm to be able to make step that is larger than 2Gb : Make several smaller steps instead
  
  int maxInt = std::numeric_limits<int>::max() - 1;   // -1 to be on the safe side, also for negative byte positions
  if( bytePosRelative < 0 ) maxInt *= -1;
  
  csInt64_t numSteps  = bytePosRelative / (csInt64_t)maxInt + 1LL;
  int bytePosResidual = (int)(bytePosRelative % (csInt64_t)maxInt);
  
  for( csInt64_t istep = 0; istep < numSteps - 1; ++istep ) { 
    myFileBin->clear(); // Clear all flags
    myFileBin->seekg( maxInt, std::ios_base::cur );
    if( myFileBin->fail() ) return false;
  }
  
  myFileBin->seekg( bytePosResidual, std::ios_base::cur );
  
  return true;
}
//********************************************************************************

int csRSFReader::hdrIntValue( int hdrIndex ) const {
  if( hdrIndex == csRSFHeader::HDR_DIM2 ) {
    return (int)computeDim2(myCurrentTraceIndex);
  }
  else if( hdrIndex == csRSFHeader::HDR_DIM3 ) {
    return (int)computeDim3(myCurrentTraceIndex);
  }
  else if( hdrIndex == csRSFHeader::HDR_TRCNO ) {
    return( myCurrentTraceIndex+1 );
  }
  else {
    return 0;
  }
}
float csRSFReader::hdrFloatValue( int hdrIndex ) const {
  if( hdrIndex == csRSFHeader::HDR_DIM2 ) {
    return (float)computeDim2(myCurrentTraceIndex);
  }
  else if( hdrIndex == csRSFHeader::HDR_DIM3 ) {
    return (float)computeDim3(myCurrentTraceIndex);
  }
  else if( hdrIndex == csRSFHeader::HDR_TRCNO ) {
    return (float)( myCurrentTraceIndex+1 );
  }
  else {
    return 0;
  }
}
double csRSFReader::hdrDoubleValue( int hdrIndex ) const {
  if( hdrIndex == csRSFHeader::HDR_DIM2 ) {
    return computeDim2(myCurrentTraceIndex);
  }
  else if( hdrIndex == csRSFHeader::HDR_DIM3 ) {
    return computeDim3(myCurrentTraceIndex);
  }
  else if( hdrIndex == csRSFHeader::HDR_TRCNO ) {
    return (double)( myCurrentTraceIndex+1 );
  }
  else {
    return 0.0;
  }
}
csInt64_t csRSFReader::hdrInt64Value( int hdrIndex ) const {
  if( hdrIndex == csRSFHeader::HDR_DIM2 ) {
    return (csInt64_t)computeDim2(myCurrentTraceIndex);
  }
  else if( hdrIndex == csRSFHeader::HDR_DIM3 ) {
    return (csInt64_t)computeDim3(myCurrentTraceIndex);
  }
  else if( hdrIndex == csRSFHeader::HDR_TRCNO ) {
    return (csInt64_t)( myCurrentTraceIndex+1 );
  }
  else {
    return 0;
  }
}
std::string csRSFReader::hdrStringValue( int hdrIndex ) const {
  return "";
}

int csRSFReader::numTraceHeaders() const {
  return 3;
}
int csRSFReader::headerIndex( std::string const& headerName ) const {
  if( !headerName.compare( "dim2" ) ) {
    return csRSFHeader::HDR_DIM2;
  }
  else if( !headerName.compare( "dim3" ) ) {
    return csRSFHeader::HDR_DIM3;
  }
  else if( !headerName.compare(cseis_geolib::HDR_TRCNO.name) ) {
    return csRSFHeader::HDR_TRCNO;
  }
  else {
    return csRSFHeader::HDR_NONE;
  }
}
std::string csRSFReader::headerName( int hdrIndex ) const {
  if( hdrIndex == csRSFHeader::HDR_DIM2 ) {
    return "dim2";
  }
  else if( hdrIndex == csRSFHeader::HDR_DIM3 ) {
    return "dim3";
  }
  else if( hdrIndex == csRSFHeader::HDR_TRCNO ) {
    return cseis_geolib::HDR_TRCNO.name;
  }
  else {
    return "NONE";
  }
}
std::string csRSFReader::headerDesc( int hdrIndex ) const {
  if( hdrIndex == csRSFHeader::HDR_DIM2 ) {
    return "RSF data dimension 2";
  }
  else if( hdrIndex == csRSFHeader::HDR_DIM3 ) {
    return "RSF data dimension 3";
  }
  else if( hdrIndex == csRSFHeader::HDR_TRCNO ) {
    return cseis_geolib::HDR_TRCNO.description;
  }
  else {
    return "NONE";
  }
}
cseis_geolib::type_t csRSFReader::headerType( int hdrIndex ) const {
  if( hdrIndex == csRSFHeader::HDR_DIM2 ) {
    return cseis_geolib::TYPE_DOUBLE;
  }
  else if( hdrIndex == csRSFHeader::HDR_DIM3 ) {
    return cseis_geolib::TYPE_DOUBLE;
  }
  else if( hdrIndex == csRSFHeader::HDR_TRCNO ) {
    return cseis_geolib::HDR_TRCNO.type;
  }
  else {
    return cseis_geolib::TYPE_UNKNOWN;
  }
}
float const* csRSFReader::getNextTracePointer() {
  if( myCopyBuffer == NULL ) {
    myCopyBuffer = new float[numSamples()];
  }
  if( getNextTrace( (byte_t*)myCopyBuffer, numSamples() ) ) return myCopyBuffer;
  else return NULL;
}


int csRSFReader::getCSEISDomain() const {
  if( myHdr->unit1 == csRSFHeader::SAMPLE_UNIT_MS || myHdr->unit1 == csRSFHeader::SAMPLE_UNIT_S ) {
    return cseis_geolib::DOMAIN_XT;
  }
  else if( myHdr->unit1 == csRSFHeader::SAMPLE_UNIT_M || myHdr->unit1 == csRSFHeader::SAMPLE_UNIT_KM ) {
    return cseis_geolib::DOMAIN_XD;
  }
  return cseis_geolib::DOMAIN_XT;
}

