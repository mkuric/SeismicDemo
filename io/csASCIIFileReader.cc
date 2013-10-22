


#include <string>
#include <cstring>
#include <limits>
#include "csASCIIFileReader.h"
#include "geolib/csVector.h"
#include "geolib/csException.h"
#include "geolib/geolib_string_utils.h"

using namespace std;
using namespace cseis_io;
using namespace cseis_geolib;

ASCIIParam::ASCIIParam() {
  sampleList    = new cseis_geolib::csVector<float>(512);
  sampleInt     = 0.0;
  srcDepth      = 0;
  timeFirstSamp = 0;
  timeLastSamp  = 0;
  myNumSamples  = 0;
}
ASCIIParam::~ASCIIParam() {
  if( sampleList != NULL ) {
    sampleList->clear();
    delete sampleList;
    sampleList = NULL;
  }
}
float ASCIIParam::sample(int index) const {
  if( sampleList == NULL || (index < 0 || index >= sampleList->size()) ) {
    throw( cseis_geolib::csException("ASCIIParam::sample(): Program bug: Wrong index passed") );
  }
  return sampleList->at(index);
}
int ASCIIParam::numSamples() const {
  return myNumSamples;
}
float const* ASCIIParam::getSamples() const {
  if( sampleList == NULL ) return NULL;
  return sampleList->toArray();
}
void ASCIIParam::clear() {
  if( sampleList != NULL ) sampleList->clear();
}

//--------------------------------------------------------------------------------
//
csASCIIFileReader::csASCIIFileReader( std::string const& filename, int format ) {
  myFormat = format;
  myCurrentTraceIndex = 0;
  myCounterLines = 0;
  myIsAtEOF   = false;
  myFileASCII = fopen( filename.c_str(), "r" );  myCurrentTraceIndex = 0;

  if( myFileASCII == NULL ) {
    throw( cseis_geolib::csException("Could not open file '%s'", filename.c_str() ) );
  }
}

csASCIIFileReader::~csASCIIFileReader() {
  if( myFileASCII != NULL ) {
    fclose( myFileASCII );
    myFileASCII = NULL;
  }
}

bool csASCIIFileReader::isAtEOF() const {
  return myIsAtEOF;
}

//--------------------------------------------------------------------------------

bool csASCIIFileReader::initialize( ASCIIParam* param ) {
  if( myCurrentTraceIndex > 0 ) return false;

  if( myFormat == FORMAT_NUCLEUS_SIGNATURE ) {
    bool isReadingHdr = true;
    cseis_geolib::csVector<std::string> tokenList;
    int counter = 0;
    bool isNucleusPlus = false;
    while( fgets( myBuffer, 1024, myFileASCII ) != NULL && isReadingHdr ) {
      counter += 1;
      tokenList.clear();
      cseis_geolib::tokenize( myBuffer, tokenList, false );
      if( tokenList.size() > 3 ) {
        if( !tokenList.at(5).compare("Nucleus+") ) {
          isNucleusPlus = true;
        }
        else if( !isNucleusPlus ) {
          if( !strncmp(myBuffer,"Source depth",12) ) {
            param->srcDepth = atof(tokenList.at(3).c_str());
          }
          else if( !strncmp(myBuffer,"Sample interval",15) ) {
            param->sampleInt = atof(tokenList.at(3).c_str());
          }
          else if( !strncmp(myBuffer,"  Time of 1st sample",20) ) {
            if( tokenList.size() > 4 ) {
              param->timeFirstSamp = atof(tokenList.at(4).c_str());
            }
          }
          else if( !strncmp(myBuffer,"Time is increasing",18) ) {
            isReadingHdr = false;
          }
        }
        else { // This is NUcleus+ version
          if( !strncmp(myBuffer,"Source average depth",20) ) {
            param->srcDepth = atof(tokenList.at(5).c_str());
          }
          else if( !strncmp(myBuffer,"Sample interval",15) ) {
            param->sampleInt = atof(tokenList.at(4).c_str());
          }
          else if( !strncmp(myBuffer,"  Start time (ms)",17) ) {
            if( tokenList.size() > 4 ) {
              param->timeFirstSamp = atof(tokenList.at(4).c_str());
            }
          }
          else if( !strncmp(myBuffer,"Time is increasing",18) ) {
            isReadingHdr = false;
          }
        }
      }
    }

    // Read in data to get number of samples:
    bool success = csASCIIFileReader::readNextTrace( param );
    if( !success ) return false;
    param->myNumSamples = param->sampleList->size();
    // Rewind input file and set pointer back to start of data
    rewind( myFileASCII );
    for( int i = 0; i < counter; i++ ) {
      success = fgets( myBuffer, 1024, myFileASCII );
    }
    myIsAtEOF = false;
    myCurrentTraceIndex = 0;
  }
  //--------------------------------------------------------------------------------
  //
  else if( myFormat == FORMAT_COLUMNS ) {
    csVector<double> timeList;
    csVector<float> sampleList;
    readOneTraceColumnFormat( &timeList, &sampleList, numeric_limits<int>::max(), 1 );
    myNumSamples = sampleList.size();
    param->myNumSamples = myNumSamples;
    if( myNumSamples == 0 ) {
      throw( csException("Input file does not contain any valid data") );
    }
    param->timeFirstSamp = timeList.at(0);
    param->timeLastSamp  = timeList.at(myNumSamples-1);
    param->sampleInt = (float)( timeList.at(1) - timeList.at(0) );
    rewind( myFileASCII );
  }
  //----------------------------------------------------------------------
  //
  else if( myFormat == FORMAT_ZMAP ) {
    throw( csException("csASCIIFileReader::initialize: For format 'zmap', use a different initialize function") );
  }

  return true;
}

bool csASCIIFileReader::initializeZMap( ASCIIParam* param,
                                        int& zmap_numTraces,
                                        double& zmap_x1,
                                        double& zmap_y1,
                                        double& zmap_x2,
                                        double& zmap_y2 )
{
  if( myCurrentTraceIndex > 0 ) return false;
  if( myFormat != FORMAT_ZMAP ) throw( csException("csASCIIFileReader::initializeZMap: Wrong ASCII file format. This is a program bug in the calling function") );

    if( fgets( myBuffer, 1024, myFileASCII ) == NULL ) {
      throw( csException("ASCII input file contains no data.") );
    }
    myCounterLines = 1;
    fprintf(stderr,"LINE #%d: %s\n", myCounterLines, myBuffer);
    if( myBuffer[0] != '!' ) {
      throw( csException("Unexpected character found in line #%d: %s", myCounterLines+1, myBuffer) );
    }
    while( fgets( myBuffer, 1024, myFileASCII ) != NULL ) {
      if( myBuffer[0] != '!' ) {
	break;
      }
      fprintf(stderr,"LINE #%d: %s\n", myCounterLines+1, myBuffer);
      myCounterLines += 1;
    }
    if( myBuffer[0] != '@' ) {
      throw( csException("Unexpected character found in line #%d: %s", myCounterLines+1, myBuffer) );
    }
    if( fgets( myBuffer, 1024, myFileASCII ) == NULL ) {
      throw( csException("Unexpected end of file") );
    }
    csVector<std::string> valueList;
    tokenize( myBuffer, valueList );
    myZMap_noValue = atof(valueList.at(1).c_str());

    if( fgets( myBuffer, 1024, myFileASCII ) == NULL ) {
      throw( csException("Unexpected end of file") );
    }
    valueList.clear();
    tokenize( myBuffer, valueList );

    myNumColumns = valueList.size();
    if( myNumColumns < 6 ) {
    }
    param->myNumSamples = atoi(valueList.at(0).c_str());
    zmap_numTraces  = atoi(valueList.at(1).c_str());
    zmap_x1   = atof(valueList.at(2).c_str());
    zmap_y1   = atof(valueList.at(3).c_str());
    zmap_x2   = atof(valueList.at(4).c_str());
    zmap_y2   = atof(valueList.at(5).c_str());
    param->sampleInt = 1000 * (zmap_x1-zmap_x2) / param->myNumSamples;

    if( fgets( myBuffer, 1024, myFileASCII ) == NULL ) {
      throw( csException("Unexpected end of file") );
    }
    if( fgets( myBuffer, 1024, myFileASCII ) == NULL ) {
      throw( csException("Unexpected end of file") );
    }
    myNumSamples = param->myNumSamples;
    /*
    vars->zmap_numValues = atoi(valueList.at(0).c_str());
    vars->zmap_numBlocks = atoi(valueList.at(1).c_str());

    fprintf(stderr,"DEBUG INFO: %d %d   %f  %f %f %f %f\n", vars->zmap_numValues, vars->zmap_numBlocks, vars->zmap_noValue,
	    vars->zmap_x1, vars->zmap_y1, vars->zmap_x2, vars->zmap_y2);
    fprintf(stderr,"...ZMAP format not fully supported yet.\n");

    vars->samples = new float[numSamples];
    vars->hdrId_trcno = hdef->addStandardHeader(cseis_geolib::HDR_TRCNO.name);
    vars->hdrId_biny = hdef->addStandardHeader(cseis_geolib::HDR_BIN_Y.name);
    */

    myCurrentTraceIndex += 1;

  return true;
}


//--------------------------------------------------------------------------------

bool csASCIIFileReader::readNextTrace( ASCIIParam* param ) {
  if( myIsAtEOF ) return false;

  bool retValue = false;
  param->sampleList->clear();

  if( myFormat == FORMAT_NUCLEUS_SIGNATURE ) {
    cseis_geolib::csVector<std::string> valueList;
    while( fgets( myBuffer, 1024, myFileASCII ) != NULL ) {
      tokenize( myBuffer, valueList );
      if( valueList.size() == 0 ) continue;  // Read over blank lines
      for( int i = 0; i < valueList.size(); i++ ) {
        float sampleValue = atof( valueList.at(i).c_str() );
        param->sampleList->insertEnd( sampleValue );
      }
      valueList.clear();
    }
    myIsAtEOF = true;
    retValue = true;
  }
  else if( myFormat == FORMAT_COLUMNS ) {
    csVector<double> timeList;
    bool success = readOneTraceColumnFormat( &timeList, param->sampleList, myNumSamples, myCurrentTraceIndex+1 );
    if( !success ) {
      throw( csException("Inconsistent data in input file. Terminated while reading trace #%d, number of samples: %d, expected number of samples: %d",
                         myCurrentTraceIndex+1, param->sampleList->size(), myNumSamples) );
    }
    if( param->sampleList->size() == 0 ) {
      retValue = false;
    }
    else {
      param->timeFirstSamp = timeList.at(0);
      param->timeLastSamp  = timeList.at(timeList.size()-1);
      param->sampleInt = (float)( timeList.at(1) - timeList.at(0) );
      retValue = true;
    }
  }
  else if( myFormat == FORMAT_ZMAP ) {
    csVector<std::string> valueList;
    int counterLines  = 0;
    bool success = fgets( myBuffer, 1024, myFileASCII );
    if( !success ) {
      myIsAtEOF = true;
      retValue = false;
    }
    else {
      retValue = true;
      while( success ) {
        valueList.clear();
        tokenize( myBuffer, valueList );
        for( int i = 0; i < valueList.size(); i++ ) {
          double value = atof(valueList.at(i).c_str());
          if( value == myZMap_noValue ) value = 0.0;
          param->sampleList->insertEnd(value);
          if( param->sampleList->size() >= myNumSamples ) break;
        }
        counterLines += 1;
        if( param->sampleList->size() >= myNumSamples ) break;
        success = fgets( myBuffer, 1024, myFileASCII );
      }
    }
  }

  if( retValue ) myCurrentTraceIndex += 1;
  return retValue;
}

//--------------------------------------------------------------------------------
//
//
bool csASCIIFileReader::readOneTraceColumnFormat( cseis_geolib::csVector<double>* timeList,
                                                  cseis_geolib::csVector<float>* sampleList,
                                                  int maxSamplesToRead,
                                                  int traceIndexToRead )
{
  sampleList->clear();
  cseis_geolib::csVector<std::string> valueList;

  int counter = 0;
  double timePrev    = 0.0;
  double timeCurrent = 0.0;
  int trcIndex = traceIndexToRead;

  while( counter < maxSamplesToRead && fgets( myBuffer, 1024, myFileASCII ) != NULL ) {
    valueList.clear();
    cseis_geolib::tokenize( myBuffer, valueList );
    if( valueList.size() == 2 ) {
      // Nothing..
    }
    else if( valueList.size() > 2 ) {
      trcIndex = atoi( valueList.at(2).c_str() );
    }
    else if( valueList.size() == 0 ) {
      continue; // Skip blank lines
    }
    else { //  if( valueList.size() < 2 ) {
      throw( cseis_geolib::csException("Incorrect number of columns (=%d) in input file, line #%d",
                                       valueList.size(), counter+myCounterLines ));
    }
    timeCurrent = atof( valueList.at(0).c_str() );

    if( timeCurrent < timePrev || trcIndex != traceIndexToRead ) { // Time jumps backwards, new trace index --> New trace
      return false;
    }
    timeList->insertEnd(timeCurrent);
    float sampleValue = atof( valueList.at(1).c_str() );
    sampleList->insertEnd( sampleValue );

    counter += 1;
  }
  return true;
}


