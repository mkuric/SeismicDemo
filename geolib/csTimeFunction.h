/* Copyright (c) Colorado School of Mines, 2013.*/
/* All rights reserved.                       */



#ifndef CS_TIME_FUNCTION_H
#define CS_TIME_FUNCTION_H

#include "csException.h"
#include "csVector.h"

namespace cseis_geolib {

/**
 * Time function, used in Table
 *
 * @author Bjorn Olofsson
 * @date 2008
 */
template <typename T>
class csTimeFunction {
public:
  csTimeFunction();
  ~csTimeFunction();
  void set( csTimeFunction<T> const* timeFunc );
  void set( T const* values, double const* times, int numValues );
  void set( csVector<T> const* valueList, csVector<double> const* timeList );
  inline void getKneePoint( int kneePointIndex, double& time, T& value ) const;
  inline T valueAt( double time ) const;
  inline T valueAtIndex( int index ) const;
  inline double timeAtIndex( int index ) const;
  inline void getContinuousFunction( double time ) const;
  inline int numValues() const;

private:
  void resetBuffer( int numValues );
  float mySampleInt;
  T* myValueBuffer;
  double* myTimeBuffer;
  int myNumValues;
};

template<typename T>csTimeFunction<T>::csTimeFunction() {
  myValueBuffer = NULL;
  myTimeBuffer  = NULL;
  myNumValues   = 0;
}
template<typename T>csTimeFunction<T>::~csTimeFunction() {
  if( myValueBuffer != NULL ) {
    delete [] myValueBuffer;
    myValueBuffer = NULL;
  }
  if( myTimeBuffer != NULL ) {
    delete [] myTimeBuffer;
    myTimeBuffer = NULL;
  }
}

template<typename T> inline double csTimeFunction<T>::timeAtIndex( int index ) const {
  return myTimeBuffer[index];
}

template<typename T> void csTimeFunction<T>::set( csTimeFunction<T> const* timeFunc ) {
  set( timeFunc->myValueBuffer, timeFunc->myTimeBuffer, timeFunc->myNumValues );
}

template<typename T> void csTimeFunction<T>::set( csVector<T> const* valueList, csVector<double> const* timeList ) {
  int numValues = valueList->size();
  resetBuffer( numValues );
  for( int i = 0; i < myNumValues; i++ ) {
    myValueBuffer[i] = valueList->at(i);
  }
  for( int i = 0; i < myNumValues; i++ ) {
    myTimeBuffer[i] = timeList->at(i);
  }
}
template<typename T> void csTimeFunction<T>::set( T const* values, double const* times, int numValues ) {
  resetBuffer( numValues );
  for( int i = 0; i < myNumValues; i++ ) {
    myValueBuffer[i] = values[i];
  }
  for( int i = 0; i < myNumValues; i++ ) {
    myTimeBuffer[i] = times[i];
  }
}
template<typename T> inline int csTimeFunction<T>::numValues() const {
  return myNumValues;
}
template<typename T> inline void csTimeFunction<T>::getKneePoint( int kneePointIndex, double& time, T& value ) const {
  if( kneePointIndex >= 0 && kneePointIndex < myNumValues ) {
    value = myValueBuffer[kneePointIndex];
    time  = myTimeBuffer[kneePointIndex];
  }
  else {
    throw( csException("Wrong index passed to csTimeFunction object, method value(int)..") );
  }
}
template<typename T> inline T csTimeFunction<T>::valueAtIndex( int index ) const {
  return myValueBuffer[index];
}
template<typename T> inline T csTimeFunction<T>::valueAt( double time ) const {
  if( myNumValues == 0 ) {
    throw( csException("Called csTimeFunction 'value' functon, but object is not set yet") );
  }
  else if( time <= myTimeBuffer[0] ) {
    return myValueBuffer[0];
  }
  else if( time >= myTimeBuffer[myNumValues-1] ) {
    return myValueBuffer[myNumValues-1];
  }
  // Linear interpolation
  else {
    int topIndex = -1;
    // Temp, slow 'find' method for correct index... !CHANGE!
    for( int i = 1; i < myNumValues; i++ ) {
      if( myTimeBuffer[i] >= time ) {
        topIndex = i-1;
        break;
      }
    }
    int bottomIndex = topIndex+1;
    double weight = (time-myTimeBuffer[topIndex]) / (myTimeBuffer[bottomIndex]-myTimeBuffer[topIndex]);
    return( (T)( (double)myValueBuffer[topIndex] + weight*( myValueBuffer[bottomIndex] - myValueBuffer[topIndex] ) ) );
  }
}

template<typename T> void csTimeFunction<T>::resetBuffer( int numValues ) {
  if( numValues > myNumValues ) {
    if( myValueBuffer != NULL ) {
      delete [] myValueBuffer;
      myValueBuffer = NULL;
    }
    if( myTimeBuffer != NULL ) {
      delete [] myTimeBuffer;
      myTimeBuffer = NULL;
    }
    myValueBuffer = new T[numValues];
    myTimeBuffer  = new double[numValues];
  }
  myNumValues = numValues;
}


} // namespace
#endif


