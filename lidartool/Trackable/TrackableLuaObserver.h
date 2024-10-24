// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef TRACKABLE_LUA_OBSERVER_H
#define TRACKABLE_LUA_OBSERVER_H

#include <mutex>
#include <thread>
#include <atomic>

#include <chrono>
#include <unistd.h>

#include <string.h>
#include <math.h>

#include "lua.hpp"

/***************************************************************************
*** 
*** Lua
***
****************************************************************************/

/***************************************************************************
*** 
*** ObsvLuaUserData
***
****************************************************************************/

class ObsvLuaUserData : public ObsvUserData
{
public:
  void			*m_Lua;
  int			 m_TableRef;
  ObsvLuaUserData( void *lua, int tableIndex=0 );

  ~ObsvLuaUserData();
  
};

/***************************************************************************
*** 
*** TrackableLuaRegions
***
****************************************************************************/

class TrackableRegion;

class TrackableLuaRegions : public std::vector<TrackableRegion*>
{
public:
  TrackableRegion   *get( const char *name );
  
};

/***************************************************************************
*** 
*** TrackableLuaObserver
***
****************************************************************************/

class TrackableLuaObserver : public TrackableFileObserver
{
public:

  lua_State			*m_Lua;
  TrackableLuaRegions		regions;
  
  KeyValueMap 			m_Descr;
  
  void registerMethod( lua_CFunction function );
  bool call( const char *funcName, uint64_t timestamp=0,   bool asError=false );
  bool call( const char *funcName, ObsvObjects *objects, uint64_t timestamp, bool asError=false );
  bool call( const char *funcName, ObsvObject  *object,  uint64_t timestamp, bool asError=false );

  void openLua();
  void closeLua();
  
  TrackableLuaObserver()
  : TrackableFileObserver(),
    m_Lua( NULL )
  {
    type 	   = Lua;
    continuous	   = true;
    fullFrame      = false;
    isJson         = false;
    isThreaded     = false;
    name           = "lua";

    obsvFilter.parseFilter( "timestamp=ts" );
  }

  ~TrackableLuaObserver();

  void	initialize();
  
  virtual void setParam( KeyValueMap &descr )
  {
    TrackableFileObserver::setParam( descr );
    m_Descr = descr;
  }

  virtual void setFileName( const char *fileName )
  {
    TrackableFileObserver::setFileName( fileName );
    isThreaded = !logFileName.empty();
  }

  void report();

  bool stall ( uint64_t timestamp=0 );
  bool resume( uint64_t timestamp=0 );
  bool start ( uint64_t timestamp=0, bool startRects=false );
  bool stop  ( uint64_t timestamp=0, bool stopRects=false );
 
};


#endif // TRACKABLE_LUA_OBSERVER_H

