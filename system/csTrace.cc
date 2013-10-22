

#include "csTrace.h"
#include "csTraceHeader.h"
#include "csTraceData.h"
#include "csTracePool.h"
#include "geolib/csException.h"

using namespace cseis_system;

int csTrace::myIdentCounter = 0;

//---------------------------------------------------------
csTrace::csTrace( csTracePool* const tracePoolPtr ) :
  myTracePoolPtr( tracePoolPtr ),
  myIdentNumber( myIdentCounter++ )
{
  myTraceHeader = new csTraceHeader();
  myData        = new csTraceData();
}
//---------------------------------------------------------
csTrace::csTrace() :
  myTracePoolPtr( NULL ),
  myIdentNumber( myIdentCounter++ )
{
  myTraceHeader = new csTraceHeader();
  myData        = new csTraceData();
}
//---------------------------------------------------------
csTrace::~csTrace() {
  if( myTraceHeader ) {
    delete myTraceHeader;
    myTraceHeader = NULL;
  }
  if( myData ) {
    delete myData;
    myData = NULL;
  }
}
//--------------------------------------------------
csTraceHeader* csTrace::getTraceHeader() {
  return myTraceHeader;
}
csTraceHeader const* csTrace::getTraceHeader() const {
  return myTraceHeader;
}
csTraceData const* csTrace::getTraceDataObject() const {
  return myData;
}
csTraceData* csTrace::getTraceDataObject() {
  return myData;
}
float* csTrace::getTraceSamples() {
  return myData->getSamples();
}
float const* csTrace::getTraceSamples() const {
  return myData->getSamples();
}
int csTrace::numSamples() const {
  return myData->numSamples();
}
int csTrace::numHeaders() const {
  return myTraceHeader->numHeaders();
}
//--------------------------------------------------
void csTrace::free() {
  if( myTracePoolPtr != NULL ) {
    myTracePoolPtr->freeTrace( this );
    myTraceHeader->clear();
  }
  else {  // TEMP
    throw cseis_geolib::csException("csTrace: ERROR, pool pointer is NULL");
  }
}
void csTrace::trim() {
  return myData->trim();
}


