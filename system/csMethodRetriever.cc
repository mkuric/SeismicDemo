

#include <string>
#include <fstream>
#include <cstring>
#include "csMethodRetriever.h"
#include "geolib/csException.h"
#include "geolib/csVector.h"
#include "geolib/geolib_string_utils.h"
#include "geolib/geolib_platform_dependent.h"

#ifdef PLATFORM_WINDOWS
#include "cseis_modules_all.h"
#elif PLATFORM_SOLARIS
#include "cseis_modules_all.h"
#elif PLATFORM_LINUX
#include "cseis_modules.h"
#include <dlfcn.h>
#elif PLATFORM_APPLE
#include "cseis_modules.h"
#include <dlfcn.h>
#endif

using namespace cseis_system;

#if defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)
//----------------------------------------------------------
//
void csMethodRetriever::getParamInitMethod( std::string const& name, int verMajor, int verMinor, MParamPtr& param, MInitPtr& init ) {
  std::string nameLower = cseis_geolib::toLowerCase( name );
  
  char* soName     = new char[200];
  char const* ptr  = nameLower.c_str();
  sprintf(soName,"libmod_%s.so.%d.%d",ptr,verMajor,verMinor);

  void* handle = dlopen( soName, RTLD_LAZY );
  const char *dlopen_error = dlerror();
  delete [] soName;

  if( dlopen_error ) {
    throw( cseis_geolib::csException("Error occurred while opening shared library. ...does module '%s' exist? Does version '%d.%d' exist?\nSystem message: %s\n",
                       name.c_str(), verMajor, verMinor, dlopen_error ) );
  }

  param = getParamMethod( nameLower, handle );
  init  = getInitMethod( nameLower, handle );

  //  dlclose(handle);
  //  fprintf(stderr,"Param method pointer found: %x\n", param);
}
//----------------------------------------------------------------------------
//
MParamPtr csMethodRetriever::getParamMethod( std::string const& nameLower, char const* soName ) {
  void* handle = dlopen( soName, RTLD_LAZY );
  const char *dlopen_error = dlerror();
  delete [] soName;
  if( dlopen_error ) {
    fprintf(stdout, "Error occurred while opening shared library. ...does module '%s' exist?\nSystem message: %s\n\n",
            nameLower.c_str(), dlopen_error );
    fflush(stdout);
    throw( cseis_geolib::csException("Error occurred while opening shared library. ...does module '%s' exist?\nSystem message: %s\n",
      				     nameLower.c_str(), dlopen_error ) );
  }
  return getParamMethod( nameLower, handle );
}
MParamPtr csMethodRetriever::getParamMethod( std::string const& name ) {
  std::string nameLower = cseis_geolib::toLowerCase( name );
  char* soName     = new char[200];
  char const* ptr  = nameLower.c_str();
  sprintf(soName,"libmod_%s.so",ptr);
  return getParamMethod( nameLower, soName );
}
MParamPtr csMethodRetriever::getParamMethod( std::string const& name, int verMajor, int verMinor ) {
  std::string nameLower = cseis_geolib::toLowerCase( name );
  char* soName     = new char[200];
  char const* ptr  = nameLower.c_str();
  sprintf(soName,"libmod_%s.so.%d.%d",ptr,verMajor,verMinor);
  return getParamMethod( nameLower, soName );
}
MParamPtr csMethodRetriever::getParamMethod( std::string const& name, std::string versionString ) {
  std::string nameLower = cseis_geolib::toLowerCase( name );
  char* soName     = new char[200];
  char const* ptr  = nameLower.c_str();
  sprintf(soName,"libmod_%s.so.%s",ptr,versionString.c_str());
  return getParamMethod( nameLower, soName );
}
//----------------------------------------------------------------------------
//
MInitPtr csMethodRetriever::getInitMethod( std::string const& nameLower, void* handle  ) {
  char* methodName = new char[200];
  sprintf( methodName, "_init_mod_%s_", nameLower.c_str() );
  
  //  fprintf(stderr,"Init method name:  '%s'\n", methodName );

  //  MInitPtr method = reinterpret_cast<MInitPtr>( dlsym(handle,methodName) );
  MInitPtr method;
  void *ptr = dlsym(handle,methodName);
  memcpy(&method, &ptr, sizeof(void *));

  const char *dlsym_error = dlerror();
  delete [] methodName;
  if( dlsym_error ) {
    throw( cseis_geolib::csException("Cannot find init definition method. System message:\n%s\n", dlsym_error ) );
  }
  return method;
}
//----------------------------------------------------------------------------
//
MParamPtr csMethodRetriever::getParamMethod( std::string const& nameLower, void* handle ) {
  char* methodName = new char[200];
  sprintf( methodName, "_params_mod_%s_", nameLower.c_str() );

  //  MParamPtr method = reinterpret_cast<MParamPtr>( dlsym(handle,methodName) );
  MParamPtr method;
  void *ptr = dlsym(handle,methodName);
  memcpy(&method, &ptr, sizeof(void *));

  const char *dlsym_error = dlerror();

  delete [] methodName;
  if( dlsym_error ) {
    throw( cseis_geolib::csException("Cannot find parameter definition method. System message:\n%s\n", dlsym_error) );
  }

  return method;
}
//--------------------------------------------------------------------
//
void csMethodRetriever::getExecMethodSingleTrace( std::string const& name, int verMajor, int verMinor, MExecSingleTracePtr& exec ) {
  std::string nameLower = cseis_geolib::toLowerCase( name );

  char* soName     = new char[200];
  char const* ptr  = nameLower.c_str();
  sprintf(soName,"libmod_%s.so.%d.%d",ptr,verMajor,verMinor);

  void* handle = dlopen( soName, RTLD_LAZY );
  const char *dlopen_error = dlerror();
  delete [] soName;

  if( dlopen_error ) {
    throw( cseis_geolib::csException("Error occurred while opening shared library. ...does module '%s' exist? Does version '%d.%d' exist?\nSystem message: %s\n",
                       name.c_str(), verMajor, verMinor, dlopen_error ) );
  }

  exec  = getExecMethodSingleTrace( nameLower, handle );

  //  myMethodNameList.insertEnd(nameLower);
  //  myExecList.insertEnd(exec);
}
//----------------------------------------------------------
//
void csMethodRetriever::getExecMethodMultiTrace( std::string const& name, int verMajor, int verMinor, MExecMultiTracePtr& exec ) {

  std::string nameLower = cseis_geolib::toLowerCase( name );

  char* soName     = new char[200];
  char const* ptr  = nameLower.c_str();
  sprintf(soName,"libmod_%s.so.%d.%d",ptr,verMajor,verMinor);

  void* handle = dlopen( soName, RTLD_LAZY );
  const char *dlopen_error = dlerror();
  delete [] soName;

  if( dlopen_error ) {
    //    dlclose(handle);
    throw( cseis_geolib::csException("Error occurred while opening shared library. ...does module '%s' exist? Does version '%d.%d' exist?\nSystem message: %s\n",
                       name.c_str(), verMajor, verMinor, dlopen_error ) );
  }

  exec  = getExecMethodMultiTrace( nameLower, handle );

  //  myMethodNameList.insertEnd(nameLower);
  // myExecList.insertEnd(exec);
}
//
//---------------------------------------------------------
MExecSingleTracePtr csMethodRetriever::getExecMethodSingleTrace( std::string const& nameLower, void* handle  ) {
  char* methodName = new char[200];
  sprintf( methodName, "_exec_mod_%s_", nameLower.c_str() );
  
  //  fprintf(stderr,"Exec method name:  '%s'\n", methodName );

  //  MExecSingleTracePtr method = reinterpret_cast<MExecSingleTracePtr>( dlsym(handle,methodName) );
  MExecSingleTracePtr method;
  void *ptr = dlsym(handle,methodName);
  memcpy(&method, &ptr, sizeof(void *));

  const char *dlsym_error = dlerror();
  delete [] methodName;
  if( dlsym_error ) {
    throw( cseis_geolib::csException("Cannot find exec definition method. System message:\n%s\n", dlsym_error) );
  }
  return method;
}
//---------------------------------------------------------
MExecMultiTracePtr csMethodRetriever::getExecMethodMultiTrace( std::string const& nameLower, void* handle  ) {
  char* methodName = new char[200];
  sprintf( methodName, "_exec_mod_%s_", nameLower.c_str() );
  
  //  fprintf(stderr,"Exec method name:  '%s'\n", methodName );

  //  MExecMultiTracePtr method = reinterpret_cast<MExecMultiTracePtr>( dlsym(handle,methodName) );
  MExecMultiTracePtr method;
  void *ptr = dlsym(handle,methodName);
  memcpy(&method, &ptr, sizeof(void *));

  const char *dlsym_error = dlerror();
  delete [] methodName;
  if( dlsym_error ) {
    throw( cseis_geolib::csException("Cannot find exec definition method. System message:\n%s\n", dlsym_error) );
  }
  return method;
}

#endif

//--------------------------------------------------------------------
//
int csMethodRetriever::getNumStandardModules() {
  return N_METHODS;
}
//--------------------------------------------------------------------
//
std::string const* csMethodRetriever::getStandardModuleNames() {
  return NAMES;
}

#ifdef PLATFORM_WINDOWS

//----------------------------------------------------------
//
void csMethodRetriever::getParamInitMethod( std::string const& name, int verMajor, int verMinor, MParamPtr& param, MInitPtr& init ) {
  int index = getMethodIndex( name );
  if( index >= 0 ) {
    init  = METHODS_INIT[index];
    param = METHODS_PARAM[index];
  }
}
//----------------------------------------------------------------------------
//
MParamPtr csMethodRetriever::getParamMethod( std::string const& name, std::string versionString ) {
  return csMethodRetriever::getParamMethod( name );
}
MParamPtr csMethodRetriever::getParamMethod( std::string const& name ) {
  int index = getMethodIndex( name );
  if( index >= 0 ) {
    return METHODS_PARAM[index];
  }
  else {
    return NULL;
  }
}
//--------------------------------------------------------------------
//
void csMethodRetriever::getExecMethodSingleTrace( std::string const& name, int verMajor, int verMinor, MExecSingleTracePtr& exec ) {
  int index = getMethodIndex( name );
  if( index >= 0 ) {
    exec = METHODS_EXEC_SINGLE[index];
  }
}
//----------------------------------------------------------
//
void csMethodRetriever::getExecMethodMultiTrace( std::string const& name, int verMajor, int verMinor, MExecMultiTracePtr& exec ) {
  int index = getMethodIndex( name );
  if( index >= 0 ) {
    exec = METHODS_EXEC_MULTI[index];
  }
}
//--------------------------------------------------------------------
//
int csMethodRetriever::getMethodIndex( std::string const& name ) {
  for( int i = 0; i < N_METHODS; i++ ) {
    if( !name.compare(NAMES[i]) ) {
      return i;
    }
  }
  return METHOD_NOT_FOUND;
}
#endif


