/* Copyright (c) Colorado School of Mines, 2013.*/
/* All rights reserved.                       */



#ifndef CS_SORTED_VECTOR_H
#define CS_SORTED_VECTOR_H

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include "csVector.h"

namespace cseis_geolib {

/**
* Vector implementation of general collection
* Vector where elements are sorted by '<' and '>' operators
*
* @author Bjorn Olofsson
*/
template <typename T>
class csSortedVector : public csVector<T> {
public:
  csSortedVector( int initial_capacity );
  csSortedVector();
  csSortedVector( csSortedVector<T> const& obj );
  virtual ~csSortedVector();
  virtual inline int insert( T const& value );
  virtual inline int getPosition( T const& value ) const;
};

//-------------------------------------------------
template<typename T>osSortedVector<T>::csSortedVector() : 
  csVector<T>() {
}
//-------------------------------------------------
template<typename T>osSortedVector<T>::csSortedVector( int initialCapacity ) :
  csVector<T>(initialCapacity) {
}
//-------------------------------------------------
template<typename T>osSortedVector<T>::csSortedVector( csSortedVector<T> const& obj ) : 
  csVector<T>( obj ) {
}
//-------------------------------------------------
template<typename T>osSortedVector<T>::~csSortedVector() { 
}
//-------------------------------------------------
template<typename T> inline int csSortedVector<T>::insert( T const& value ) {
  fprintf(stderr,"Insert...\n");
  int atIndex = getPosition( value );
  csVector<T>::insert( value, atIndex );
  return atIndex;
}
//-------------------------------------------------
template<typename T> inline int csSortedVector<T>::getPosition( T const& value ) const {
  int indexLeft  = 0;
  int indexRight = csCollection<T>::mySize - 1;

  if( csCollection<T>::myArray[indexLeft] > value ) {
    return -1;
  }
  else if( csCollection<T>::myArray[indexRight] < value ) {
    return csCollection<T>::mySize;
  }
  else if( csCollection<T>::myArray[indexRight] == value ) {
    return indexRight;
  }

  while( (indexRight-indexLeft) > 1 ) {
    int atIndex = (indexLeft+indexRight)/2;
    if( csCollection<T>::myArray[atIndex] < value ) {
      indexLeft = atIndex;
    }
    else if( csCollection<T>::myArray[atIndex] > value ) {
      indexRight = atIndex;
    }
    else { //if( csCollection<T>::myArray[atIndex] == value )
      return atIndex;
    }
  }
  return -1;
}

} // END namespace

#endif



