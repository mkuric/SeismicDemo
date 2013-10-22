

#include "cseis_defines.h"
#include "csTraceHeaderDef.h"
#include "csTraceHeaderInfo.h"
#include "geolib/csHeaderInfo.h"
#include "geolib/csGeolibUtils.h"
#include "geolib/csStandardHeaders.h"
#include "csMemoryPoolManager.h"
#include <string>
#include "geolib/csVector.h"

using namespace cseis_system;

std::string csTraceHeaderDef::HEADER_NAME_UNKNOWN = std::string("UNKNOWN_HEADER");

csTraceHeaderDef::csTraceHeaderDef( csTraceHeaderDef* refHdrDefPtr ) {
  init( 1, refHdrDefPtr->myMemoryManager );
  // Always set absolute time headers. These must ALWAYS exist. Added 080724
  addHeader_internal( cseis_geolib::HDR_TIME_SAMP1.type, cseis_geolib::HDR_TIME_SAMP1.name, cseis_geolib::HDR_TIME_SAMP1.description, 1 );
  addHeader_internal( cseis_geolib::HDR_TIME_SAMP1_US.type, cseis_geolib::HDR_TIME_SAMP1_US.name, cseis_geolib::HDR_TIME_SAMP1_US.description, 1 );
}
csTraceHeaderDef::csTraceHeaderDef( int numInputPorts, csTraceHeaderDef const** hdef, csMemoryPoolManager* memManager ) {
  if( numInputPorts > 0 ) {
    init( numInputPorts, memManager );
    initInputPorts( hdef );
  }
  else {
    init( 1, memManager );
    // Always set absolute time headers. These must ALWAYS exist. Added 080724
    addHeader_internal( cseis_geolib::HDR_TIME_SAMP1.type, cseis_geolib::HDR_TIME_SAMP1.name, cseis_geolib::HDR_TIME_SAMP1.description, 1 );
    addHeader_internal( cseis_geolib::HDR_TIME_SAMP1_US.type, cseis_geolib::HDR_TIME_SAMP1_US.name, cseis_geolib::HDR_TIME_SAMP1_US.description, 1 );
  }
}
void csTraceHeaderDef::init( int numInputPorts, csMemoryPoolManager* memManager ) {
  myMemoryManager = memManager;

  myTraceHeaderInfoList = new cseis_geolib::csVector<csTraceHeaderInfo const*>;
  myTotalNumBytes = 0;

  myByteLocation = NULL;

  myIndexOfHeadersToDel = new cseis_geolib::csVector<int>(0);

  myNumInputPorts = numInputPorts;

  myIndexOfHeadersToAdd    = new cseis_geolib::csVector<int>[myNumInputPorts];
  myNumBytesOfHeadersToAdd = new int[myNumInputPorts];
  myNumBytesOfHeadersToAdd[0] = 0;
}
csMemoryPoolManager* csTraceHeaderDef::getMemoryManager() {
  return myMemoryManager;
}

//----------------------------------------------
//
void csTraceHeaderDef::initInputPorts( csTraceHeaderDef const** hdefPrev ) {
  // Copy all headers from first input port
  csTraceHeaderDef const* hdef = hdefPrev[0];
  int nHeaders = hdef->numHeaders();
  cseis_geolib::csVector<int> const* indexOfHeadersToDel = hdef->getIndexOfHeadersToDel();
  int nHeadersToDel = indexOfHeadersToDel->size();
  int counterDel = 0;
  for( int iHdr = 0; iHdr < nHeaders; iHdr++ ) {
    csTraceHeaderInfo const* info = hdef->myTraceHeaderInfoList->at(iHdr);
    // This header shall be deleted, not to be passed on...
    if( nHeadersToDel > counterDel && indexOfHeadersToDel->at(counterDel) == iHdr ) {
      counterDel += 1;
    }
    else {
      myTraceHeaderInfoList->insertEnd( info );
    }
  }
  myTotalNumBytes = hdefPrev[0]->myTotalNumBytes;
  myNumBytesOfHeadersToAdd[0] = 0;

  if( myNumInputPorts > 1 ) {
    // Copy all new headers from input ports 1,2,..N
    for( int inPort = 1; inPort < myNumInputPorts; inPort++ ) {
      hdef = hdefPrev[inPort];
      nHeaders = hdef->numHeaders();
      indexOfHeadersToDel = hdef->getIndexOfHeadersToDel();
      nHeadersToDel = indexOfHeadersToDel->size();
      counterDel = 0;
      for( int iHdr = 0; iHdr < nHeaders; iHdr++ ) {
        csTraceHeaderInfo const* info = hdef->myTraceHeaderInfoList->at(iHdr);
        int index = 0;
        // This header shall be deleted, not to be passed on...
        if( nHeadersToDel > counterDel && indexOfHeadersToDel->at(counterDel) == iHdr ) {
          counterDel += 1;
          continue;
        }
        if( !getIndex(info->name,index) ) {
          myTraceHeaderInfoList->insertEnd( info );
          myTotalNumBytes += cseis_geolib::csGeolibUtils::numBytes( info->type )*info->nElements;
        }
        else { // Header already exists -> check if type is the same
          if( myTraceHeaderInfoList->at( index )->type != info->type ) {
            throw( cseis_geolib::csException("Trace header from different input ports already exists but has different type...") );
          }
          else if( myTraceHeaderInfoList->at( index )->nElements != info->nElements ) {
            throw( cseis_geolib::csException("Trace header from different input ports already exists but has different number of elements...") );
          }
        }
      }
    }
    // Check every input port; if trace header does not exist yet, add it to this port. If header exists and is at a different location --> Exception
    nHeaders = numHeaders();
    for( int inPort = 0; inPort < myNumInputPorts; inPort++ ) {
      hdef = hdefPrev[inPort];
      myIndexOfHeadersToAdd[inPort].clear();
      myNumBytesOfHeadersToAdd[inPort] = 0;
      int ihead = 0;
      while( ihead < nHeaders && hdef->headerName(ihead) == headerName(ihead) ) { ihead++; }
      for( ; ihead < nHeaders; ihead++ ) {
        csTraceHeaderInfo const* info = myTraceHeaderInfoList->at(ihead);
        int indexDummy = 0;
        if( !hdef->getIndex( info->name, indexDummy ) ) {
          myIndexOfHeadersToAdd[inPort].insertEnd( ihead );
          myNumBytesOfHeadersToAdd[inPort] += info->nElements * cseis_geolib::csGeolibUtils::numBytes( info->type );
        }
        else {
//          bool comp = (hdef->headerName(ihead) == headerName(ihead));
//          bool comp2 = hdef->headerName(ihead).compare(headerName(ihead));
//          fprintf(stdout,"CHECK-X Header %d %d: %s  .. %d .. %d ..\n", inPort, ihead, hdef->headerName(ihead).c_str(), comp, comp2);
          // !CHANGE! Add capability to merge input ports with chaotic differences in trace headers, by reshuffling trace headers when merging
          throw( cseis_geolib::csException("Trace headers from different input ports of this module do not match well, or have different order. Merge not possible.") );
        }
      } // END for ihead
    } // END for inPort
  } // END   if( myNumInputPorts > 1 ) {
}
//----------------------------------------------------------------
//
csTraceHeaderDef::~csTraceHeaderDef() {
  if( myTraceHeaderInfoList != NULL ) {
    delete myTraceHeaderInfoList;
    myTraceHeaderInfoList = NULL;
  }
  if( myIndexOfHeadersToAdd != NULL ) {
    delete [] myIndexOfHeadersToAdd;
    myIndexOfHeadersToAdd = NULL;
  }
  if( myNumBytesOfHeadersToAdd != NULL ) {
    delete [] myNumBytesOfHeadersToAdd;
    myNumBytesOfHeadersToAdd = NULL;
  }
  if( myIndexOfHeadersToDel != NULL ) {
    delete myIndexOfHeadersToDel;
    myIndexOfHeadersToDel = NULL;
  }
//  if( myNumBytesOfHeadersToDel != NULL ) {
//    delete myNumBytesOfHeadersToDel;
//    myNumBytesOfHeadersToDel = NULL;
//  }
//  if( myByteLocOfHeadersToDel != NULL ) {
//    delete myByteLocOfHeadersToDel;
//    myByteLocOfHeadersToDel = NULL;
//  }
  if( myByteLocation != NULL ) {
    delete [] myByteLocation;
    myByteLocation = NULL;
  }
}
int csTraceHeaderDef::numHeaders() const {
  return myTraceHeaderInfoList->size();
}
//----------------------------------------------------------------
//
bool csTraceHeaderDef::equals( csTraceHeaderDef const* hdef ) const {
  int nHeaders = numHeaders();
  if( nHeaders != hdef->numHeaders() || getTotalNumBytes() != hdef->getTotalNumBytes() ) return false;
  for( int i = 0; i < nHeaders; i++ ) {
    csTraceHeaderInfo const* info1 = myTraceHeaderInfoList->at( i );
    csTraceHeaderInfo const* info2 = hdef->myTraceHeaderInfoList->at( i );
    if( info1->name.compare( info2->name ) || (info1->type != info2->type ) ) {
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------
int csTraceHeaderDef::headerIndex( std::string const& name ) const {
  int index = 0;
  if( !getIndex( name, index ) ) {
    throw( cseis_geolib::csException("Trace header not found: '%s'\n", name.c_str() ) );
  }
  return index;
}
//----------------------------------------------------------------
//
int csTraceHeaderDef::numElements( int index ) const {
  if( index >= 0 && index < myTraceHeaderInfoList->size() ) {
    return myTraceHeaderInfoList->at(index)->nElements;
  }
  else {
    return 0;
  }
}
//----------------------------------------------------------------
//
cseis_geolib::type_t csTraceHeaderDef::headerType( int index ) const {
  if( myTraceHeaderInfoList == NULL ) return 0;
  if( index >= 0 && index < myTraceHeaderInfoList->size() ) {
    return myTraceHeaderInfoList->at(index)->type;
  }
  else {
    return cseis_geolib::TYPE_UNKNOWN;
  }
}
//----------------------------------------------------------------
//
cseis_geolib::type_t csTraceHeaderDef::headerType( std::string const& name ) const {
  int index = 0;
  getIndex( name, index );
  return headerType( index );
}
//----------------------------------------------------------------
//
std::string csTraceHeaderDef::headerName( int index ) const {
  if( index >= 0 && index < myTraceHeaderInfoList->size() ) {
    return myTraceHeaderInfoList->at(index)->name;
  }
  else {
    return HEADER_NAME_UNKNOWN;
  }
}
std::string csTraceHeaderDef::headerDesc( int index ) const {
  if( index >= 0 && index < myTraceHeaderInfoList->size() ) {
    return myTraceHeaderInfoList->at(index)->description;
  }
  else {
    return HEADER_NAME_UNKNOWN;
  }
}
//----------------------------------------------------------------
//
bool csTraceHeaderDef::headerExists( std::string const& name ) const {
  int index = 0;
  return( getIndex( name, index ) );
}
//----------------------------------------------------------------
//
bool csTraceHeaderDef::getIndex( std::string const& name, int& index ) const {
  for( index = 0; index < myTraceHeaderInfoList->size(); index++ ) {
    if( myTraceHeaderInfoList->at(index)->name == name ) {
      return true;
    }
  }
  index = HEADER_NOT_FOUND;
  return false;
}
//----------------------------------------------------------------
//
csTraceHeaderInfo const* csTraceHeaderDef::headerInfo( int index ) const {
  return myTraceHeaderInfoList->at(index);
}
//----------------------------------------------------------------
//
//int csTraceHeaderDef::addStandardHeader( cseis_geolib::csHeaderInfo const* info ) {
//  return addStandardHeader( info->name );
//}
int csTraceHeaderDef::addStandardHeader( std::string const& name ) {
  cseis_geolib::csHeaderInfo const* info = cseis_geolib::csStandardHeaders::get( name );
  if( info == NULL ) {
    throw( cseis_geolib::csException("Cannot add standard trace header '%s': Standard trace header with this name does not exist. Specify name and type if new non-standard header shall be added.", name.c_str() ) );
  }
  else {
    return addHeader_internal( info->type, info->name, info->description, 1 );
  }
}
int csTraceHeaderDef::addHeader( cseis_geolib::type_t type, std::string const& name, int nElements ) {
  cseis_geolib::csHeaderInfo const* info = cseis_geolib::csStandardHeaders::get( name );
  if( info == NULL ) {
    return addHeader_internal( type, name, "NONE", nElements );
  }
  else if( info->type == type ) {
    return addHeader_internal( type, name, info->description, nElements );
  }
  else {
    throw( cseis_geolib::csException("Cannot add new trace header '%s': Standard header exists with same name but with different type (%s).",
           name.c_str(), cseis_geolib::csGeolibUtils::typeText(info->type)) );
  }
}
int csTraceHeaderDef::addHeader( cseis_geolib::csHeaderInfo const* info, int nElements ) {
  return addHeader( info->type, info->name, info->description, nElements );
}
int csTraceHeaderDef::addHeader( cseis_geolib::type_t type, std::string const& name, std::string const& description, int nElements ) {
  if( name.length() < 1 ) throw( cseis_geolib::csException("csTraceHeaderDef::addHeader: Header name empty. This is a program bug in the calling function") );
  cseis_geolib::csHeaderInfo const* info = cseis_geolib::csStandardHeaders::get( name );
  if( info == NULL || (info->type == type ) ) {
    //  if( info == NULL || (info->type == type && (!info->description.compare(description)) ) ) { // 100510: COMMENTED out to avoid breaking existing module code
    return addHeader_internal( type, name, description, nElements );
  }
  else if( info->type != type ) {
    throw( cseis_geolib::csException("Cannot add new trace header '%s': Standard header with same name exists, but with different type (%s).",
           name.c_str(), cseis_geolib::csGeolibUtils::typeText(info->type)) );
  }
  else { //if( info->description ) {
    throw( cseis_geolib::csException("Cannot add new trace header '%s': Standard header with same name exists, but with different description (%s).",
           name.c_str(), info->description.c_str() ) );
  }
}
int csTraceHeaderDef::addHeader_internal( cseis_geolib::type_t type, std::string const& name, std::string const& description, int nElements ) {
#ifdef CS_DEBUG
  if( type == TYPE_STRING && nElements <= 1 ) {
    throw csException("csTraceHeaderDef::addHeader: Attempted to add STRING header of length = %d. Must specify length of string (argument nElements). This is a program bug in the calling function.", nElements);
  }
#endif
  int index = 0;
  if( !getIndex( name, index ) ) {
    csTraceHeaderInfo const* info = myMemoryManager->getNewTraceHeaderInfo( type, nElements, name, description );
    myTraceHeaderInfoList->insertEnd( info );
    index = numHeaders()-1;
    for( int iport = 0; iport < myNumInputPorts; iport++ ) {
      myIndexOfHeadersToAdd[iport].insertEnd( index );
      myNumBytesOfHeadersToAdd[iport] += info->nElements * cseis_geolib::csGeolibUtils::numBytes( info->type );
    }
    myTotalNumBytes += cseis_geolib::csGeolibUtils::numBytes( type ) * nElements;
    //    printf("New total num bytes: %d (header %s) %d %d\n", myTotalNumBytes, info->name.c_str(), type, nElements);
    return( index );
  }
  else if( type == myTraceHeaderInfoList->at(index)->type ) {
    if( nElements == myTraceHeaderInfoList->at(index)->nElements ) {
      return index;
    }
    else {
      throw( cseis_geolib::csException("csTraceHeaderDef::addHeader(): Trace header already exists but has different number of elements") );
    }
  }
  else {
    throw( cseis_geolib::csException("csTraceHeaderDef::addHeader(): Trace header already exists but has different type") );
  }
}
//----------------------------------------------------------------
//
int csTraceHeaderDef::getByteLocation( int index ) const {
#ifdef CS_DEBUG
  if( index < 0 || index >= myTraceHeaderInfoList->size() ) {
    throw( csException("csTraceHeaderDef::getByteLocation: Wrong header index passed to function") );
  }
#endif
  return myByteLocation[index];  
}
//----------------------------------------------------------------
//
int csTraceHeaderDef::deleteHeader( std::string const& name ) {
  try {
    int index = 0;
    if( getIndex( name, index ) ) {
      if( !isSystemTraceHeader( index ) ) {
        deleteHeader( index );
//      fprintf(stdout,"Delete header: %s index %d  (%d)\n", name.c_str(), index, insertAt);
        return index;
      }
    }
  }
  catch( ... ) {
    //
  }
  return HEADER_NOT_FOUND;
}
void csTraceHeaderDef::deleteAllHeaders() {
  try {
    for( int index = 0; index < numHeaders(); index++ ) {
      if( !isSystemTraceHeader( index ) ) {
        deleteHeader( index );
      }
    }
  }
  catch( ... ) {
    //
  }
}
void csTraceHeaderDef::deleteHeader( int index ) {
  if( index < 0 || index >= numHeaders() ) throw cseis_geolib::csException("csTraceHeaderDef::deleteHeader(): Wrong trace header index passed to method.");
  int insertAt = 0;
  int size = myIndexOfHeadersToDel->size();
  while( insertAt < size && myIndexOfHeadersToDel->at(insertAt) < index ) {
    insertAt++;
  }
  myIndexOfHeadersToDel->insert( index, insertAt );
}

cseis_geolib::csVector<int> const* csTraceHeaderDef::getIndexOfHeadersToAdd( int inPort ) const {
  return &myIndexOfHeadersToAdd[inPort];
}
cseis_geolib::csVector<int> const* csTraceHeaderDef::getIndexOfHeadersToDel() const {
  return myIndexOfHeadersToDel;
}
int csTraceHeaderDef::getNumBytesOfHeadersToAdd( int inPort ) const {
  return myNumBytesOfHeadersToAdd[inPort];
}
void csTraceHeaderDef::dump() const {
  fprintf(stdout,"********* csTraceHeaderDef::dump(), total num bytes: %d *********\n", myTotalNumBytes);
  for( int i = 0; i < myTraceHeaderInfoList->size(); i++ ) {
    csTraceHeaderInfo const* info = myTraceHeaderInfoList->at( i );
    fprintf(stdout,"Info %2d, type %2d: '%-20s', Desc: '%s'\n", i, info->type, info->name.c_str(), info->description.c_str() );
  }
}
void csTraceHeaderDef::resetByteLocation() {
  if( myByteLocation != NULL ) {
    delete [] myByteLocation;
    myByteLocation = NULL;
  }
  int nHeaders = numHeaders();
  myByteLocation = new int[nHeaders];
  myByteLocation[0] = 0;
  int numBytes = 0;
  for( int ihdr = 1; ihdr < nHeaders; ihdr++ ) {
    csTraceHeaderInfo const* info = myTraceHeaderInfoList->at(ihdr-1);
    if( info->type != cseis_geolib::TYPE_STRING ) {
      numBytes += cseis_geolib::csGeolibUtils::numBytes( info->type );
    }
    else { // if( type == TYPE_STRING ) {
      // BUGFIX 080704: Number of bytes for string headers were previously computed as length of description string. This was preliminary code
      numBytes += info->nElements;
    }
    myByteLocation[ihdr] = numBytes;
  }
}

bool csTraceHeaderDef::isSystemTraceHeader( std::string const& name ) const {
  int index = 0;
  getIndex( name, index );
  return( isSystemTraceHeader( index ) );
}

bool csTraceHeaderDef::isSystemTraceHeader( int index ) {
  return( index == csTraceHeaderDef::HDR_ID_TIME_SAMP1_S || index == csTraceHeaderDef::HDR_ID_TIME_SAMP1_US );
}


