

#include "csStackUtil.h"
#include "geolib/csVector.h"
#include "cseis_includes.h"
#include <cmath>

using namespace cseis_system;

csStackUtil::csStackUtil( int nSamples, float normFactor, int outputOption ) {
  init( nSamples, normFactor, outputOption );
}
//csStackUtil::csStackUtil( int nSamples, float normFactor, int outputOption, bool normTimeVariant ) {
//  init( nSamples, normFactor, outputOption, normTimeVariant );
//}
void csStackUtil::init( int nSamples, float normFactor, int outputOption ) {
  myNumSamples   = nSamples;
  myNormFactor   = normFactor;
  myOutputOption = outputOption;

  myNormTimeVariant = false;
  //  myNormTrace = NULL;
  myOutputNormTrace = false;
  myNormTraceList = NULL;
  myNormTraceIndexMap = NULL;
  myHdrId_keyValue = -1;
}
csStackUtil::~csStackUtil() {
  //  if( myNormTrace != NULL ) {
  //  delete [] myNormTrace;
  //   myNormTrace = NULL;
  // }
  if( myNormTraceList != NULL ) {
    for( int i = 0; i < myNormTraceList->size(); i++ ) {
      delete [] myNormTraceList->at(i);
    }
    delete myNormTraceList;
    myNormTraceList = NULL;
  }
  if( myNormTraceIndexMap != NULL ) {
    delete myNormTraceIndexMap;
    myNormTraceIndexMap = NULL;
  }
}
void csStackUtil::setOutputNormTrace( bool doOutputNormTrace ) {
  myOutputNormTrace = doOutputNormTrace;
}
void csStackUtil::setTimeVariantNorm( bool doTimeVariantNorm, int hdrId_keyValue ) {
  myNormTimeVariant = doTimeVariantNorm;
  myHdrId_keyValue  = hdrId_keyValue;
  if( myNormTimeVariant ) {
    //    myNormTrace = new int[myNumSamples];
    //    for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
    //  myNormTrace[isamp] = 0;
    // }
    myNormTraceList = new cseis_geolib::csVector<int*>();
    myNormTraceIndexMap = new std::map<int,int>();
  }
}
//-----------------------------------------------------------
//
void csStackUtil::stackTraces( csTraceGather* traceGather ) {
  int nTraces = traceGather->numTraces();
  if( nTraces == 0 ) return;
  csTrace* traceOut = traceGather->trace(0);

  float* samplesOut = traceGather->trace(0)->getTraceSamples();
  for( int itrc = 1; itrc < nTraces; itrc++ ) {
    float const* samplesIn = traceGather->trace(itrc)->getTraceSamples();
    for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
      samplesOut[isamp] += samplesIn[isamp];
    }
  }
  if( myNormTimeVariant ) {
    for( int itrc = 1; itrc < nTraces; itrc++ ) {
      float const* samplesIn = traceGather->trace(itrc)->getTraceSamples();
      int keyValue = 0;
      if( myHdrId_keyValue >= 0 ) keyValue = traceGather->trace(itrc)->getTraceHeader()->intValue(myHdrId_keyValue);
      std::map<int,int>::iterator iter = myNormTraceIndexMap->find( keyValue );
      if( iter == myNormTraceIndexMap->end() ) {
        throw( cseis_geolib::csException("csStackUtil::stackTraces(): Key value not found: %d\n", keyValue) );
        return; // Oops!
      }
      int* normTrace = myNormTraceList->at( iter->second );
      for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
        if( samplesIn[isamp] != 0.0 ) normTrace[isamp] += 1;
      }
    }
  }
  if( myOutputOption == csStackUtil::OUTPUT_FIRST ) {
    // Nothing to be done
  }
  else if( myOutputOption == csStackUtil::OUTPUT_LAST ) {
    traceOut->getTraceHeader()->copyFrom( traceGather->trace(nTraces-1)->getTraceHeader() );
  }
  else if( myOutputOption == csStackUtil::OUTPUT_AVERAGE ) {
    for( int itrc = 1; itrc < nTraces; itrc++ ) {
      stackHeaders( traceOut->getTraceHeader(), traceGather->trace(itrc)->getTraceHeader() ) ;
    }
  }
  normStackedTrace( traceOut, nTraces );

  traceGather->freeTraces( 1, nTraces-1 );
}
//-----------------------------------------------------------
//
void csStackUtil::stackTraceOld( csTrace* stackedTrace, csTrace const* traceIn ) {
  float* samplesOut = stackedTrace->getTraceSamples();
  float const* samplesIn = traceIn->getTraceSamples();
  for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
    samplesOut[isamp] += samplesIn[isamp];
  }
  //  if( myNormTimeVariant ) {
  //  for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
  //    if( samplesIn[isamp] != 0.0 ) myNormTrace[isamp] += 1;
  //  }
  //  }
  if( myOutputOption == OUTPUT_FIRST ) {
    // Nothing to be done
  }
  else if( myOutputOption == csStackUtil::OUTPUT_LAST ) {
    stackedTrace->getTraceHeader()->copyFrom( traceIn->getTraceHeader() );
  }
  else if( myOutputOption == csStackUtil::OUTPUT_AVERAGE ) {
    stackHeaders( stackedTrace->getTraceHeader(), traceIn->getTraceHeader() ) ;
  }
}
void csStackUtil::stackTrace( csTrace* stackedTrace ) {
  stackTrace( stackedTrace, stackedTrace, true );
}
void csStackUtil::stackTrace( csTrace* stackedTrace, csTrace const* traceIn ) {
  stackTrace( stackedTrace, traceIn, false );
}
void csStackUtil::stackTrace( csTrace* stackedTrace, csTrace const* traceIn, bool newTrace ) {
  // If this is a new trace, no need to stack in data samples and set headers. Otherwise, yes:
  if( !newTrace ) stackTraceOld( stackedTrace, traceIn );
  if( myNormTimeVariant ) {
    int keyValue = 0;
    if( myHdrId_keyValue >= 0 ) keyValue = traceIn->getTraceHeader()->intValue(myHdrId_keyValue);
    
    std::map<int,int>::iterator iter = myNormTraceIndexMap->find( keyValue );
    int* normTrace = NULL;
    if( iter != myNormTraceIndexMap->end() ) {
      // Key value already exists, use existing coverage trace
      normTrace = myNormTraceList->at( iter->second );
    }
    else { // Create new coverage trace:
      normTrace = new int[myNumSamples];
      for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
        normTrace[isamp] = 0;
      }
      myNormTraceIndexMap->insert( std::pair<int,int>( keyValue, myNormTraceList->size() ) );
      myNormTraceList->insertEnd(normTrace);
    }
    float const* samplesIn = traceIn->getTraceSamples();
    for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
      if( samplesIn[isamp] != 0.0 ) normTrace[isamp] += 1;
    }
    //    fprintf(stdout,"STACK %d %d   %f   %d\n", myNormTraceIndexMap->size(), iter->second, keyValue, normTrace[1000] );
  }
}
//-----------------------------------------------------------
//
void csStackUtil::normStackedTraceOld( csTrace* trace, int nTraces ) {
  float* samplesOut = trace->getTraceSamples();
  
  float norm = 1.0;
  if( myNormFactor == 0.5 ) {
    norm = sqrt((float)nTraces);
  }
  else if( myNormFactor == 1.0 ) {
    norm = (float)nTraces;
  }
  else if( myNormFactor == 0.0 ) {
    norm = 1.0;
  }
  else {
    norm = pow( nTraces, myNormFactor );
  }
  if( !myNormTimeVariant ) {
    for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
      samplesOut[isamp] /= norm;
    }
  }
  //  else if( !myOutputNormTrace ) {
  //  for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
  //    if( myNormTrace[isamp] != 0 ) {
  //      samplesOut[isamp] /= pow(myNormTrace[isamp],myNormFactor);
  //    }
  //  }
  // }
  //else {
  //  for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
  //    samplesOut[isamp] = myNormTrace[isamp];
  //  }
  // }
  
  if( myOutputOption == OUTPUT_AVERAGE ) {
    normHeaders( trace->getTraceHeader(), nTraces );
  }
}
void csStackUtil::normStackedTrace( csTrace* trace, int nTraces ) {
  normStackedTraceOld( trace, nTraces );
  float* samplesOut = trace->getTraceSamples();

  if( myNormTimeVariant ) {
    int keyValue = 0;
    if( myHdrId_keyValue >= 0 ) keyValue = trace->getTraceHeader()->intValue(myHdrId_keyValue);
    std::map<int,int>::iterator iter = myNormTraceIndexMap->find( keyValue );
    if( iter == myNormTraceIndexMap->end() ) {
      throw( cseis_geolib::csException("csStackUtil::normStackedTrace(): Key value not found: %d\n", keyValue) );
      return; // Oops!
    }
    int* normTrace = myNormTraceList->at( iter->second );
    if( !myOutputNormTrace ) {
      for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
        if( normTrace[isamp] != 0 ) {
          samplesOut[isamp] /= pow(normTrace[isamp],myNormFactor);
        }
      }
    }
    else {
      //      fprintf(stdout,"NORM %d %d   %f   %d\n", myNormTraceList->size(), iter->second, keyValue, normTrace[1000]);
      for( int isamp = 0; isamp < myNumSamples; isamp++ ) {
        samplesOut[isamp] = normTrace[isamp];
      }
    }
  }
}
void csStackUtil::stackHeaders( csTraceHeader* trcHdrOut, csTraceHeader const* trcHdrIn ) {
  int nHeaders = trcHdrIn->numHeaders();
  for( int ihdr = 0; ihdr < nHeaders; ihdr++ ) {
    cseis_geolib::type_t type = trcHdrIn->type(ihdr);
    switch( type ) {
      case cseis_geolib::TYPE_INT:
        trcHdrOut->setIntValue( ihdr, trcHdrIn->intValue( ihdr ) + trcHdrOut->intValue( ihdr ) );
        break;
      case cseis_geolib::TYPE_FLOAT:
        trcHdrOut->setFloatValue( ihdr, trcHdrIn->floatValue( ihdr ) + trcHdrOut->floatValue( ihdr ) );
        break;
      case cseis_geolib::TYPE_DOUBLE:
        trcHdrOut->setDoubleValue( ihdr, trcHdrIn->doubleValue( ihdr ) + trcHdrOut->doubleValue( ihdr ) );
        break;
    }
  }
}
void csStackUtil::normHeaders( csTraceHeader* trcHdrOut, int nTraces ) {
  int nHeaders = trcHdrOut->numHeaders();
  for( int ihdr = 0; ihdr < nHeaders; ihdr++ ) {
    cseis_geolib::type_t type = trcHdrOut->type(ihdr);
    switch( type ) {
      case cseis_geolib::TYPE_INT:
        trcHdrOut->setIntValue( ihdr, trcHdrOut->intValue( ihdr ) / nTraces );
        break;
      case cseis_geolib::TYPE_FLOAT:
        trcHdrOut->setFloatValue( ihdr, trcHdrOut->floatValue( ihdr ) / nTraces );
        break;
      case cseis_geolib::TYPE_DOUBLE:
        trcHdrOut->setDoubleValue( ihdr, trcHdrOut->doubleValue( ihdr ) / nTraces );
        break;
    }
  }
}
//--------------------------------------------------------------------------------
//
// BUGFIX 080702 Commented out one line in method stackTrace that led to trace headers to be summed ALWAYS, not only for OUTPUT_AVERAGE
//



