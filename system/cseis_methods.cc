

#include <cstdio>
#include <string>
#include <cstring>
#include <stdarg.h>

#include "cseis_defines.h"
#include "geolib/geolib_string_utils.h"
#include "csUserConstant.h"
#include "geolib/csCompareVector.h"
#include "geolib/csKey.h"
#include "geolib/csVector.h"
#include "geolib/csException.h"


/// !CHANGE! these methods, maybe put in different file or class...

namespace cseis_system {


void exitOnError( char const* text, ... ) {
  va_list argList;
  va_start( argList, text );
  vfprintf( stderr, text, argList );
  fprintf( stderr, "\n" );
  exit(-1);
}

  std::string replaceUserConstants( char const* line, cseis_geolib::csVector<cseis_system::csUserConstant> const* list ) {
  int length = strlen(line);
  std::string strNew = line;
  int nFound = 0;
  int counter = 0;
  while( counter < length ) {
    if( line[counter] == csUserConstant::LETTER_DEFINE ) {
      int smallLength = strlen(line+counter);
      csUserConstant const* defPtr;
      for( int i = 0; i < list->size(); i++ ) {
        defPtr = &(list->at(i));
        if( smallLength >= defPtr->nameLength && !defPtr->name.compare(0,defPtr->nameLength,line+counter,0,defPtr->nameLength) ) {
          counter += defPtr->nameLength-1;
          strNew = cseis_geolib::replaceStr( strNew, defPtr->name, defPtr->value );
          nFound++;
          break;
        }
      }
    }
    counter++;
  }
  return strNew;
}

/**
* Create individual flow from master flow
*
* @return Number of modules
*/
int cseis_create_flow( FILE* f_master_flow, FILE* f_flow, cseis_geolib::csVector<csUserConstant> const* masterConstants ) {
  int counterModules = 0;
  int counterLines   = 0;
  char line[MAX_LINE_LENGTH];
  cseis_geolib::csVector<std::string> tokenList;

  while( fgets( line, MAX_LINE_LENGTH, f_master_flow ) != NULL ) {
    counterLines++;
    char c;
    if( cseis_geolib::firstNonBlankChar( line, c ) ) {
      std::string tmpStr = replaceUserConstants( line, masterConstants );
      fprintf(f_flow,"%s",tmpStr.c_str());
    }
    else {
      fprintf(f_flow,"%s",line);
    }
  }
  return counterModules;
}
//----------------------------------------------------------------------------------
/**
* Read spreadsheet file that contains user constant table (sort of survey database)
*
*/
void cseis_read_spreadSheet( char const* filenameSheet, FILE* f_spreadSheet,
                 cseis_geolib::csCompareVector<csUserConstant>* masterConstants, 
                 cseis_geolib::csVector< cseis_geolib::csVector<std::string> >* spreadSheetConstantList ) {  
    
  cseis_geolib::csVector<std::string> tokenList;
  int counterLines = 0;
  int nConstants = 0;
  char line[MAX_LINE_LENGTH];
  // Read in first line. Expected to find list of constant names in first line
  // Constant names must start with a letter
  // First column must contain unique key
  if( fgets( line, MAX_LINE_LENGTH, f_spreadSheet ) != NULL ) {
    counterLines++;
    tokenize( line, tokenList );
    nConstants = tokenList.size();
  }
  if( nConstants == 0 ) {
    throw( cseis_geolib::csException("Error in spread sheet. Empty first line, expected constant names.") );
  }
  // Tokenize constant name list
  for( int i = 0; i < tokenList.size(); i++ ) {
    csUserConstant def( tokenList.at(i), "" );
    if( def.nameLength == 2 ) {
      exitOnError("Error in spread sheet '%s'. Empty first line, expected constant names.\n", filenameSheet );
    }
    else if( !cseis_geolib::isLetter(def.name[1]) ) {
      exitOnError("Error in spread sheet '%s', line %d. Constant names must start with a letter: '%s'\n",
        filenameSheet, counterLines, def.pureName().c_str() );
    }
    if( masterConstants->contains(def) ) {
      exitOnError("Error in spread sheet '%s', line %d. Duplicate constant name: '%s'\n",
        filenameSheet, counterLines, def.pureName().c_str() );
    }
    masterConstants->insertEnd( def );
  } // END: for loop, tokenize all constant names
  cseis_geolib::csCompareVector<cseis_geolib::csKey> keyList;
  while( fgets( line, MAX_LINE_LENGTH, f_spreadSheet ) != NULL ) {
    counterLines++;
    tokenList.clear();
    tokenize( line, tokenList );
    if( tokenList.size() != masterConstants->size() ) {
      exitOnError("Error in spread sheet '%s', line %d: %s Inconsistent number of columns. Expected: %d, found: %d.\n",
        filenameSheet, counterLines, line, masterConstants->size(), tokenList.size() );
    }
    if( keyList.contains( cseis_geolib::csKey(tokenList.at(0)) ) ) {
      exitOnError("Error in spread sheet '%s', line %d: %s Duplicate key found. First column must contain a unique key.\n",
        filenameSheet, counterLines, line );
    }
    keyList.insertEnd( cseis_geolib::csKey(tokenList.at(0)) );
    spreadSheetConstantList->insertEnd( tokenList );
  }
  if( counterLines < 2 ) {
    exitOnError("Error in spread sheet '%s'. File does not contain any values.\n", filenameSheet );
  }
/*
  for( int i = 0; i < masterConstants->size(); i++ ) {
    fprintf(stderr,"Master constant %d %s...\n", i, masterConstants->at(i).pureName().c_str() );
    for( int k = 0; k < spreadSheetConstantList->size(); k++ ) {
      fprintf(stderr,"Master constant value %d %s...\n", k, spreadSheetConstantList->at(k).at(i).c_str() );
    }
  }
*/
}
//---------------------------------------------------------------
/**
*
*
*
*/
void cseis_read_globalConst( FILE* f_globalConst, cseis_geolib::csCompareVector<csUserConstant>* globalConstList ) {
  cseis_geolib::csVector<std::string> tokenList(3);
  int counterLines = 0;
  char line[MAX_LINE_LENGTH];
  while( fgets( line, MAX_LINE_LENGTH, f_globalConst ) != NULL ) {
    counterLines++;
    char c;
    if( cseis_geolib::firstNonBlankChar( line, c ) && c != LETTER_COMMENT ) {
      if( c == csUserConstant::LETTER_DEFINE ) {
        tokenList.clear();
        tokenize( line, tokenList );
        if( !tokenList.at(0).compare(csUserConstant::defineWord()) ) {
          if( tokenList.size() != 3 ) {
            exitOnError("Syntax error in global constant input file, line %d: %s &define statement requires two arguments: &define <name> <value>.",
                  counterLines, line );
          }
          csUserConstant uc(tokenList.at(1), tokenList.at(2));
          if( globalConstList->contains( uc ) ) {
            exitOnError("Syntax error in global constant input file, line %d: %s Duplicate definition of global constant '%s'.",
                  counterLines, line, uc.pureName().c_str() );
          }
          globalConstList->insertEnd( uc );
        }
      }
    }
  }
  
}

} // namespace


