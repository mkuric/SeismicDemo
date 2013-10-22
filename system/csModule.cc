

#include "csModule.h"
#include "cseis_includes.h"

#include "csTraceGather.h"
#include "csTraceHeaderDef.h"
#include "csTrace.h"
#include "csTraceHeader.h"
#include "csUserParam.h"
#include "csParamDef.h"
#include "csMemoryPoolManager.h"
#include "csMethodRetriever.h"
#include "csExecPhaseDef.h"
#include "csInitExecEnv.h"

#include "geolib/csException.h"
#include "geolib/csVector.h"
#include "geolib/csQueue.h"
#include "geolib/csFlexNumber.h"
#include "geolib/csTimer.h"
#include "geolib/csTable.h"
#include <cstdlib>

using namespace cseis_system;

csModule::csModule( std::string const& name, int id_number, csMemoryPoolManager* memManager ) : myUniqueID(id_number) {
  myMemoryPoolManager = memManager;
  myHeaderDef = NULL;
  myName = name;
  init();
}
void csModule::init() {
  myExecPhaseDef = new csExecPhaseDef( myName );
  myTraceGather  = new csTraceGather( myMemoryPoolManager );
  myTraceQueue   = new cseis_geolib::csQueue<csTrace*>();

  myMethodParam = NULL;
  myMethodInit  = NULL;
  myMethodExecSingleTrace = NULL;
  myMethodExecMultiTrace  = NULL;

  myNumTracesToBePassed     = 0;
  myTotalNumProcessedTraces = 0;
  myTotalNumIncomingTraces  = 0;
  myExecEnvPtr              = NULL;
  mySuperHeader             = new csSuperHeader();

  myEnsembleKeyHeaderIndex = NULL;
  myEnsembleKeyValue       = NULL;
  myNextEnsembleKeyValue   = NULL;
  myHelperHdrValues = NULL;
  myIsEnsembleFull = false;

  myNumOutputPorts         = 1;
  myNumInputPorts          = 1;
  myTimeExecPhaseCPU       = 0.0;

  myIsFinishedProcessing = false;

  myVersion[MAJOR] = 1;
  myVersion[MINOR] = 0;

// Special modules
  if( !myName.compare("IF") ) {
    myModuleType = MODTYPE_IF;
    myNumOutputPorts = 2;
  }
  else if( !myName.compare("ELSEIF") ) {
    myModuleType = MODTYPE_ELSEIF;
    myNumOutputPorts = 2;
  }
  else if( !myName.compare("ELSE") ) {
    myModuleType = MODTYPE_ELSE;
  }
  else if( !myName.compare("ENDIF") ) {
    myModuleType = MODTYPE_ENDIF;
  }
  else if( !myName.compare("SPLIT") ) {
    myModuleType = MODTYPE_SPLIT;
    myNumOutputPorts = 2;
  }
  else if( !myName.compare("ENDSPLIT") ) {
    myModuleType = MODTYPE_ENDSPLIT;
  }
  else {
    myModuleType = MODTYPE_UNKNOWN;
  }
}
//---------------------------------------------------------------------
//
csModule::~csModule() {
  if( myExecPhaseDef != NULL ) {
    delete myExecPhaseDef;
    myExecPhaseDef = NULL;
  }
  if( myExecEnvPtr != NULL ) {
    delete myExecEnvPtr;
    myExecEnvPtr = NULL;
  }
  if( myHeaderDef != NULL ) {
    delete myHeaderDef;
    myHeaderDef = NULL;
  }
  if( myTraceGather != NULL ) {
    delete myTraceGather;
    myTraceGather = NULL;
  }
  if( myTraceQueue != NULL ) {
    delete myTraceQueue;
    myTraceQueue = NULL;
  }
  if( mySuperHeader ) {
    delete mySuperHeader;
    mySuperHeader = NULL;
  }
  if( myEnsembleKeyHeaderIndex ) {
    delete [] myEnsembleKeyHeaderIndex;
    myEnsembleKeyHeaderIndex = NULL;
  }
  if( myEnsembleKeyValue ) {
    delete [] myEnsembleKeyValue;
    myEnsembleKeyValue = NULL;
  }
  if( myNextEnsembleKeyValue ) {
    delete [] myNextEnsembleKeyValue;
    myNextEnsembleKeyValue = NULL;
  }
  if( myHelperHdrValues != NULL ) {
    delete [] myHelperHdrValues; myHelperHdrValues = NULL;
  }
}
//*********************************************************************
//
// Module submission methods
//
//*********************************************************************

void csModule::retrieveParamInitMethods() {
  csMethodRetriever::getParamInitMethod( myName, myVersion[MAJOR], myVersion[MINOR], myMethodParam, myMethodInit );
  if( myMethodParam == NULL ) {
    throw( cseis_geolib::csException("Program bug: Parameter definition method not found.") );
  }
  else if( myMethodInit == NULL ) {
    throw( cseis_geolib::csException("Program bug: Init phase method not found.") );
  }
}

void csModule::getParamDef( csParamDef* paramDef ) {
  if( myMethodParam == NULL ) {
    retrieveParamInitMethods();
  }
  (*myMethodParam)( paramDef );
}
//-------------------------------------------------------------------
void csModule::submitInitPhase( csParamManager* paramManager, csLogWriter* log, cseis_geolib::csTable const** tables, int numTables ) {
  if( myHeaderDef == NULL ) throw("csModule::submitInitPhase: Program bug: header definition object has not been initialized yet.");
  csInitPhaseEnv initEnv( myHeaderDef, myExecPhaseDef, mySuperHeader, tables, numTables );
  
  if( myMethodInit == NULL ) {
    retrieveParamInitMethods();
  }

  // Run init phase:
  (*myMethodInit)( paramManager, &initEnv, log );

  // Set exec phase method pointer
  if( myExecPhaseDef->execType() == EXEC_TYPE_SINGLETRACE || myExecPhaseDef->execType() == EXEC_TYPE_INPUT ) {
    myExecPhaseDef->traceMode = TRCMODE_FIXED;  // Make sure trace mode is set
    csMethodRetriever::getExecMethodSingleTrace( myName, myVersion[MAJOR], myVersion[MINOR], myMethodExecSingleTrace );
    if( myMethodExecSingleTrace == NULL ) {
      throw( cseis_geolib::csException("Program bug: Module exec phase method not found.") );
    }
  }
  else if( myExecPhaseDef->execType() == EXEC_TYPE_MULTITRACE ) {  // Multitrace module
    if( myExecPhaseDef->traceMode == TRCMODE_UNKNOWN ) {
      throw( cseis_geolib::csException("Program bug: Trace mode not set in module init method. Should be set to FIXED, ENSEMBLE...") );
    }

    csMethodRetriever::getExecMethodMultiTrace( myName, myVersion[MAJOR], myVersion[MINOR], myMethodExecMultiTrace );
    if( myMethodExecMultiTrace == NULL ) {
      throw( cseis_geolib::csException("Program bug: Module exec phase method not found.") );
    }
  }
  else {
    throw( cseis_geolib::csException("Program bug: Incorrect exec phase type specified.") );
  }
  
  if( initEnv.errorCount() > 0 ) {
    throw( cseis_geolib::csException("Error(s) occurred in init phase. See log file for details.") );
  }
  //--------------------------------------------------------------------------------
  // Consistency check of certain settings (basically checking that module init phase module is correct)
  //
  // 1) Check that all ensemble key headers exist. If not, remove this key from superheader, issue warning message
  int keyIndex = 0;
  while( keyIndex < mySuperHeader->numEnsembleKeys() ) {
    std::string const* name = mySuperHeader->ensembleKey( keyIndex );
    if( myHeaderDef->headerExists( name->c_str() ) ) {
      keyIndex += 1;
    }
    else {
      log->warning("Super header: Ensemble key header '%s' does not exist in trace header. Ensemble key has been removed from super header.", name->c_str());
      mySuperHeader->removeEnsembleKey( keyIndex );
    }
  }
  // Store trace header index of ensemble keys, for quick access
  int numKeys = mySuperHeader->numEnsembleKeys();
  if( numKeys > 0 ) {
    myEnsembleKeyHeaderIndex = new int [numKeys];
    myEnsembleKeyValue       = new cseis_geolib::csFlexNumber[numKeys];
    myNextEnsembleKeyValue   = new cseis_geolib::csFlexNumber[numKeys];
    for( int ikey = 0; ikey < numKeys; ikey++ ) {
      myEnsembleKeyHeaderIndex[ikey] = myHeaderDef->headerIndex( *mySuperHeader->ensembleKey(ikey) );
    }
    myHelperHdrValues = new cseis_geolib::csFlexNumber[numKeys];
  }
  myHeaderDef->resetByteLocation();
}
//-------------------------------------------------------------------
//
//
//
bool csModule::isReadyToSubmitExec( bool forceToRun ) const {
  if( myNumTracesToBePassed != 0 ) {
    throw( "csModule::isReadyToSubmitExec(): Program bug? Processed traces in trace gather have not been passed yet.." );
  }
  //---------------------------------------------------------------------------
  if( myExecPhaseDef->execType() == EXEC_TYPE_SINGLETRACE ) {  // Single trace module
    return( !myTraceGather->isEmpty() || !myTraceQueue->isEmpty() );
  }
  //---------------------------------------------------------------------------
  else if( myExecPhaseDef->execType() == EXEC_TYPE_MULTITRACE ) {  // Multitrace module
    int totalNumTraces = myTraceGather->numTraces()+myTraceQueue->size();
    if( myExecPhaseDef->traceMode == TRCMODE_FIXED ) {
      //printf("isReadyToSubmit '%s': gather ntraces %d, myExecPhase ntraces %d, queue ntraces: %d, forcetorun: %d\n", getName(), myTraceGather->numTraces(), myExecPhaseDef->numTraces, myTraceQueue->size(), forceToRun);
      if( totalNumTraces >= myExecPhaseDef->numTraces ) return true;
    }
    else { // if( myExecPhaseDef->traceMode == TRCMODE_ENSEMBLE ) {
//      printf("isReadyToSubmit ENSEMBLE '%s': gather ntraces %d, myExecPhase ntraces %d, queue ntraces: %d, force: %d %d\n", getName(), myTraceGather->numTraces(), myExecPhaseDef->numTraces,  myTraceQueue->size(), forceToRun, myExecPhaseDef->tracesAreWaiting() );
      if( mySuperHeader->numEnsembleKeys() > 0 && myIsEnsembleFull ) return true;
      // else: No ensemble key set --> process entire data set only at end, when forced
    }
    return( forceToRun && (totalNumTraces > 0 || myExecPhaseDef->tracesAreWaiting()) );
  }
  //---------------------------------------------------------------------------
  else if( myExecPhaseDef->execType() == EXEC_TYPE_INPUT ) {  // Input module
    return true;
  }
  else {
    throw( cseis_geolib::csException("csModule::isReadyToSubmitExec(): Program bug. Should never get here (B)...") );
  }
  return false;
}
//-------------------------------------------------------------------
//
//
bool csModule::submitExecPhase(  bool forceToProcess, csLogWriter* log, int& outPort ) {
  cseis_geolib::csTimer timer;
  timer.start();
  int nProcessedTraces = 0;
  myExecPhaseDef->myIsLastCall = forceToProcess;

  //  printf("submitExecPhase: Gather ntraces: %d  ---  Queue ntraces: %d \n", myTraceGather->numTraces(), myTraceQueue->size() );
  if( myExecPhaseDef->execType() == EXEC_TYPE_INPUT ) {
    // Input: Retrieve/allocate new trace, then submit exec phase to read in this one trace
    csTrace* trace = myMemoryPoolManager->getNewTrace( myHeaderDef, mySuperHeader->numSamples );
    myTraceGather->addTrace( trace );
    outPort = 0;  // Output port should be 0, because input module only has one output port...
    
    if( (*myMethodExecSingleTrace)( trace, &outPort, myExecEnvPtr, log ) ) {
      nProcessedTraces = 1;
    }
    else {   // No trace has been read in
      myTraceGather->freeAllTraces();
    }
    myIsFinishedProcessing = true;
  }
  //----------------------------------------------------------------------------------------
  else if( myExecPhaseDef->execType() == EXEC_TYPE_SINGLETRACE ) {
    //fprintf(stdout,"  Submit single trace module\n");
    if( myTraceGather->numTraces() <= 0 ) {
      if( !myTraceQueue->isEmpty() ) {
        myTraceGather->addTrace( myTraceQueue->pop() );
      }
      else {
        return false;
      }
    }
    myTotalNumIncomingTraces += 1;
    myExecPhaseDef->myIsLastCall = (forceToProcess && myTraceQueue->size() == 0);
    bool success = true;
    success = (*myMethodExecSingleTrace)( myTraceGather->trace(0), &outPort, myExecEnvPtr, log );
    // If valgrind reports error in the following line (Conditional jump...depends on uninitialised value), some single-trace module
    // has not properly returned a value. This check should really be done by compiler, but is not by default (use -Wall).
    if( success ) {
      nProcessedTraces = 1;
    }
    else {   // Trace shall be removed from flow
      myTraceGather->freeTrace( 0 );
    }
    myIsFinishedProcessing = (myTraceQueue->size() == 0);
    //fprintf(stdout,"  num trace/queue/finished: %d %d - %d\n", myTraceGather->numTraces(), myTraceQueue->size(), myIsFinishedProcessing );
  }
  //----------------------------------------------------------------------------------------
  else if( myExecPhaseDef->execType() == EXEC_TYPE_MULTITRACE ) {
    //fprintf(stdout,"  Submit multi trace module\n");
    if( myExecPhaseDef->traceMode == TRCMODE_FIXED ) {
      while( myTraceGather->numTraces() < myExecPhaseDef->numTraces && !myTraceQueue->isEmpty() ) {
        myTraceGather->addTrace( myTraceQueue->pop() );
      }
      myExecPhaseDef->myIsLastCall = (forceToProcess && myTraceQueue->size() == 0);
    }
    myTotalNumIncomingTraces += myTraceGather->numTraces();  // Accumulate number of traces input to this module

    int numTrcToKeep = 0;  // Number of traces to keep, i.e. traces to roll into next pass of this exec method
    myExecPhaseDef->myTracesAreWaiting = false;

    (*myMethodExecMultiTrace)( myTraceGather, &outPort, &numTrcToKeep, myExecEnvPtr, log );

    myIsEnsembleFull = false;
    myTotalNumIncomingTraces -= numTrcToKeep;  // Correct number of traces input to this module by traces that are kept

    if( numTrcToKeep > myTraceGather->numTraces() ) {
      log->warning("Module %s: Supposed number of traces to keep is larger than number of traces in gather. This is probably due to a program bug in the module method.", myName.c_str());
      numTrcToKeep = myTraceGather->numTraces();
    }
    else if( myExecPhaseDef->myIsLastCall && numTrcToKeep ) {
      // Do nothing. Allow modules to continue processing even if forced to run. This may lead to infinite loops if modules are not handled correctly
//      log->warning("Module %s: Number of traces to keep (%d) will be ignored, due to last call to this module. This is probably due to a program bug in the module method.", myName.c_str(), numTrcToKeep);
//      numTrcToKeep = 0;
    }

    if( myExecPhaseDef->traceMode == TRCMODE_FIXED ) {
      nProcessedTraces = myTraceGather->numTraces() - numTrcToKeep;
      myIsFinishedProcessing = true;
      int totalNumTracesLeft = numTrcToKeep + myTraceQueue->size();
      if( totalNumTracesLeft >= myExecPhaseDef->numTraces || (forceToProcess && totalNumTracesLeft != 0) ) {
        myIsFinishedProcessing = false;
      }
    }
    else { //if( myExecPhaseDef->traceMode == TRCMODE_ENSEMBLE ) {  // Case 2
      nProcessedTraces = myTraceGather->numTraces() - numTrcToKeep;
      if( myTraceGather->numTraces() == 0 ) { // Case 2.1
        updateTracesEnsembleModule();
      }
      else if( numTrcToKeep == 0 ) {  // Case 2.2
        updateTracesEnsembleModule();
      }
      else { // if( nProcessedTraces == 0 ) { // Case 2.3 & 2.4
        myIsEnsembleFull = true;
        myIsFinishedProcessing = false;
      }
      if( forceToProcess && (myTraceGather->numTraces()-nProcessedTraces + myTraceQueue->size()) != 0 ) {
        myIsEnsembleFull = true;
        myIsFinishedProcessing = false;
      }
    }
    //fprintf(stdout,"  num trace/queue/finished: %d %d - %d %d\n", myTraceGather->numTraces(), myTraceQueue->size(), myIsFinishedProcessing, myIsEnsembleFull );
  }
  else {
    throw( cseis_geolib::csException("csModule::submitExecPhase(): Program bug. Should never get here...") );
  }

// ******** DELETE HEADERS (if any) ON TRACES OUTPUT TO NEXT MODULE *********
//
  if( myHeaderDef->getIndexOfHeadersToDel()->size() > 0 ) {
    for( int itrc = 0; itrc < nProcessedTraces; itrc++ ) {
      myTraceGather->trace(itrc)->getTraceHeader()->deleteHeaders( myHeaderDef );
    }
  }
// ******** SET NUMBER OF SAMPLES FOR ALL TRACES MOVED TO FURTHER MODULES ********
  for( int itrc = 0; itrc < nProcessedTraces; itrc++ ) {
    myTraceGather->trace(itrc)->getTraceDataObject()->set( mySuperHeader->numSamples );
  }
// **************************************************************************
  if( outPort > myNumOutputPorts ) {
    throw( cseis_geolib::csException("csModule::submitExecPhase(): Output port number too large. This is most likely due to a program bug in the module method...") );
  }
  myNumTracesToBePassed     += nProcessedTraces; // Add processed traces to number of traces to be passed to next module
  myTotalNumProcessedTraces += nProcessedTraces; // Accumulate number of traces processed by this module
  myTimeExecPhaseCPU += timer.getElapsedTime();  // Accumulate exec phase CPU time
  return( nProcessedTraces > 0 );
}
//-------------------------------------------------------------------
//
//
bool csModule::submitCleanupPhase(  csLogWriter* log ) {
  myExecPhaseDef->myIsCleanup = true;
  int outPortDummy = 0;
  int nTracesDummy = 0;
  bool success = true;
  switch( myExecPhaseDef->execType() ) {
    case EXEC_TYPE_INPUT:
    case EXEC_TYPE_SINGLETRACE:
      if( myMethodExecSingleTrace ) {
        success = (*myMethodExecSingleTrace)( NULL, &outPortDummy, myExecEnvPtr, log );
      }
      break;
    case EXEC_TYPE_MULTITRACE:
      if( myMethodExecMultiTrace ) {
        (*myMethodExecMultiTrace)( NULL, &outPortDummy, &nTracesDummy, myExecEnvPtr, log );
      }
      break;
  }
  return success;
}
//*********************************************************************
//
// Trace management methods
//
//*********************************************************************

//------------------------------------------------------
void csModule::addNewTraceToGather( csTrace* trace, int inPort ) {
  trace->getTraceHeader()->setHeaders( myHeaderDef, inPort );
  trace->getTraceDataObject()->setMax( mySuperHeader->numSamples );
  myTraceGather->addTrace( trace );
}
void csModule::addNewTraceToQueue( csTrace* trace, int inPort ) {
  trace->getTraceHeader()->setHeaders( myHeaderDef, inPort );
  trace->getTraceDataObject()->setMax( mySuperHeader->numSamples );
  myTraceQueue->push( trace );
}
//------------------------------------------------------
//
bool csModule::traceIsPartOfCurrentEnsemble( csTrace* trace ) {
  csTraceHeader* trcHdrPtr = trace->getTraceHeader();
  int numKeys = mySuperHeader->numEnsembleKeys();
  for( int ikey = 0; ikey < numKeys; ikey++ ) {
    int hdrIndex = myEnsembleKeyHeaderIndex[ikey];
    char hdrType = myHeaderDef->headerType( hdrIndex );
    if( hdrType == cseis_geolib::TYPE_INT ) {
    //        printf("Current trace ensemble header value: %d  (current ensemble: %d), num traces currently held:%d\n",
     //        trcHdrPtr->intValue(hdrIndex), myEnsembleKeyValue[0].intValue(), myTraceGather->numTraces() );
      myHelperHdrValues[ikey].setIntValue( trcHdrPtr->intValue(hdrIndex) );
    }
    else if( hdrType == cseis_geolib::TYPE_FLOAT ) {
      myHelperHdrValues[ikey].setFloatValue( trcHdrPtr->floatValue(hdrIndex) );
    }
    else if( hdrType == cseis_geolib::TYPE_DOUBLE ) {
      myHelperHdrValues[ikey].setDoubleValue( trcHdrPtr->doubleValue(hdrIndex) );
    }
    else if( hdrType == cseis_geolib::TYPE_STRING ) {
      throw( cseis_geolib::csException("Encountered ensemble key of type string. This is currently not supported.") );
    }
  }

  if( myTraceGather->numTraces() > 0 ) {  // Current module already has traces in gather -> Check if ensemble header values agree
    for( int ikey = 0; ikey < numKeys; ikey++ ) {
      if( myEnsembleKeyValue[ikey] != myHelperHdrValues[ikey] ) {
        for( int ikeyNew = 0; ikeyNew < numKeys; ikeyNew++ ) {
          myNextEnsembleKeyValue[ikeyNew] = myHelperHdrValues[ikeyNew];
        }
        myIsEnsembleFull = true;
        return false;
      }
    }
  }
  else {  // Current module does not have any traces yet in gather -> set ensemble keys
    for( int ikey = 0; ikey < numKeys; ikey++ ) {
      myEnsembleKeyValue[ikey] = myHelperHdrValues[ikey];
    }
  }
  return true;
}

//------------------------------------------------------------------
//
void csModule::updateTracesEnsembleModule() {
  for( int i = 0; i < mySuperHeader->numEnsembleKeys(); i++ ) {
    myEnsembleKeyValue[i] = myNextEnsembleKeyValue[i];
  }
  myIsEnsembleFull = false;
  myIsFinishedProcessing = true;
  if( !myTraceQueue->isEmpty() ) {
    // Move first trace from queue to gather, this doesn't need check
    myTraceGather->addTrace( myTraceQueue->pop() );
    while( !myTraceQueue->isEmpty() ) {
      csTrace* trace = myTraceQueue->peek();
      if( traceIsPartOfCurrentEnsemble( trace ) ) {
        myTraceGather->addTrace( myTraceQueue->pop() );
      }
      else {  // Trace is NOT part of current ensemble
        myIsEnsembleFull = true;
        myIsFinishedProcessing = false;
        break;
      }
    }  // end while
  }
}
//------------------------------------------------------
// Move traces. Start with first trace (trace index 0)
void csModule::moveTracesFrom( csModule* module, int inPort ) {
  //  printf("Move traces from '%s' (ntraces= %d, queue= %d) to '%s' (ntraces= %d, queue= %d), ntr to be passed= %d\n",
  //        module->myName.c_str(), module->myTraceGather->numTraces(), module->myTraceQueue->size(),
  //       myName.c_str(), myTraceGather->numTraces(), myTraceQueue->size(), module->myNumTracesToBePassed );
  if( module->myNumTracesToBePassed == 0 ) {  // Does this ever occur?
    throw( cseis_geolib::csException("csModule::moveTracesFrom: No traces passed to be moved to next module. Is that good or bad?") );
  }

  // Case A) Single trace or fixed trace module
  if( myExecPhaseDef->execType() == EXEC_TYPE_SINGLETRACE || myExecPhaseDef->execType() == EXEC_TYPE_INPUT ||
      myExecPhaseDef->traceMode == TRCMODE_FIXED ) {
    int firstTraceIndex = 0;
    int numFixedTraces = myExecPhaseDef->numTraces;
    // A1. Move as many traces as possible from the other 'module' to the 'trace gather'.
    while( myTraceGather->numTraces() < numFixedTraces && firstTraceIndex < module->myNumTracesToBePassed ) {
      addNewTraceToGather( module->myTraceGather->trace( firstTraceIndex++ ), inPort );
      //   printf("A1. Old gather: %d, new gather: %d   --- %d (%d)\n", module->myTraceGather->numTraces(), myTraceGather->numTraces(), firstTraceIndex, module->myNumTracesToBePassed );
    }
    // A2. Move remaining traces to the 'trace queue'
    for( int itrc = firstTraceIndex; itrc < module->myNumTracesToBePassed; itrc++ ) {
      addNewTraceToQueue( module->myTraceGather->trace( itrc ), inPort );
      //       printf("A2. Old gather: %d, new queue: %d   ---  %d\n", module->myTraceGather->numTraces(), myTraceQueue->size(), firstTraceIndex);
    }
  }
  // Case B) Ensemble trace module or module with variable number of traces
  else { // else if( myExecPhaseDef->traceMode == TRCMODE_ENSEMBLE ) {
    // First attempt: - Move all traces to the 'trace queue'.
    // - Next, try to move as many traces as possible from trace queue to trace gather

    // B1. The main case for multi-trace ensemble modules:
    // Check whether each trace is part of the current 'trace gather'.
    // If yes, add trace to trace gather. Otherwise, move this and all remaining traces into trace queue.
    if( mySuperHeader->numEnsembleKeys() > 0 ) {
      int firstTraceIndex = 0;
      // 1. If next ensemble has not been found yet, keep moving traces to trace gather
      while( firstTraceIndex < module->myNumTracesToBePassed ) {
        csTrace* trace = module->myTraceGather->trace( firstTraceIndex );
        //printf("moveTracesFrom: Trace is FULL: %s\n", myIsEnsembleFull ? "yes" : "no" );
        if( myIsEnsembleFull || !traceIsPartOfCurrentEnsemble( trace ) ) {
          //printf("moveTracesFrom: Trace is FULL or NOT part of current ensemble.  %s\n",  myName.c_str() );
          addNewTraceToQueue( trace, inPort );
        }
        else { //if( traceIsPartOfCurrentEnsemble( trace ) ) {
          //printf("moveTracesFrom: Trace is part of current ensemble.  %s\n", myName.c_str());
          addNewTraceToGather( trace, inPort );
        }
        firstTraceIndex += 1;
      }  // end while
    }
    // B2. No ensemble key set --> buffer entire data set directly into the trace gather
    else { //if( mySuperHeader->numEnsembleKeys() == 0 ) {
      for( int itrc = 0; itrc < module->myNumTracesToBePassed; itrc++ ) {
        addNewTraceToGather( module->myTraceGather->trace( itrc ), inPort );
      }
    }
  }
  // Remove traces from trace gather, but DO NOT FREE TRACES. Traces are freed when last module is reached.
  // Traces can not be freed yet because they are still in use by the remaining modules.
  module->myTraceGather->deleteTraces( 0, module->myNumTracesToBePassed );
  module->myNumTracesToBePassed = 0;
}

//------------------------------------------------------
//
void csModule::lastModuleTraceCleanup() {
  // Free processed traces from trace gather
  myTraceGather->freeTraces( 0, myNumTracesToBePassed );
  myNumTracesToBePassed = 0;
}
//-------------------------------------------------------------------
//
//

bool csModule::finishedProcessing() const {
//  printf("finishedProcessing '%s': gather ntraces %d, numTracesPassed: %d, queue ntraces: %d, finished: %d\n", getName(), myTraceGather->numTraces(), myNumTracesToBePassed,  myTraceQueue->size(), (myTraceGather->numTraces() == myNumTracesToBePassed && myTraceQueue->isEmpty()) );
  return( myIsFinishedProcessing );
}
int csModule::getExecType() const {
  return myExecPhaseDef->execType();
}
csTraceHeaderDef const* csModule::getHeaderDef() const {
  return myHeaderDef;
}
csExecPhaseDef const* csModule::getExecPhaseDef() const {
  return myExecPhaseDef;
}

//*********************************************************************
//
// Input port methods
//
//*********************************************************************
//
void csModule::setInputPorts( cseis_geolib::csVector<csModule const*> const* moduleList ) {
  myNumInputPorts = moduleList->size();

  if( myHeaderDef != NULL ) {
    delete myHeaderDef;
  }
  csTraceHeaderDef const** hdefPrev = new csTraceHeaderDef const*[myNumInputPorts];
  for( int inPort = 0; inPort < myNumInputPorts; inPort++ ) {
    hdefPrev[inPort] = moduleList->at(inPort)->myHeaderDef;
    if( inPort == 0 ) {
      mySuperHeader->set( moduleList->at(inPort)->mySuperHeader );
    }
    else {
      csSuperHeader* shdr = moduleList->at(inPort)->mySuperHeader;
      if( mySuperHeader->sampleInt != shdr->sampleInt ) {
        throw( cseis_geolib::csException("Module #%d (%s): Data from different input ports to this module have unequal sample interval", myUniqueID+1, getName()) );
      }
      else if( mySuperHeader->numSamples != shdr->numSamples ) {
        throw( cseis_geolib::csException("Module #%d (%s): Data from different input ports to this module have unequal number of samples", myUniqueID+1, getName()) );
      }
    }
  }
  myHeaderDef = new csTraceHeaderDef( myNumInputPorts, hdefPrev, myMemoryPoolManager );
  delete [] hdefPrev;

  if( myExecEnvPtr != NULL ) {
    delete myExecEnvPtr;
  }
  myExecEnvPtr = new csExecPhaseEnv( myHeaderDef, myExecPhaseDef, mySuperHeader );
}
//------------------------------------------------------------
//
void csModule::setDebugFlag( bool doDebug ) {
  myExecPhaseDef->myIsDebug = doDebug;
}
int csModule::tempNumTraces() { return myTraceGather->numTraces(); }  // debug only, remove!

//--------------------------------------------------------------
std::string csModule::versionString() const {
  char ver[40];
  sprintf( ver, "%d.%d%c", myVersion[MAJOR], myVersion[MINOR], '\0' );
  return( std::string(ver) );
}


