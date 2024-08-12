// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#if USE_LUA

#ifndef TRACKABLE_LUA_OBSERVER_CPP
#define TRACKABLE_LUA_OBSERVER_CPP

#include "keyValueMap.h"
#include "TrackableObserver.h"
#include "TrackableLuaObserver.h"
#include "TrackBase.h"

/***************************************************************************
*** 
*** Lua
***
****************************************************************************/

typedef int (*Lua_CFunction) (lua_State *lua, ...);

typedef struct LuaFunc_Reg {
  const char *name;
  Lua_CFunction func;
} LuaFunc_Reg;

/***************************************************************************
*** 
*** registerClass
***
****************************************************************************/

#define getInstance( instance,  lua, Class )	\
Class *instance; \
{ Class *get_##Class( lua_State *lua, int index=1 );	\
  instance = get_##Class( lua ); \
}

#define registerClass( Class, TableString )	\
const std::string Class##TableName( TableString );    \
int dispatchNewIndex##Class( lua_State *lua )				\
{ Lua_CFunction newIndexFunc = NULL;					   \
  extern LuaFunc_Reg Class##Functions[]; \
  const char *key = luaL_checkstring( lua, 2 );	   \
  for ( int i = 0; Class##Functions[i].name != NULL; ++i )	\
  {	\
    if ( strcmp(Class##Functions[i].name,key) == 0 )			\
    { lua_pushcfunction( lua, (lua_CFunction) Class##Functions[i].func ); \
      return 0;\
    }						    \
    else if ( strcmp(Class##Functions[i].name,"__newindex") == 0 )		\
      newIndexFunc = Class##Functions[i].func;				\
  }		   \
  if ( newIndexFunc != NULL ) \
  { int result = newIndexFunc( lua, key );		\
    if ( result > 0 )					\
      return 0;					\
  }						\
  lua_getuservalue( lua, 1 ); \
  if ( !lua_istable( lua, -1 ) ) \
  { lua_pop( lua, 1 ); \
    lua_newtable( lua ); \
    lua_pushvalue( lua, -1 );			\
    lua_setuservalue( lua, 1 ); \
  } \
  lua_pushvalue( lua, -2 );			\
  lua_setfield( lua, -2, key );			\
  lua_pop( lua, 1 );				\
  return 0; \
} \
 \
int dispatchIndex##Class( lua_State *lua )				\
{ Lua_CFunction indexFunc = NULL;					   \
  Lua_CFunction methodsFunc = NULL;					   \
  Lua_CFunction isKnownMethod = NULL;					   \
  extern LuaFunc_Reg Class##Functions[]; \
  const char *key = luaL_checkstring( lua, 2 );	    \
  lua_getmetatable(lua, 1 );   \
  lua_getuservalue( lua, 1 );  \
  if ( lua_istable( lua, -1 ) )  \
  { lua_getfield( lua, -1, key );  \
    if ( !lua_isnil( lua, -1 ) )  \
      return 1;  \
    lua_pop( lua, 1 );  \
  }  \
  lua_pop( lua, 2);  \
  \
  for ( int i = 0; Class##Functions[i].name != NULL; ++i )	\
  {	\
    if ( strcmp(Class##Functions[i].name,key) == 0 )			\
      { lua_pushcfunction( lua, (lua_CFunction) Class##Functions[i].func ); \
      return 1;\
    }						    \
    else if ( strcmp(Class##Functions[i].name,"__index") == 0 )		\
      indexFunc = Class##Functions[i].func;				\
    else if ( strcmp(Class##Functions[i].name,"__methods") == 0 )		\
      methodsFunc = Class##Functions[i].func;	\
    else if ( strcmp(Class##Functions[i].name,"__knownMethods") == 0 )		\
      isKnownMethod = Class##Functions[i].func;	\
  }		   \
  if ( indexFunc != NULL ) \
  { \
    int result = indexFunc( lua, key );		\
    if ( result > 0 ) \
       return result;					\
  }						\
  if ( methodsFunc != NULL ) \
  { if ( isKnownMethod == NULL || isKnownMethod( lua, key, 1 ) )	\
    { lua_pushstring( lua, key );				\
      lua_pushcclosure( lua, (lua_CFunction) methodsFunc, 1 );	\
      return 1;				  	\
    }						\
  }						\
  lua_pushnil( lua );				    \
  return 1;\
} \
Class *get_##Class( lua_State *lua, int index=1 )			\
{ return *(Class **)luaL_checkudata( lua, index, Class##TableName.c_str() ); \
} \
void register_##Class( lua_State *lua, Class *instance, int tableIndex=0 ) \
{  \
  Class **pptr = (Class**) lua_newuserdata(lua, sizeof(Class*)); \
  *pptr = (instance);						 \
  if (luaL_newmetatable(lua, Class##TableName.c_str()))	\
  { \
    lua_pushvalue(lua, -1 );			   \
    lua_setfield (lua, -2, "__index");				   \
    lua_pushvalue(lua, -1 );   					   \
    lua_setfield (lua, -2, "__newindex");   			   \
    lua_pushcfunction(lua, dispatchIndex##Class );		   \
    lua_setfield (lua, -2, "__index");				   \
    lua_pushcfunction(lua, dispatchNewIndex##Class );		   \
    lua_setfield (lua, -2, "__newindex");				   \
  }								   \
  if (tableIndex != 0)					   \
    lua_pushvalue(lua, -2+tableIndex ); \
  else						   \
    lua_pushnil(lua ); \
  lua_setuservalue(lua, -3 ); \
  lua_setmetatable(lua, -2); \
} \

#define registerUserDataClass( Class, TableString )		\
  registerClass( Class, TableString )					\
void registerUserData_##Class( lua_State *lua, Class *instance )	\
{	\
  ObsvLuaUserData *uData = static_cast<ObsvLuaUserData *>( instance->userData ); \
  if ( uData == NULL )	\
  { lua_newtable( lua );	\
    instance->userData = uData = new ObsvLuaUserData(lua,-1);	\
    lua_pop( lua, 1 );							\
  }									\
  \
  lua_rawgeti( lua, LUA_REGISTRYINDEX, uData->m_TableRef );	\
  if ( lua_type( lua, -1 ) == LUA_TTABLE )	\
    register_##Class( lua, instance, -1 );	\
  else	\
    register_##Class( lua, instance );	\
} \


#define registerInstance( lua, instance, Class ) register_##Class( lua, instance )
#define registerUserDataInstance( lua, instance, Class ) registerUserData_##Class( lua, instance )
#define registerInstanceWithTable( lua, instance, Class, tableIndex ) register_##Class( lua, instance, tableIndex )

/***************************************************************************
*** 
*** ObsvLuaUserData
***
****************************************************************************/

ObsvLuaUserData::ObsvLuaUserData( void *lua, int tableIndex )
  : m_Lua     ( lua ),
    m_TableRef( 0 )
{
  if ( tableIndex != 0 )
    { lua_pushvalue( static_cast<lua_State*>( lua ), tableIndex );
    m_TableRef = luaL_ref( static_cast<lua_State*>( lua ), LUA_REGISTRYINDEX );
  } 
}

ObsvLuaUserData::~ObsvLuaUserData()  
{
  if ( m_Lua != NULL && m_TableRef != 0 )
    luaL_unref( static_cast<lua_State*>( m_Lua ), LUA_REGISTRYINDEX, m_TableRef );
}

/***************************************************************************
*** 
*** TrackableLuaRegions
***
****************************************************************************/

TrackableRegion *
TrackableLuaRegions::get( const char *name )
{
  for ( int i = (int)(this->size()-1); i >= 0; --i )
    if ( (*this)[i]->name == name )
      return (*this)[i];

  return NULL;
}

/***************************************************************************
*** 
*** Class Definitions
***
****************************************************************************/

registerUserDataClass( ObsvObject,   "ObsvObject" )
registerUserDataClass( ObsvObjects,  "ObsvObjects" )
registerClass( TrackableRegion,  "TrackableRegion" )
registerClass( TrackableRegions,  "TrackableRegions" )
registerClass( TrackableLuaRegions,  "TrackableLuaRegions" )
registerClass( TrackableLuaObserver, "TrackableLuaObserver" )

#define registerGetFunction( name, lua, Class, type ) \
int Class##_##name( lua_State *lua ) \
{ getInstance( Ptr, lua, Class ); \
  lua_push##type( lua, Ptr->name ); \
  return 1; \
}

/***************************************************************************
*** 
*** ObsvObjectFunctions
***
****************************************************************************/

registerGetFunction( size, lua, ObsvObject, integer )
registerGetFunction( id, lua, ObsvObject, integer )
registerGetFunction( timestamp, lua, ObsvObject, integer )
registerGetFunction( timestamp_enter, lua, ObsvObject, integer )
registerGetFunction( timestamp_touched, lua, ObsvObject, integer )

int
ObsvObject_x( lua_State *lua )
{
  getInstance( object, lua, ObsvObject );
  lua_pushnumber( lua, object->x - object->objects->centerX );
  return 1;
}

int
ObsvObject_y( lua_State *lua )
{
  getInstance( object, lua, ObsvObject );
  lua_pushnumber( lua, object->y - object->objects->centerY );
  return 1;
}

int
ObsvObject_z( lua_State *lua )
{
  getInstance( object, lua, ObsvObject );
  lua_pushnumber( lua, object->z - object->objects->centerZ );
  return 1;
}

int
ObsvObject_lifeSpan( lua_State *lua )
{
  getInstance( object, lua, ObsvObject );
  lua_pushnumber( lua, object->timestamp_touched - object->timestamp_enter );
  return 1;
}

int
ObsvObject_hasMoved( lua_State *lua )
{
  getInstance( object, lua, ObsvObject );

  lua_getglobal( lua, "obsv" );
  lua_getfield( lua, -1, "observerPointer" );
  TrackableLuaObserver *obsv = static_cast<TrackableLuaObserver*>( lua_touserdata(lua, -1) );
  lua_pushboolean( lua, (obsv->continuous || object->d >= obsv->reportDistance) && (object->status == ObsvObject::Move) );
  return 1;
}

int
ObsvObject_moveDone( lua_State *lua )
{
  getInstance( object, lua, ObsvObject );
  object->moveDone();
  return 0;
}

int
ObsvObject_movedDistance( lua_State *lua )
{
  getInstance( object, lua, ObsvObject );
  lua_pushnumber( lua, object->d );
  return 1;
}

int
ObsvObject_uuid( lua_State *lua )
{
  getInstance( object, lua, ObsvObject );
  lua_pushstring( lua, object->uuid.str().c_str() );
  return 1;
}

int
ObsvObject_type( lua_State *lua )
{
  getInstance( object, lua, ObsvObject );

  if ( object->status == ObsvObject::Move )
    lua_pushstring( lua, "move" );
  else if ( object->status == ObsvObject::Enter )
    lua_pushstring( lua, "enter" );
  else
    lua_pushstring( lua, "leave" );

  return 1;
}

int
ObsvObject_objects( lua_State *lua )
{
  getInstance( object, lua, ObsvObject );

  registerUserDataInstance( lua, object->objects, ObsvObjects );

  return 1;
}

LuaFunc_Reg ObsvObjectFunctions[] =
{
  { "x", (Lua_CFunction)ObsvObject_x },
  { "y", (Lua_CFunction)ObsvObject_y },
  { "z", (Lua_CFunction)ObsvObject_z },
  { "size", (Lua_CFunction)ObsvObject_size },
  { "lifeSpan", (Lua_CFunction)ObsvObject_lifeSpan },
  { "hasMoved", (Lua_CFunction)ObsvObject_hasMoved },
  { "movedDistance", (Lua_CFunction)ObsvObject_movedDistance },
  { "moveDone", (Lua_CFunction)ObsvObject_moveDone },
  { "id", (Lua_CFunction)ObsvObject_id },
  { "type", (Lua_CFunction)ObsvObject_type },
  { "uuid", (Lua_CFunction)ObsvObject_uuid },
  { "timestamp", (Lua_CFunction)ObsvObject_timestamp },
  { "timestamp_enter", (Lua_CFunction)ObsvObject_timestamp_enter },
  { "timestamp_touched", (Lua_CFunction)ObsvObject_timestamp_touched },
  { "objects", (Lua_CFunction)ObsvObject_objects },
  { NULL, NULL }
};

/***************************************************************************
*** 
*** ObsvObjectsFunctions
***
****************************************************************************/

registerGetFunction( alive, lua, ObsvObjects, boolean )
registerGetFunction( centerX, lua, ObsvObjects, number )
registerGetFunction( centerY, lua, ObsvObjects, number )
registerGetFunction( centerZ, lua, ObsvObjects, number )
registerGetFunction( frame_id, lua, ObsvObjects, integer )
registerGetFunction( enterCount, lua, ObsvObjects, integer )
registerGetFunction( leaveCount, lua, ObsvObjects, integer )
registerGetFunction( gateCount, lua, ObsvObjects, integer )
registerGetFunction( avgLifespan, lua, ObsvObjects, number )
registerGetFunction( timestamp, lua, ObsvObjects, integer )
registerGetFunction( operational, lua, ObsvObjects, number )

int
ObsvObjects_size( lua_State *lua )
{
  getInstance( objects, lua, ObsvObjects );
  lua_pushinteger( lua, objects->size() );
  return 1;
}

int
ObsvObjects_count( lua_State *lua )
{
  getInstance( objects, lua, ObsvObjects );

  bool includePrivate = true;
  int valueType = lua_type( lua, 2 );
  if ( valueType == LUA_TBOOLEAN )
    includePrivate = lua_toboolean( lua, 2 );

  if ( includePrivate )
    lua_pushinteger( lua, objects->validCount );
  else
  {
    int validCount = 0;
 
    for ( auto &iter: *objects )
      if ( iter.second.status == ObsvObject::Move )
	if ( !iter.second.isPrivate() )
	  validCount += 1;
    
    lua_pushinteger( lua, objects->validCount );
  }
  
  return 1;
}

int
ObsvObjects_switch( lua_State *lua )
{
  getInstance( objects, lua, ObsvObjects );
  bool includePrivate = true;
  int valueType = lua_type( lua, 2 );
  if ( valueType == LUA_TBOOLEAN )
    includePrivate = lua_toboolean( lua, 2 );

  if ( includePrivate )
    lua_pushboolean( lua, objects->validCount );
  else
  {
    for ( auto &iter: *objects )
      if ( iter.second.status == ObsvObject::Move )
      {
	if ( !iter.second.isPrivate() )
        { lua_pushboolean( lua, true );
	  break;
	}
      }
  }

  return 1;
}

int
ObsvObjects_switchduration( lua_State *lua )
{
  getInstance( objects, lua, ObsvObjects );

  if ( objects->switch_timestamp == 0 )
    lua_pushinteger( lua, objects->switch_timestamp );
  else
    lua_pushinteger( lua, objects->timestamp-objects->switch_timestamp );
    
  return 1;
}

int
ObsvObjects_regionName( lua_State *lua )
{
  getInstance( objects, lua, ObsvObjects );
  lua_pushstring( lua, objects->region.c_str() );
  return 1;
}

int
ObsvObjects_regionX( lua_State *lua )
{
  getInstance( objects, lua, ObsvObjects );
  lua_pushnumber( lua, objects->rect->x + objects->rect->width / 2 );
  return 1;
}

int
ObsvObjects_regionY( lua_State *lua )
{
  getInstance( objects, lua, ObsvObjects );
  lua_pushnumber( lua, objects->rect->y + objects->rect->height / 2 );
  return 1;
}

int
ObsvObjects_regionWidth( lua_State *lua )
{
  getInstance( objects, lua, ObsvObjects );
  lua_pushnumber( lua, objects->rect->width );
  return 1;
}

int
ObsvObjects_regionHeight( lua_State *lua )
{
  getInstance( objects, lua, ObsvObjects );
  lua_pushnumber( lua, objects->rect->height );
  return 1;
}

int
ObsvObjects_at( lua_State *lua )
{
  getInstance( objects, lua, ObsvObjects );

  int index = luaL_checkinteger( lua, 2 );
  ObsvObjects::iterator iter( objects->begin() );
  std::advance(iter, index);

  ObsvObject *obj = &iter->second;
  registerUserDataInstance( lua, obj, ObsvObject );

  return 1;
}

int
ObsvObjects_byId( lua_State *lua )
{
  getInstance( objects, lua, ObsvObjects );

  int index = luaL_checkinteger( lua, 2 );

  ObsvObject *obj = &(*objects)[index];
  registerUserDataInstance( lua, obj, ObsvObject );

  return 1;
}

LuaFunc_Reg ObsvObjectsFunctions[] =
{
  { "size", (Lua_CFunction)ObsvObjects_size },
  { "alive", (Lua_CFunction)ObsvObjects_alive },
  { "centerX", (Lua_CFunction)ObsvObjects_centerX },
  { "centerY", (Lua_CFunction)ObsvObjects_centerY },
  { "centerZ", (Lua_CFunction)ObsvObjects_centerZ },
  { "count", (Lua_CFunction)ObsvObjects_count },
  { "switch", (Lua_CFunction)ObsvObjects_switch },
  { "switchduration", (Lua_CFunction)ObsvObjects_switchduration },
  { "regionName", (Lua_CFunction)ObsvObjects_regionName },
  { "regionX", (Lua_CFunction)ObsvObjects_regionX },
  { "regionY", (Lua_CFunction)ObsvObjects_regionY },
  { "regionWidth", (Lua_CFunction)ObsvObjects_regionWidth },
  { "regionHeight", (Lua_CFunction)ObsvObjects_regionHeight },
  { "frameId", (Lua_CFunction)ObsvObjects_frame_id },
  { "timestamp", (Lua_CFunction)ObsvObjects_timestamp },
  { "enterCount", (Lua_CFunction)ObsvObjects_enterCount },
  { "leaveCount", (Lua_CFunction)ObsvObjects_leaveCount },
  { "gateCount", (Lua_CFunction)ObsvObjects_gateCount },
  { "avgLifespan", (Lua_CFunction)ObsvObjects_avgLifespan },
  { "operational", (Lua_CFunction)ObsvObjects_operational },
  { "byId", (Lua_CFunction)ObsvObjects_byId },
  { "at", (Lua_CFunction)ObsvObjects_at },
  { "__index", (Lua_CFunction)ObsvObjects_at },
  { NULL, NULL }
};

/***************************************************************************
*** 
*** TrackableRegionFunctions
***
****************************************************************************/

registerGetFunction( x, lua, TrackableRegion, number )
registerGetFunction( y, lua, TrackableRegion, number )
registerGetFunction( width, lua, TrackableRegion, number )
registerGetFunction( height, lua, TrackableRegion, number )

int
TrackableRegion_name( lua_State *lua )
{
  getInstance( region, lua, TrackableRegion );
  lua_pushstring( lua, region->name.c_str() );
  return 1;
}

int
TrackableRegion_shape( lua_State *lua )
{
  getInstance( region, lua, TrackableRegion );
  lua_pushstring( lua, TrackableRegion::regionShape_str( (RegionShape)region->shape ).c_str() );
  return 1;
}

int
TrackableRegion_layers( lua_State *lua )
{
  getInstance( region, lua, TrackableRegion );
  lua_pushstring( lua, region->layersStr.c_str() );
  return 1;
}

int
TrackableRegion_tags( lua_State *lua )
{
  getInstance( region, lua, TrackableRegion );
  lua_pushstring( lua, region->tagsStr.c_str() );
  return 1;
}

int
TrackableRegion_hasLayer( lua_State *lua )
{
  getInstance( region, lua, TrackableRegion );
  std::string layer( luaL_checkstring( lua, 2 ) );
  bool result = region->hasLayer( layer );
  lua_pushboolean( lua, result );
  return 1;
}

int
TrackableRegion_hasTag( lua_State *lua )
{
  getInstance( region, lua, TrackableRegion );
  std::string tag( luaL_checkstring( lua, 2 ) );
  bool result = region->hasTag( tag );
  lua_pushboolean( lua, result );
  return 1;
}

int
TrackableRegion_contains( lua_State *lua )
{
  getInstance( region, lua, TrackableRegion );
  float size = 0.0;
  float x = luaL_checknumber( lua, 2 );
  float y = luaL_checknumber( lua, 3 );
  int valueType = lua_type( lua, 4 );
  if ( valueType == LUA_TNUMBER )
    size = lua_tonumber( lua, 4 );

  bool result = region->contains( x, y, size );
  lua_pushboolean( lua, result );
  return 1;
}

LuaFunc_Reg TrackableRegionFunctions[] =
{
  { "x", (Lua_CFunction)TrackableRegion_x },
  { "y", (Lua_CFunction)TrackableRegion_y },
  { "width", (Lua_CFunction)TrackableRegion_width },
  { "height", (Lua_CFunction)TrackableRegion_height },
  { "name", (Lua_CFunction)TrackableRegion_name },
  { "shape", (Lua_CFunction)TrackableRegion_shape },
  { "layers", (Lua_CFunction)TrackableRegion_layers },
  { "tags", (Lua_CFunction)TrackableRegion_tags },
  { "hasLayer", (Lua_CFunction)TrackableRegion_hasLayer },
  { "hasTag", (Lua_CFunction)TrackableRegion_hasTag },
  { "contains", (Lua_CFunction)TrackableRegion_contains },
  { NULL, NULL }
};

/***************************************************************************
*** 
*** TrackableRegionsFunctions
***
****************************************************************************/

int
TrackableRegions_size( lua_State *lua )
{
  getInstance( regions, lua, TrackableRegions );
  lua_pushinteger( lua, regions->size() );
  return 1;
}

int
TrackableRegions_index( lua_State *lua )
{
  getInstance( regions, lua, TrackableRegions );
  TrackableRegion *region = NULL;

  int valueType = lua_type( lua, 2 );
  if ( valueType == LUA_TSTRING )
  { std::string name = lua_tostring( lua, 2 );
    region = regions->get( name.c_str() );
  }
  else if ( valueType == LUA_TNUMBER )
    region = &(*regions)[ lua_tointeger( lua, 2 ) ];

  if ( region != NULL )
    registerInstance( lua, region, TrackableRegion );
  else
    lua_pushnil( lua );		   

  return 1;
}

LuaFunc_Reg TrackableRegionsFunctions[] =
{
  { "size", (Lua_CFunction)TrackableRegions_size },
  { "region", (Lua_CFunction)TrackableRegions_index },
  { "__index", (Lua_CFunction)TrackableRegions_index },
  { NULL, NULL }
};

/***************************************************************************
*** 
*** TrackableLuaRegionsFunctions
***
****************************************************************************/

int
TrackableLuaRegions_size( lua_State *lua )
{
  getInstance( regions, lua, TrackableLuaRegions );
  lua_pushinteger( lua, regions->size() );
  return 1;
}

int
TrackableLuaRegions_index( lua_State *lua )
{
  getInstance( regions, lua, TrackableLuaRegions );
  TrackableRegion *region = NULL;

  int valueType = lua_type( lua, 2 );
  if ( valueType == LUA_TSTRING )
  { std::string name = lua_tostring( lua, 2 );
    region = regions->get( name.c_str() );
  }
  else if ( valueType == LUA_TNUMBER )
    region = (*regions)[ lua_tointeger( lua, 2 ) ];

  if ( region != NULL )
    registerInstance( lua, region, TrackableRegion );
  else
    lua_pushnil( lua );		   

  return 1;
}

LuaFunc_Reg TrackableLuaRegionsFunctions[] =
{
  { "size", (Lua_CFunction)TrackableLuaRegions_size },
  { "region", (Lua_CFunction)TrackableLuaRegions_index },
  { "__index", (Lua_CFunction)TrackableLuaRegions_index },
  { NULL, NULL }
};

/***************************************************************************
*** 
*** TrackableLuaObserverFunctions
***
****************************************************************************/

int
TrackableLuaObserver_numRects( lua_State *lua )
{
  TrackableLuaObserver *observer = get_TrackableLuaObserver( lua, lua_upvalueindex(1) );
  
  int size = observer->rects.size();

  lua_pushinteger( lua, size );

  return 1;
}

int
TrackableLuaObserver_isStarted( lua_State *lua )
{
  TrackableLuaObserver *observer = get_TrackableLuaObserver( lua, lua_upvalueindex(1) );

  bool isStarted = false;
  observer->m_Descr.get( "isStarted", isStarted );

  lua_pushboolean( lua, isStarted );

  return 1;
}

int
TrackableLuaObserver_timestamp( lua_State *lua )
{
  TrackableLuaObserver *observer = get_TrackableLuaObserver( lua, lua_upvalueindex(1) );

  uint64_t timestamp = observer->timestamp;
  std::string format;

  int valueType = lua_type( lua, 1 );
  if ( valueType == LUA_TSTRING )
    format = lua_tostring( lua, 1 );
  else if ( valueType == LUA_TNUMBER )
    timestamp = lua_tointeger( lua, 1 );

  valueType = lua_type( lua, 2 );
  if ( valueType == LUA_TSTRING )
    format = lua_tostring( lua, 2 );
  else if ( valueType == LUA_TNUMBER )
    timestamp = lua_tointeger( lua, 2 );

  const char *templ = (format.empty() ? NULL : format.c_str());
  std::string ts( timestampString( templ, timestamp, false ) );
  lua_pushstring( lua, ts.c_str() );

  return 1;
}

int
TrackableLuaObserver_fileName( lua_State *lua )
{
  TrackableLuaObserver *observer = get_TrackableLuaObserver( lua, lua_upvalueindex(1) );

  std::string fn;
  int valueType = lua_type( lua, 1 );
  if ( valueType == LUA_TSTRING )
    fn = TrackableObserver::configFileName( observer->applyDateToString( luaL_checkstring( lua, 1 ), observer->timestamp ).c_str() );
  else
    fn = observer->templateToFileName( observer->timestamp );

  lua_pushstring( lua, fn.c_str() );

  return 1;
}

int
TrackableLuaObserver_setFileName( lua_State *lua )
{
  TrackableLuaObserver *observer = get_TrackableLuaObserver( lua, lua_upvalueindex(1) );

  std::string fn( luaL_checkstring( lua, 1 ) );
  observer->setFileName( fn.c_str() );

  fn = observer->templateToFileName( observer->timestamp );
  lua_pushstring( lua, fn.c_str() );

  return 1;
}

int
TrackableLuaObserver_setStatusMsg( lua_State *lua )
{
  TrackableLuaObserver *observer = get_TrackableLuaObserver( lua, lua_upvalueindex(1) );

  std::string msg( luaL_checkstring( lua, 1 ) );

  if ( observer->statusMsg != msg )
    observer->statusMsg = msg;
  
  return 0;
}

int
TrackableLuaObserver_writeJsonMsg( lua_State *lua )
{
  TrackableLuaObserver *observer = get_TrackableLuaObserver( lua, lua_upvalueindex(1) );

  uint64_t timestamp = observer->timestamp;
  std::string msg;

  int valueType = lua_type( lua, 1 );
  if ( valueType == LUA_TSTRING )
    msg = lua_tostring( lua, 1 );
  else if ( valueType == LUA_TNUMBER )
    timestamp = lua_tointeger( lua, 1 );

  valueType = lua_type( lua, 2 );
  if ( valueType == LUA_TSTRING )
    msg = lua_tostring( lua, 2 );
  else if ( valueType == LUA_TNUMBER )
    timestamp = lua_tointeger( lua, 2 );

  observer->writeJsonMsg( msg, timestamp );

  return 0;
}


int
TrackableLuaObserver_descrGetBool( lua_State *lua )
{
  TrackableLuaObserver *observer = get_TrackableLuaObserver( lua, lua_upvalueindex(1) );
  
  const char *key = luaL_checkstring( lua, 1 );
  bool value = false;
  if ( lua_type( lua, 2 ) == LUA_TBOOLEAN )
    value = lua_toboolean( lua, 2 );

  bool success = observer->m_Descr.get( key, value );
  lua_pushboolean( lua, value );

  return 1;
}

int
TrackableLuaObserver_descrGetString( lua_State *lua )
{
  TrackableLuaObserver *observer = get_TrackableLuaObserver( lua, lua_upvalueindex(1) );
  
  const char *key = luaL_checkstring( lua, 1 );
  std::string value;
  if ( lua_type( lua, 2 ) == LUA_TSTRING )
    value = lua_tostring( lua, 2 );

  bool success = observer->m_Descr.get( key, value );
  lua_pushstring( lua, value.c_str() );

  return 1;
}

int
TrackableLuaObserver_descrGetNumber( lua_State *lua )
{
  TrackableLuaObserver *observer = get_TrackableLuaObserver( lua, lua_upvalueindex(1) );
  
  const char *key = luaL_checkstring( lua, 1 );
  double value;
  if ( lua_type( lua, 2 ) == LUA_TNUMBER )
    value = lua_tonumber( lua, 2 );

  bool success = observer->m_Descr.get( key, value );
  lua_pushnumber( lua, value );
//  lua_pushboolean( lua, success );

  return 1;
}

int
TrackableLuaObserver_descrGetInteger( lua_State *lua )
{
  TrackableLuaObserver *observer = get_TrackableLuaObserver( lua, lua_upvalueindex(1) );
  
  const char *key = luaL_checkstring( lua, 1 );
  int value;
  if ( lua_type( lua, 2 ) == LUA_TNUMBER )
    value = lua_tointeger( lua, 2 );

  bool success = observer->m_Descr.get( key, value );
  lua_pushinteger( lua, value );

  return 1;
}

int
TrackableLuaObserver_objects( lua_State *lua )
{
  TrackableLuaObserver *observer = get_TrackableLuaObserver( lua, lua_upvalueindex(1) );
  
  int valueType = lua_type( lua, 1 );
  if ( valueType == LUA_TSTRING )
  { std::string regionName( lua_tostring( lua, 1 ) );
    if ( regionName.empty() )
    { ObsvObjects *objects = &observer->rects.rect(0).objects;
      registerUserDataInstance( lua, objects, ObsvObjects );
    }
    else
    { int index;
      for ( index = ((int)observer->rects.size())-1; index >= 0; --index )
	if ( observer->rects.rect(index).name == regionName )
        { ObsvObjects *objects = &observer->rects.rect(index).objects;
	  registerUserDataInstance( lua, objects, ObsvObjects );
	  break;
	}

      if ( index < 0 )
	lua_pushnil( lua );
    }
  }
  else if ( valueType == LUA_TNUMBER )
  {
    int index = luaL_checkinteger( lua, 1 );
    if ( index >= 0 && index < observer->rects.size() )
    { ObsvObjects *objects = &observer->rects.rect(index).objects;
      registerUserDataInstance( lua, objects, ObsvObjects );
    }
    else
      lua_pushnil( lua );
  }
  else
  { ObsvObjects *objects = &observer->rects.rect(0).objects;
    registerUserDataInstance( lua, objects, ObsvObjects );
  }

  return 1;
}

int
TrackableLuaObserver_action( lua_State *lua )
{
  getInstance( observer, lua, TrackableLuaObserver );

  if ( observer->startStopStatusChanged == 1 )
    lua_pushstring( lua, "start" );
  else if ( observer->startStopStatusChanged == 0 )
    lua_pushstring( lua, "stop" );
  else
    lua_pushstring( lua, "" );

  return 1;
}

int
TrackableLuaObserver_verbose( lua_State *lua )
{
  getInstance( observer, lua, TrackableLuaObserver );
  lua_pushboolean( lua, observer->verbose );
  return 1;
}

/*
int
TrackableLuaObserver_index( lua_State *lua )
{
  TrackableLuaObserver *observer = get_TrackableLuaObserver( lua, lua_upvalueindex(1) );
  
  const char *name = luaL_checkstring( lua, 2 );
  std::string value;
  if ( observer->m_Descr.get( name, value ) )
    lua_pushstring( lua, value.c_str() );
  else
    lua_pushnil(lua );		   

  return 1;
}
*/

LuaFunc_Reg TrackableLuaObserverFunctions[] =
{
/*
  { "numRegions", TrackableLuaObserver_numRects },
  { "fileName", TrackableLuaObserver_fileName },
  { "setFileName", TrackableLuaObserver_setFileName },
  { "log", TrackableLuaObserver_writeJsonMsg },
  { "configDir", TrackableLuaObserver_configDir },
  { "timestamp", TrackableLuaObserver_timestamp },
  { "objects", TrackableLuaObserver_objects },
  { "action", TrackableLuaObserver_action },
  { "verbose", TrackableLuaObserver_verbose },
*/
  { NULL, NULL }
};

/***************************************************************************
*** 
*** TrackableLuaObserver
***
****************************************************************************/

TrackableLuaObserver::~TrackableLuaObserver()
{
  closeLua();
}

void
TrackableLuaObserver::registerMethod( lua_CFunction function )
{
  registerInstance( m_Lua, this, TrackableLuaObserver );
  lua_pushcclosure( m_Lua, function, 1 );
}

void
TrackableLuaObserver::closeLua()
{
  if ( m_Lua == NULL )
    return;
  
  lua_close( m_Lua );

  m_Lua = NULL;
}

void
TrackableLuaObserver::openLua()
{
  closeLua();

  m_Lua = luaL_newstate();
  
  luaL_openlibs( m_Lua );

  lua_newtable( m_Lua );

  lua_pushlightuserdata( m_Lua, this );
  lua_setfield( m_Lua, -2, "observerPointer" );

  registerInstance( m_Lua, &this->regions, TrackableLuaRegions );
  lua_setfield ( m_Lua, -2, "regions" );
  
  registerMethod( TrackableLuaObserver_fileName );
  lua_setfield ( m_Lua, -2, "logFileName" );
  
  registerMethod( TrackableLuaObserver_setFileName );
  lua_setfield ( m_Lua, -2, "setLogFileName" );
  
  registerMethod( TrackableLuaObserver_writeJsonMsg );
  lua_setfield ( m_Lua, -2, "writeJson" );
  
  registerMethod( TrackableLuaObserver_setStatusMsg );
  lua_setfield ( m_Lua, -2, "setStatusMsg" );
  
  lua_pushinteger( m_Lua, rects.size() );
  lua_setfield ( m_Lua, -2, "objectsCount");

  registerMethod( TrackableLuaObserver_objects );
  lua_setfield ( m_Lua, -2, "objects");

  registerMethod( TrackableLuaObserver_action );
  lua_setfield ( m_Lua, -2, "action");

  registerMethod( TrackableLuaObserver_timestamp );
  lua_setfield ( m_Lua, -2, "timestamp");

  lua_pushstring( m_Lua, name.c_str() );
  lua_setfield ( m_Lua, -2, "name");

  lua_pushboolean( m_Lua, verbose );
  lua_setfield ( m_Lua, -2, "verbose");

  lua_newtable( m_Lua );
  
  for ( KeyValueMap::iterator iter( m_Descr.begin() ); iter != m_Descr.end(); iter++ )
  { lua_pushstring( m_Lua, iter->second.c_str() );
    lua_setfield ( m_Lua, -2, iter->first.c_str() );
  }
  
  registerMethod( TrackableLuaObserver_descrGetBool );
  lua_setfield ( m_Lua, -2, "bool");

  registerMethod( TrackableLuaObserver_descrGetNumber );
  lua_setfield ( m_Lua, -2, "number");

  registerMethod( TrackableLuaObserver_descrGetInteger );
  lua_setfield ( m_Lua, -2, "integer");

  registerMethod( TrackableLuaObserver_descrGetString );
  lua_setfield ( m_Lua, -2, "string");

  lua_setfield ( m_Lua, -2, "param");
  

/*	GLOBAL	cc	*/

  lua_setglobal( m_Lua, "obsv" );

  lua_newtable( m_Lua );

  lua_pushstring( m_Lua, TrackGlobal::configDir.c_str() );
  lua_setfield ( m_Lua, -2, "configDir" );
  
  registerInstance( m_Lua, &TrackGlobal::regions, TrackableRegions );
  lua_setfield ( m_Lua, -2, "regions" );
  
  lua_setglobal( m_Lua, "track" );
}

bool
TrackableLuaObserver::call( const char *funcName, uint64_t timestamp, bool asError )
{
  lua_getglobal( m_Lua, funcName );

  if ( lua_isnil( m_Lua, -1 ) )
  { 
    if ( asError || verbose )
      warning( "TrackableLuaObserver(%s): no function '%s'", name.c_str(), funcName );
    lua_pop( m_Lua, -1 );
    return false;
  }
  else
  {
    if ( timestamp )
      lua_pushinteger( m_Lua, timestamp );

    if (lua_pcall( m_Lua, timestamp!=0, 0, 0) != 0 )
    {
      error( "TrackableLuaObserver(%s): error running function '%s': %s", name.c_str(), funcName, lua_tostring(m_Lua, -1) );
      lua_pop( m_Lua, -1 );
      return false;
    }
  }

  return true;
}

bool
TrackableLuaObserver::call( const char *funcName, ObsvObject *object, uint64_t timestamp, bool asError )
{
  lua_getglobal( m_Lua, funcName );

  if ( lua_isnil( m_Lua, -1 ) )
  { 
    if ( asError || verbose )
      warning( "TrackableLuaObserver(%s): no function '%s'", name.c_str(), funcName );
    lua_pop( m_Lua, -1 );
    return false;
  }
  else
  {
    registerUserDataInstance( m_Lua, object, ObsvObject );
    lua_remove( m_Lua, -2 );
    if ( timestamp )
      lua_pushinteger( m_Lua, timestamp );
    if (lua_pcall( m_Lua, 1+(timestamp!=0), 0, 0) != 0 )
    {
      if ( verbose )
	error( "TrackableLuaObserver(%s): error running function '%s': %s", name.c_str(), funcName, lua_tostring(m_Lua, -1) );
      lua_pop( m_Lua, -1 );
      return false;
    }
  }

  return true;
}


bool
TrackableLuaObserver::call( const char *funcName, ObsvObjects *objects, uint64_t timestamp, bool asError )
{
  lua_getglobal( m_Lua, funcName );

  if ( lua_isnil( m_Lua, -1 ) )
  { 
    if ( asError || verbose )
      warning( "TrackableLuaObserver(%s): no function '%s'", name.c_str(), funcName );
    lua_pop( m_Lua, -1 );
    return false;
  }
  else
  {
    registerUserDataInstance( m_Lua, objects, ObsvObjects );
    lua_remove( m_Lua, -2 );
    if ( timestamp )
      lua_pushinteger( m_Lua, timestamp );
    if (lua_pcall( m_Lua, 1+(timestamp!=0), 0, 0) != 0 )
    {
      if ( verbose )
	error( "TrackableLuaObserver(%s): error running function '%s': %s", name.c_str(), funcName, lua_tostring(m_Lua, -1) );
      lua_pop( m_Lua, -1 );
      return false;
    }
  }

  return true;
}


void
TrackableLuaObserver::initialize()
{
  std::string scriptFileName;
  
  if ( !m_Descr.get( "script", scriptFileName ) || scriptFileName.empty() )
  { error( "TrackableLuaObserver(%s): missing observer script", name.c_str() );
    return;
  }
  
  scriptFileName = configFileName( scriptFileName.c_str() );

  regions.clear();
  for ( int i = rects.numRects()-1; i >= 0; --i )
  {
    std::string &name( rects.rect(i).name );
    TrackableRegion *region = TrackGlobal::regions.get( name.c_str() );
    if ( region != NULL )
      regions.push_back( region );
  }

  openLua();

  std::filesystem::path path( scriptFileName.c_str() );

  if ( !path.empty() )
  { std::string string( "package.path = package.path..\";" );
    string += path;
    string += "?.lua\"";
    luaL_dostring( m_Lua, string.c_str());
  }
  
  if ( luaL_dofile( m_Lua, scriptFileName.c_str()) )
  {
    error( "TrackableLuaObserver(%s): Something went wrong loading the chunk (syntax error?)", name.c_str() );
    error( "   %s", lua_tostring( m_Lua, -1) );
    lua_pop( m_Lua, 1 );
  }

  uint64_t timestamp = this->timestamp;
  call( "init", timestamp );
}

void
TrackableLuaObserver::report()
{
  if ( m_Lua == NULL )
    initialize();

  lua_getglobal( m_Lua, "observe" );
  bool observeDefined = !lua_isnil( m_Lua, -1 );
  lua_pop( m_Lua, 1 );
  
  lua_getglobal( m_Lua, "objectsObserve" );
  bool observeObjectsDefined = !lua_isnil( m_Lua, -1 );
  lua_pop( m_Lua, 1 );
  
  lua_getglobal( m_Lua, "objectObserve" );
  bool observeObjectDefined = !lua_isnil( m_Lua, -1 );
  lua_pop( m_Lua, 1 );
  
  lua_getglobal( m_Lua, "objectEnter" );
  bool enterObjectDefined = !lua_isnil( m_Lua, -1 );
  lua_pop( m_Lua, 1 );
  
  lua_getglobal( m_Lua, "objectMove" );
  bool moveObjectDefined = !lua_isnil( m_Lua, -1 );
  lua_pop( m_Lua, 1 );
  
  lua_getglobal( m_Lua, "objectLeave" );
  bool leaveObjectDefined = !lua_isnil( m_Lua, -1 );
  lua_pop( m_Lua, 1 );
  
  if ( !observeDefined && !observeObjectsDefined && !observeObjectDefined && !enterObjectDefined && !moveObjectDefined && !leaveObjectDefined )
    warning( "TrackableLuaObserver(%s): no function observe(), objectsObserve(), objectObserve(), objectEnter(), objectMove() or objectLeave() defined !!!", name.c_str() );

  if ( enterObjectDefined )
  { for ( int i = rects.numRects()-1; i >= 0; --i )
    { ObsvObjects &objects( rects.rect(i).objects );
      for ( auto &iter : objects )
      { if (iter.second.status == ObsvObject::Enter )
	  call( "objectEnter", &iter.second, timestamp );
      }
    }
  }
  
  if ( observeDefined )
    call( "observe", timestamp );

  if ( observeObjectsDefined )
  { for ( int i = rects.numRects()-1; i >= 0; --i )
      call( "objectsObserve", &rects.rect(i).objects, timestamp );
  }
  
  if ( observeObjectDefined )
  { for ( int i = rects.numRects()-1; i >= 0; --i )
    { ObsvObjects &objects( rects.rect(i).objects );
      for ( auto &iter : objects )
	call( "objectObserve", &iter.second, timestamp );
    }
  }

  if ( moveObjectDefined )
  { for ( int i = rects.numRects()-1; i >= 0; --i )
    { ObsvObjects &objects( rects.rect(i).objects );
      for ( auto &iter : objects )
      { if (iter.second.status == ObsvObject::Move )
	  call( "objectMove", &iter.second, timestamp );
      }
    }
  }

  if ( leaveObjectDefined )
  { for ( int i = rects.numRects()-1; i >= 0; --i )
    { ObsvObjects &objects( rects.rect(i).objects );
      for ( auto &iter : objects )
      { if (iter.second.status == ObsvObject::Leave )
	  call( "objectLeave", &iter.second, timestamp );
      }
    }
  }
}

bool
TrackableLuaObserver::stall( uint64_t timestamp )
{
  if ( timestamp == 0 )
    timestamp = getmsec();
    
  if ( !TrackableFileObserver::stall( timestamp ) )
    return false;
  
  if ( m_Lua == NULL )
    initialize();

  lua_getglobal( m_Lua, "objectsStall" );
  if ( !lua_isnil( m_Lua, -1 ) )
    for ( int i = rects.numRects()-1; i >= 0; --i )
      call( "objectsStall", &rects.rect(i).objects, timestamp );
  lua_pop( m_Lua, 1 );
  
  call( "stall", timestamp );

  return true;
}

bool
TrackableLuaObserver::resume( uint64_t timestamp )
{
  if ( timestamp == 0 )
    timestamp = getmsec();
    
  if ( !TrackableFileObserver::resume( timestamp ) )
    return false;
  
  if ( m_Lua == NULL )
    initialize();

  call( "resume", timestamp );

  lua_getglobal( m_Lua, "objectsResume" );
  if ( !lua_isnil( m_Lua, -1 ) )
    for ( int i = rects.numRects()-1; i >= 0; --i )
      call( "objectsResume", &rects.rect(i).objects, timestamp );
  lua_pop( m_Lua, 1 );
  
  return true;
}

bool
TrackableLuaObserver::start( uint64_t timestamp )
{
  if ( timestamp == 0 )
    timestamp = getmsec();

  m_Descr.setBool( "isStarted", true );

  if ( !TrackableFileObserver::start( timestamp ) )
    return false;
  
  if ( m_Lua == NULL )
    initialize();

  call( "start", timestamp );

  lua_getglobal( m_Lua, "objectsStart" );
  if ( !lua_isnil( m_Lua, -1 ) )
    for ( int i = rects.numRects()-1; i >= 0; --i )
      call( "objectsStart", &rects.rect(i).objects, timestamp );
  lua_pop( m_Lua, 1 );
  
  return true;
}

bool
TrackableLuaObserver::stop( uint64_t timestamp )
{
  if ( timestamp == 0 )
    timestamp = getmsec();
    
  m_Descr.setBool( "isStarted", false );

  if ( !TrackableFileObserver::stop( timestamp ) )
    return false;
  
  if ( m_Lua == NULL )
    initialize();

  lua_getglobal( m_Lua, "objectsStop" );
  if ( !lua_isnil( m_Lua, -1 ) )
    for ( int i = rects.numRects()-1; i >= 0; --i )
      call( "objectsStop", &rects.rect(i).objects, timestamp );
  lua_pop( m_Lua, 1 );
  
  call( "stop", timestamp );

  return true;
}

 
 

#endif // TRACKABLE_LUA_OBSERVER_CPP

#endif // USE_LUA
