// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef KEYVALUE_MAP_H
#define KEYVALUE_MAP_H

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#include "helper.h"
#include "jsonTool.h"

/***************************************************************************
*** 
*** getValue
***
****************************************************************************/

inline bool
getValue( std::map<std::string, std::string> map, const char *key, std::string &value )
{
  std::map<std::string,std::string>::iterator iter( map.find(key) );
    
  if ( iter == map.end() )
    return false;
  
  value = iter->second;

  return true;
}

inline bool
getValue( std::map<std::string, std::string> map, const char *key, int &value )
{
  std::map<std::string,std::string>::iterator iter( map.find(key) );
    
  if ( iter == map.end() || iter->second.empty() )
    return false;

  std::string &v( iter->second );
  unsigned char c = std::tolower( v[0] );
  if ( c == 't' || c == 'f' )
    value = (c == 't');
  else
    value = std::atoi(iter->second.c_str());

  return true;
}

inline bool
getValue( std::map<std::string, std::string> map, const char *key, float &value )
{
  std::map<std::string,std::string>::iterator iter( map.find(key) );
    
  if ( iter == map.end() || iter->second.empty() )
    return false;
  
  value = std::atof(iter->second.c_str());

  return true;
}

inline bool
getValue( std::map<std::string, std::string> map, const char *key, double &value )
{
  std::map<std::string,std::string>::iterator iter( map.find(key) );
    
  if ( iter == map.end() || iter->second.empty() )
    return false;
  
  value = std::atof(iter->second.c_str());

  return true;
}

inline bool
getValue( std::map<std::string, std::string> map, const char *key, bool &value )
{
  std::map<std::string,std::string>::iterator iter( map.find(key) );
    
  if ( iter == map.end() || iter->second.empty() )
    return false;
  
  value = getBool( iter->second.c_str() );

  return true;
}

inline void
setValue( std::map<std::string, std::string> map, const char *key, const char *value )
{
  if ( map.count(key) )
    map[key] = value;
  else
    map.emplace( key, value );
}

inline void
setInt( std::map<std::string, std::string> map, const char *key, int value )
{
  if ( map.count(key) )
    map[key] = std::to_string(value);
  else
    map.emplace( key, std::to_string(value) );
}

inline void
setDouble( std::map<std::string, std::string> map, const char *key, double value )
{
  if ( map.count(key) )
    map[key] = std::to_string(value);
  else
    map.emplace( key, std::to_string(value) );
}

inline void
setBool( std::map<std::string, std::string> map, const char *key, bool value )
{
  if ( map.count(key) )
    map[key] = (value?"true":"false");
  else
    map.emplace( key, (value?"true":"false") );
}

inline bool
parseArg( int &i, const char *argv[], int &argc, std::map<std::string,std::string> &descr ) 
{
  bool success = false;
 
  for ( bool match = true; match && i < argc-1; )
  { match = false;
    if ( *argv[i+1] == ':' )
    { 
      std::string key  ( "filter" );
      std::string value( &argv[++i][1] );
      if ( descr.count(key) )
	descr[key] = value;
      else
	descr.emplace( key, value );
	
      match = success = true;
    }
    else if ( *argv[i+1] == '@' )
    { 
      std::string arg( &argv[++i][1] );
      arg = trim( trim(arg), "{}" );

      std::vector<std::string> keyValue( split( arg, '=', 2 ) );
      if ( keyValue.size() == 2 )
      { const char* qm = "\"";
	std::string key  ( trim( trim( keyValue[0] ), qm ) );
	std::string value( trim( trim( keyValue[1] ), qm ) );	
	if ( descr.count(key) )
	  descr[key] = value;
	else
	  descr.emplace( key, value );
      }

      match = success = true;
    }
  }

  return success;
}

inline std::map<std::string, std::string>
readKeyValuePairs( std::ifstream &stream )
{
  const char* qm = "\"";

  std::map<std::string, std::string> pairs;
  
  std::ostringstream ss;
  ss << stream.rdbuf(); // reading data

  std::vector<std::string> lines( split( ss.str(), '\n' ) );

  for ( int i = 0; i < lines.size(); ++i )
  {
    std::vector<std::string> keyValue( split( lines[i], '=', 2 ) );
    if ( keyValue.size() == 2 )
    { std::string key  ( trim( trim( keyValue[0] ), qm ) );
      std::string value( trim( trim( keyValue[1] ), qm ) );	

      if ( pairs.count(key) )
	pairs[key] = value;
      else
	pairs.emplace( key, value );
    }
  }

  return pairs;
}


/***************************************************************************
*** 
*** KeyValueMap
***
****************************************************************************/

class KeyValueMap : public std::map<std::string,std::string>
{
public:
  typedef std::map<std::string,std::string>::iterator iterator;

  inline bool get( const char *key, bool &value ) const
  { return ::getValue( *this, key, value ); }
  
  inline bool get( const char *key, int &value ) const
  { return ::getValue( *this, key, value ); }
  
  inline bool get( const char *key, float &value ) const
  { return ::getValue( *this, key, value ); }
  
  inline bool get( const char *key, double &value ) const
  { return ::getValue( *this, key, value ); }
  
  inline bool get( const char *key, std::string &value ) const
  { return ::getValue( *this, key, value ); }

  inline void set( const char *key, const char *value )
  { if ( count(key) )
      (*this)[key] = value;
    else
      emplace( key, value );
  }

  inline void setInt( const char *key, int value )
  { if ( count(key) )
      (*this)[key] = std::to_string(value);
    else
      emplace( key, std::to_string(value) );
  }

  inline void setDouble( const char *key, double value )
  { if ( count(key) )
      (*this)[key] = std::to_string(value);
    else
      emplace( key, std::to_string(value) );
  }

  inline void setBool( const char *key, bool value )
  { if ( count(key) )
      (*this)[key] = ( value?"true":"false" );
    else
      emplace( key, value?"true":"false" );
  }

  inline void set(  KeyValueMap &other )
  { for ( KeyValueMap::iterator iter( other.begin() ); iter != other.end(); iter++ )
      set( iter->first.c_str(), iter->second.c_str() );
  }
  
  inline void remove( const char *key )
  { erase( std::string(key) );
  }
  
  inline bool rename( const char *key, const char *newName )
  { auto node = extract(key);
    if (!node.empty())
    { node.key() = newName;
      insert(std::move(node));
      return true;
    }
    return false;
  }
  
  inline void dump( FILE *file, const char *msg="" )
  {
    for ( iterator iter( begin() ); iter != end(); iter++ )
    {
      const std::string   &key( iter->first );
      const std::string &value( iter->second );

      fprintf( stderr, "%s - (%s, %s)\n", msg, key.c_str(), value.c_str() );
    }
  }

};

/***************************************************************************
*** 
*** KeyValueMapDB
***
****************************************************************************/

class KeyValueMapDB : public std::map<std::string,KeyValueMap>
{
public:
  
  typedef std::map<std::string,KeyValueMap>::iterator iterator;

  inline bool get( const char *key, KeyValueMap *&map )
  { std::map<std::string,KeyValueMap>::iterator iter( this->find(key) );
    if ( iter == end() )
      return false;
    map = &iter->second;
    return true;
  }
  
  inline void get( const char *name, KeyValueMap &map )
  { 
    if ( !count(name) )
      emplace( name, KeyValueMap() );
    map = (*this)[name];
  }
  
  inline void set( const char *name, KeyValueMap &map )
  {
    KeyValueMap *m;
    if ( !get( name, m ) )
      m = &emplace( std::make_pair(name,KeyValueMap()) ).first->second;

    m->set( map );
  }
  
  inline void set( const char *name, const char *key, const char *value )
  {
    KeyValueMap *map;
    if ( !get( name, map ) )
      map = &emplace( std::make_pair(name,KeyValueMap()) ).first->second;

    map->set( key, value );
  }
  
  inline void remove( const char *name, const char *key )
  {
    KeyValueMap *map;
    if ( !get( name, map ) )
      return;
    map->erase( std::string(key) );
  }
  
  inline void remove( const char *key )
  {
    erase( std::string(key) );
  }
  
  inline bool rename( const char *key, const char *newName )
  { auto node = extract(key);
    if (!node.empty())
    { node.key() = newName;
      insert(std::move(node));
      return true;
    }
    return false;
  }
};


/***************************************************************************
*** 
*** KeyValueMap IO
***
****************************************************************************/

inline bool 
writeKeyValues( std::map<std::string,std::string> &keyValues, rapidjson::Value &json )
{ 
  if ( keyValues.size() == 1 )
  { json.SetObject();
    std::map<std::string,std::string>::iterator iter( keyValues.begin() );
    rapidjson::setString( json, "key",  iter->first );
    rapidjson::setString( json, "value", iter->second );
  }
  else
  { 
    json.SetArray();
    for ( std::map<std::string,std::string>::iterator iter( keyValues.begin() );
	  iter != keyValues.end();
	  iter++ )
    { 
      rapidjson::Value Json( rapidjson::kObjectType );
      rapidjson::setString( Json, "key",  iter->first );
      rapidjson::setString( Json, "value", iter->second );

      json.PushBack( Json, rapidjson::allocator );
    }
  }

  return true;
}


inline bool 
writeKeyValues( std::map<std::string,std::string> &keyValues, const char *fileName )
{ 
  std::ofstream outfile( fileName );
  if ( outfile.fail() )
    return false;

  rapidjson::OStreamWrapper osw(outfile);
  rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
  rapidjson::Value json;

  if ( !writeKeyValues( keyValues, json ) )
    return false;

  json.Accept(writer);

  return true;
}



inline bool
readKeyValues( std::map<std::string,std::string> &keyValues, rapidjson::Value &json )
{
  if ( json.IsObject() )
  {
    std::string key, value;

    if ( rapidjson::fromJson( json, "value", value ) &&
	 rapidjson::fromJson( json, "key",  key ) )
      if ( keyValues.count( key ) )
	keyValues[key] = value;
      else
	keyValues.emplace( key, value );
    
    return true;
  }

  for ( int i = 0; i < ((int)json.Size()); ++i )
  { rapidjson::Value &valJson( json[i] );
    std::string key, value;
      
    if ( rapidjson::fromJson( valJson, "value", value ) &&
	 rapidjson::fromJson( valJson, "key",  key ) )
      if ( keyValues.count( key ) )
	keyValues[key] = value;
      else
	keyValues.emplace( key, value );
  }

  return true;
}

inline bool
readKeyValuesFromString( std::map<std::string,std::string> &keyValues, const char *string )
{
  rapidjson::Document json;
  if ( json.Parse( string ).HasParseError() )
  { std::cout << "error parsing document\n";
    return false;
  }

  return readKeyValues( keyValues, json );
}

inline bool
readKeyValues( std::map<std::string,std::string> &keyValues, const char *fileName )
{
  std::string fn( fileName );
  std::ifstream stream( fn );

  if ( !stream.is_open()  )
    return false;

  std::ostringstream ss;
  ss << stream.rdbuf(); // reading data

  readKeyValuesFromString( keyValues, ss.str().c_str() );

  return true;
}

inline bool 
writeKeyValueMapDB( KeyValueMapDB &db, const char *fileName, const char *key="name", const char *map="map" )
{ 
  std::ofstream outfile( fileName );
  if ( outfile.fail() )
    return false;

  rapidjson::OStreamWrapper osw(outfile);
  rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
  rapidjson::Value json( rapidjson::kArrayType );;

  for ( KeyValueMapDB::iterator iter( db.begin() ); iter != db.end(); iter++ )
  { 
    rapidjson::Value jsonMap( rapidjson::kObjectType );;
    rapidjson::setString( jsonMap, key,  iter->first );
    
    rapidjson::Value Json;
    if ( !writeKeyValues( iter->second, Json ) )
      return false;
    
    jsonMap.AddMember( rapidjson::StringRef(map), Json, rapidjson::allocator );
    
    json.PushBack( jsonMap, rapidjson::allocator );
  }

  json.Accept(writer);

  return true;
}

inline bool
readKeyValueMapDB( KeyValueMapDB &db, const char *fileName, const char *key="name", const char *mapName="map" )
{
  std::string fn( fileName );
  std::ifstream stream( fn );

  if ( !stream.is_open()  )
    return false;

  std::ostringstream ss;
  ss << stream.rdbuf(); // reading data

  rapidjson::Document json;
  if ( json.Parse( ss.str().c_str() ).HasParseError() )
  { std::cout << "error parsing document\n";
    return false;
  }

  for ( int i = ((int)json.Size())-1; i >= 0; --i )
  { rapidjson::Value &valJson( json[i] );
    std::string name, map;

    if ( rapidjson::fromJson( valJson, key, name  ) && valJson.HasMember(key) )
    { rapidjson::Value &jsonMap( valJson[mapName] );
      if ( jsonMap.IsObject() || jsonMap.IsArray() )
      { KeyValueMap map;
	readKeyValues( map, jsonMap );
	db.set( name.c_str(), map );
      }
    }
  }

  return true;
}

#endif
