

#include "csSelectionManager.h"
#include "geolib/geolib_defines.h"
#include "geolib/csFlexNumber.h"
#include "geolib/csFlexHeader.h"
#include "geolib/csVector.h"
#include "geolib/csSelection.h"
#include "geolib/csException.h"
#include "csTraceHeaderDef.h"
#include "csTraceHeader.h"
#include "geolib/csException.h"
#include <string>

using namespace cseis_system;

//--------------------------------------------------------------
//
csSelectionManager::csSelectionManager() {
  myNumHeaders  = 0;
  myHeaderIndex = NULL;
  myHeaderType  = NULL;
  myHeaderNames = NULL;
  myValues      = NULL;
  mySelection   = NULL;
}
//--------------------------------------------------------------
//
csSelectionManager::~csSelectionManager() {
  if( myHeaderIndex ) { delete [] myHeaderIndex; myHeaderIndex = NULL; }
  if( myHeaderType ) { delete [] myHeaderType; myHeaderType = NULL; }
  if( myHeaderNames ) { delete [] myHeaderNames; myHeaderNames = NULL; }
  if( myValues ) { delete [] myValues; myValues = NULL; }
  if( mySelection ) { delete mySelection; mySelection = NULL; }
  myNumHeaders   = 0;
}
//--------------------------------------------------------------
//
void csSelectionManager::set( cseis_geolib::csVector<std::string> const* headerList, std::string const* selectionText,
    cseis_system::csTraceHeaderDef const* hdef ) {
  if( headerList->size() == 0 ) {
    throw cseis_geolib::csException( "Empty trace header list" );
  }
  myNumHeaders   = headerList->size();
  myHeaderIndex  = new int[myNumHeaders];
  myHeaderType   = new cseis_geolib::type_t[myNumHeaders];
  myHeaderNames  = new std::string[myNumHeaders];
  myValues       = new cseis_geolib::csFlexNumber[myNumHeaders];
  for( int k = 0; k < myNumHeaders; k++ ) {
    std::string name = headerList->at(k);
    myHeaderNames[k] = name;
    if( hdef->headerExists( name.c_str() ) ) {
      myHeaderIndex[k] = hdef->headerIndex( name.c_str() );
      myHeaderType[k]  = hdef->headerType( name.c_str() );
      if( myHeaderType[k] != cseis_geolib::TYPE_INT && myHeaderType[k] != cseis_geolib::TYPE_FLOAT && myHeaderType[k] != cseis_geolib::TYPE_DOUBLE ) {
        throw cseis_geolib::csException( "Trace header '%s': Selection is only supported for number headers.", name.c_str() );
      }
    }
    else {
      throw cseis_geolib::csException( "Unknown trace header: %s", name.c_str() );
    }
  }
  mySelection = new cseis_geolib::csSelection( myNumHeaders, myHeaderType );
  mySelection->add( *selectionText );
}
void csSelectionManager::dump() {
  mySelection->dump();
}
//----------------------------------------------------------
//
bool csSelectionManager::contains( cseis_system::csTraceHeader const* trcHeader ) {
  for( int i = 0; i < myNumHeaders; i++ ) {
    if( myHeaderType[i] == cseis_geolib::TYPE_FLOAT ) {
      myValues[i].setDoubleValue( trcHeader->floatValue(myHeaderIndex[i]) );
//      myValues[i].setFloatValue( trcHeader->floatValue(myHeaderIndex[i]) );
    }
    else if( myHeaderType[i] == cseis_geolib::TYPE_DOUBLE ) {
      myValues[i].setDoubleValue( trcHeader->doubleValue(myHeaderIndex[i]) );
    }
    else { // TYPE_INT
      myValues[i].setIntValue( trcHeader->intValue( myHeaderIndex[i] ) );
    }
  }
  if( mySelection != NULL ) {
    return mySelection->contains( myValues );
  }
  else {
    return true;
  }
}
bool csSelectionManager::contains( cseis_geolib::csFlexHeader const* hdrValue ) {
  for( int i = 0; i < myNumHeaders; i++ ) {
    if( myHeaderType[i] == cseis_geolib::TYPE_FLOAT ) {
      myValues[i].setDoubleValue( hdrValue->floatValue() );
//      myValues[i].setFloatValue( trcHeader->floatValue(myHeaderIndex[i]) );
    }
    else if( myHeaderType[i] == cseis_geolib::TYPE_DOUBLE ) {
      myValues[i].setDoubleValue( hdrValue->doubleValue());
    }
    else { // TYPE_INT
      myValues[i].setIntValue( hdrValue->intValue() );
    }
  }
  if( mySelection != NULL ) {
    return mySelection->contains( myValues );
  }
  else {
    return true;
  }
}
std::string csSelectionManager::headerName( int index ) const {
  if( index >= myNumHeaders ) throw( cseis_geolib::csException("csSelectionManager::headerName: Wrong index passed (%d). This is program bug in the calling function", index) );
  return myHeaderNames[index];
}


