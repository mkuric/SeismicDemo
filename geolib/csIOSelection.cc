
#include <cstdio>
#include "csIOSelection.h"
#include "csFlexHeader.h"
#include "csFlexNumber.h"
#include "csIReader.h"
#include "csException.h"
#include "csSelection.h"
#include "csSortManager.h"
#include "csVector.h"

using namespace std;
using namespace cseis_geolib;

csIOSelection::csIOSelection( std::string const& headerName, int sortOrder, int sortMethod ) {
  myHdrName   = headerName;
  mySortOrder = sortOrder;
  myNumSelectedTraces = 0;
  mySortManager = NULL;
  mySelectedTraceIndexList = new cseis_geolib::csVector<int>();

  if( mySortOrder != csIOSelection::SORT_NONE ) {
    mySortManager = new csSortManager( 1, sortMethod );
  }
  myCurrentSelectedIndex = -1;
}
csIOSelection::~csIOSelection() {
  if( mySortManager != NULL ) {
    delete mySortManager;
    mySortManager = NULL;
  }
  if( mySelectedTraceIndexList != NULL ) {
    delete mySelectedTraceIndexList;
    mySelectedTraceIndexList = NULL;
  }
}
bool csIOSelection::initialize( cseis_geolib::csIReader* reader, std::string const& hdrValueSelectionText ) {
  cseis_geolib::type_t hdrType;
  reader->setHeaderToPeek( myHdrName, hdrType );
  reader->moveToTrace( 0 );  // CHANGE: Save current file position?
  csSelection selection( 1, &hdrType );
  selection.add( hdrValueSelectionText );
  int numTraces = reader->numTraces();
  cseis_geolib::csVector<csFlexNumber*> selectedValueList;
  for( int itrc = 0; itrc < numTraces; itrc++ ) {
    csFlexHeader flexHdr;
    if( reader->peekHeaderValue( &flexHdr, itrc ) ) {
      csFlexNumber value(&flexHdr);
      if( selection.contains( &value ) ) {
        mySelectedTraceIndexList->insertEnd(itrc);
        if( mySortOrder != csIOSelection::SORT_NONE ) selectedValueList.insertEnd( new csFlexNumber(&value,mySortOrder == SORT_DECREASING) );
        //        fprintf(stderr,"SET value #%d   %d\n", selectedValueList.size(), selectedValueList.at( selectedValueList.size()-1 )->intValue()  );
        // fflush(stderr);
      }
    }
    else {
      throw( csException("csIOSelection::initialize: Error occurred when scanning header value for trace #%d", itrc+1) );
    }
  }
  // Move reader back to beginning
  reader->moveToTrace( 0 );

  // No traces found:
  myNumSelectedTraces = mySelectedTraceIndexList->size();
  if( myNumSelectedTraces == 0 ) {
    return false;
  }

  if( mySortOrder != csIOSelection::SORT_NONE ) {
    mySortManager->resetValues( myNumSelectedTraces );
    for( int is = 0; is < myNumSelectedTraces; is++ ) {
      csFlexNumber* flexNum = selectedValueList.at( is );
      //      fprintf(stdout,"Value #%d   %d    %d, trace %d\n", is, flexNum->intValue(), myNumSelectedTraces, mySelectedTraceIndexList->at(is) );
      mySortManager->setValue( is, 0, *flexNum, mySelectedTraceIndexList->at(is) );
      delete flexNum;
    }
    selectedValueList.clear();
    mySelectedTraceIndexList->clear();
    //    fprintf(stdout,"START sort...\n");
    //  fprintf(stdout,"START sorting %d ( =? %d ) traces...\n", myNumSelectedTraces, mySortManager->numValues());
    // mySortManager->dump();
    // fflush(stdout);
    mySortManager->sort();
    // fprintf(stdout,"END sort...\n");
    // fflush(stdout);
  }
  //  else {
  //  for( int is = 0; is < myNumSelectedTraces; is++ ) {
  //    fprintf(stdout,"Value #%d    %d, trace %d\n", is, myNumSelectedTraces, mySelectedTraceIndexList->at(is) );
  //  }
  //}
  myCurrentSelectedIndex = -1;
  return true;
}

int csIOSelection::getNextTraceIndex() {
  //  fprintf(stdout,"csIOSelection::getTraceIndex(): Call %d/%d.\n", myCurrentSelectedIndex+1, myNumSelectedTraces);
  myCurrentSelectedIndex += 1;
  if( myCurrentSelectedIndex >= myNumSelectedTraces ) {
    //throw( csException("csIOSelection::getTraceIndex(): Incorrect call to function. All traces have already been returned.") );
    //fprintf(stdout,"csIOSelection::getTraceIndex(): Incorrect call to function. All traces have already been returned.\n");
    return -1;
  }
  if( mySortOrder == SORT_NONE ) {
    //    fprintf(stdout,"    ...selected trace index:  %d\n",  mySelectedTraceIndexList->at(myCurrentSelectedIndex) );
    return mySelectedTraceIndexList->at(myCurrentSelectedIndex);
  }
  else {
    //   fprintf(stdout,"    ...sorted trace index:  %d\n",  mySortManager->sortedIndex(myCurrentSelectedIndex) );
    return mySortManager->sortedIndex(myCurrentSelectedIndex);
  }
}


