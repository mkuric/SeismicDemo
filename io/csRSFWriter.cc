
#include "csRSFWriter.h"
#include "csRSFHeader.h"
#include "geolib/csException.h"
#include "geolib/csVector.h"
#include "geolib/geolib_endian.h"
#include "geolib/csByteConversions.h"
#include <string>
#include <cstring>

using namespace cseis_io;
using namespace cseis_geolib;

csRSFWriter::csRSFWriter( std::string filename, int nTracesBuffer, bool reverseByteOrder, bool swapDim3, bool outputGrid,
			  double tolerance) :
  NTRACES_BUFFER(nTracesBuffer), TOLERANCE(tolerance)
{
  myBigBuffer    = NULL;
  mySwapDim3 = swapDim3;
  myOutputGrid = outputGrid;
  myHdr       = NULL;
  myStage = csRSFWriter::STAGE_1_INIT;

  myFilename     = filename;
  myIsAtEOF      = false;
  myDoSwapEndian = reverseByteOrder;
}
//-----------------------------------------------------------------------------------------
csRSFWriter::~csRSFWriter() {
  closeFile();
  try {
    if( myStage == csRSFWriter::STAGE_6_COMPLETE ) {
      finalize();
    }
  }
  catch(...) {
    // nothing
  }
  if( myBigBuffer ) {
    delete [] myBigBuffer;
    myBigBuffer = NULL;
  }
  if( myFile ) {
    myFile->close();
    delete myFile;
    myFile = NULL;
  }
  if( myHdr ) {
    delete myHdr;
    myHdr = NULL;
  }
}
//-----------------------------------------------------------------------------------------
void csRSFWriter::initialize( csRSFHeader const* hdr ) {
  myHdr = new csRSFHeader();
  myHdr->set( *hdr );

  myNumSamples = myHdr->n1;
  mySampleInt  = myHdr->d1;

  /*
  if( mySwapDim3 ) {
    myHdr->n1 = hdr->n3;
    myHdr->o1 = hdr->o3;
    myHdr->d1 = hdr->d3;
    myHdr->e1 = hdr->e3;
    myHdr->n3 = hdr->n1;
    myHdr->o3 = hdr->o1;
    myHdr->d3 = hdr->d1;
    myHdr->e3 = hdr->e1;
  }
  */

  mySampleByteSize = 4;   // assume 4 byte floating point
  myTotalTraceSize = myNumSamples*mySampleByteSize;

  myBigBuffer = new char[ NTRACES_BUFFER * myTotalTraceSize ];
  memset( myBigBuffer, 0, NTRACES_BUFFER * myTotalTraceSize );

  if( !myBigBuffer ) {
    throw( csException("Not enough memory...") );
  }

  myTraceCounter   = 0;
  myCurrentTrace   = 0;
  myNumSavedTraces = 0;

  openFile();

  myStage = csRSFWriter::STAGE_2_SET_ORIG;
  //  myHasBeenInitialized = true;
}
//-----------------------------------------------------------------------------------------
void csRSFWriter::finalize() {
  myHdr->e2 = myHdr->o2 + myHdr->d2 * (myHdr->n2-1);
  myHdr->e3 = myHdr->o3 + myHdr->d3 * (myHdr->n3-1);

  writeRSFHdr();
  myStage = csRSFWriter::STAGE_6_COMPLETE;
}
//-----------------------------------------------------------------------------------------
void csRSFWriter::openFile() {
  myFile = new std::ofstream();
  myFile->open( myHdr->filename_bin_full_path.c_str(), std::ios::out | std::ios::binary );
  if( myFile->fail() ) {
    throw csException("Cannot open RSF binary file for writing: %s", myHdr->filename_bin_full_path.c_str());
  }
  if( mySwapDim3 ) {
    // Create output file of right size
    int numTraces = myHdr->n1 * myHdr->n2;
    long long size = (long long)numTraces * (long long)mySampleByteSize * (long long)myHdr->n3;
    fprintf(stderr," SIZE  %d  %lld   %d   %d\n", numTraces, size, mySampleByteSize, myHdr->n3);
    for( int itrc = 0; itrc < numTraces; itrc += 1 ) {
      myFile->write( myBigBuffer, myHdr->n3 * mySampleByteSize );
    }
  }
}
void csRSFWriter::closeFile() {
  if( myFile != NULL ) {
    if( myNumSavedTraces > 0 ) {
      myFile->write( myBigBuffer, myTotalTraceSize*myNumSavedTraces );
    }
    writeNextTrace( NULL, 0, 0, 0 );
    myFile->close();
    delete myFile;
    myFile = NULL;
  }
}
//--------------------------------------------------------------------------------
//
bool csRSFWriter::check( double valDim2, double valDim3 ) {
  if( myStage <= csRSFWriter::STAGE_4_DIM3_STEP ) {
    if( myStage <= csRSFWriter::STAGE_3_DIM2_STEP ) {

      // Stage 2: Set dimension 2 and 3 origin
      if( myStage == csRSFWriter::STAGE_2_SET_ORIG ) {
	myHdr->o2 = valDim2;
	myHdr->o3 = valDim3;
	myHdr->n2 = 1;
	myHdr->n3 = 1;
	myStage = csRSFWriter::STAGE_3_DIM2_STEP;
      }
      // Stage 3: Set dimension 2 step
      else if( myStage == csRSFWriter::STAGE_3_DIM2_STEP ) {
	myHdr->d2 = valDim2 - myHdr->o2;
	myHdr->n2 = 2;
	myStage = csRSFWriter::STAGE_4_DIM3_STEP;
      }
      else { 
	throw( csException("csRSFWriter::check(): Wrong 'stage' number: %d. This is a program bug in this class", myStage) );
      }
    }
    // Stage 4: Set dimension 3 step
    else {
      myHdr->d3 = valDim3 - myHdr->o3;
      if( fabs(myHdr->d3) > TOLERANCE ) {
	myHdr->n3 = 2;
	myCurrentCounterDim2 = 1;
	myStage = csRSFWriter::STAGE_5_WRITE_RSF;
      }
      else {
	if( fabs( (valDim2-myHdr->o2) - (myHdr->n2*myHdr->d2) ) > TOLERANCE ) {
	  throw( csException("Unexpected value in sequential trace #%d for dimension2: %f. Expected value: %f. Incorrect sorting..?",
			     myTraceCounter, valDim2, myHdr->o2 + myHdr->n2*myHdr->d2 ) );
	}
	myHdr->n2 += 1;
      }
    }
  } // END if myStage <= STAGE_4
  // Beyond stage 4: Check that all traces are sorted correctly
  else {
    myCurrentCounterDim2 += 1;
    // Within a 'row' --> Check dim2 step & dim3 value
    if( myCurrentCounterDim2 <= myHdr->n2 ) {
      if( fabs( (valDim2-myPreviousValueDim2) - myHdr->d2 ) > TOLERANCE ) {
	throw( csException("Unexpected value in sequential trace #%d for dimension2: %f. Expected value: %f. Incorrect sorting..?",
			   myTraceCounter, valDim2, myPreviousValueDim2+myHdr->d2) );
      }
      else if( fabs( valDim3-myPreviousValueDim3 ) > TOLERANCE ) {
	throw( csException("Unexpected value in sequential trace #%d for dimension3: %f. Expected value: %f. Incorrect sorting..?",
			   myTraceCounter, valDim3, myPreviousValueDim3) );
      }
    }
    // New 'row' just starting --> Check dim2 origin, dim2 amount & dim3 step
    else {
      if( fabs( valDim2-myHdr->o2 ) > TOLERANCE ) {
	throw( csException("Unexpected value in sequential trace #%d for dimension2: %f. Expected value (=origin): %f. Incorrect sorting..?",
			   myTraceCounter, valDim2, myHdr->o2) );
      }
      else if( fabs( (valDim3-myPreviousValueDim3) - myHdr->d3 ) > TOLERANCE ) {
	throw( csException("Unexpected value in sequential trace #%d for dimension3: %f. Expected value: %f. Incorrect sorting..?",
			   myTraceCounter, valDim3, myPreviousValueDim3+myHdr->d3) );
      }
      myHdr->n3 += 1;
      myCurrentCounterDim2 = 1;
    }
  }

  myPreviousValueDim2 = valDim2;
  myPreviousValueDim3 = valDim3;
  return true;
}
//*******************************************************************
//
// Write RSF header file
//
//*******************************************************************
void csRSFWriter::writeRSFHdr() {
  FILE* fhdr = fopen(myFilename.c_str(),"w");
  if( fhdr == NULL ) {
    throw csException("Cannot open rsf header file for writing: %s", myFilename.c_str());
  }
  myHdr->dump(fhdr,myOutputGrid);
  fclose(fhdr);
}

//*******************************************************************
//
// The big method, writing one trace...
//
//*******************************************************************
//
void csRSFWriter::writeNextTrace( byte_t const* theBuffer, int nSamples, double valDim2, double valDim3 ) {
  
  // TEMP START
  if( mySwapDim3 ) {
    memcpy( myBigBuffer, theBuffer, nSamples*mySampleByteSize );
    if( myDoSwapEndian ) {
      swapEndian4( myBigBuffer, nSamples*mySampleByteSize );
    } // END doSwapEndian
    fprintf(stderr,"Save trace %d\n", myCurrentTrace);
    myFile->seekp( myCurrentTrace*myTotalTraceSize, ios_base::beg ); // Jump to start of trace X
    if( myFile->fail() ) {
      throw( csException("Unexpected error occurred when writing to RSF file: Cannot jump to trace %d", myCurrentTrace) );
    }
    int stepBytes = myTotalTraceSize;
    for( int isamp = 0; isamp < nSamples; isamp += 1 ) {
      myFile->write( &myBigBuffer[isamp*mySampleByteSize], mySampleByteSize );
      if( myFile->fail() ) {
	throw( csException("Unexpected error occurred when writing to RSF file: Cannot write to position %d %d", myCurrentTrace, isamp) );
      }
      myFile->seekp( stepBytes, ios::cur );
      if( myFile->fail() ) {
	throw( csException("Unexpected error occurred when writing to RSF file: Cannot jump to position %d %d", myCurrentTrace, isamp) );
      }
    }
    myCurrentTrace += 1;
    return;
  }
  if( myStage == csRSFWriter::STAGE_1_INIT ) {
    throw( csException("Accessing method to read first trace before initializing RSF Writer. This is a program bug in the calling method") );
  }
  if( nSamples == 0 || nSamples > myNumSamples ) nSamples = myNumSamples;

  if( myCurrentTrace == NTRACES_BUFFER ) {
    if( myDoSwapEndian ) {
      for( int itrc = 0; itrc < myNumSavedTraces; itrc++ ) {
        swapEndian4( myBigBuffer+myTotalTraceSize*itrc, nSamples*mySampleByteSize );
      }
    } // END doSwapEndian

    if( myNumSavedTraces > 0 ) {
      myFile->write( myBigBuffer, myTotalTraceSize*myNumSavedTraces );
      //  int sizeWrite = fwrite( myBigBuffer, myTotalTraceSize, myNumSavedTraces, myFile );
      if( myFile->fail() ) {
        myFile->close();
        delete myFile;
	myFile = NULL;
        throw( csException("Unexpected error occurred when writing to RSF file") );
      }
    }
    myNumSavedTraces = 0;
    myCurrentTrace   = 0;
  }

  // buffer will be NULL when last traces shall be written out... Just return.
  if( theBuffer == NULL ) {
    return;
  }
  // Set input buffers

  int indexCurrentTrace = myCurrentTrace*myTotalTraceSize;
  memcpy( &myBigBuffer[indexCurrentTrace], theBuffer, nSamples*mySampleByteSize );

  myTraceCounter++;
  myCurrentTrace++;
  myNumSavedTraces++;

  check( valDim2, valDim3 );
}


