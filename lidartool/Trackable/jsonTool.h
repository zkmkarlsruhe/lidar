// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef JSON_TOOL_H
#define JSON_TOOL_H

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <unistd.h>

#include <string.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

namespace rapidjson
{
  extern Document::AllocatorType allocator;

  bool inline writeToFile( rapidjson::Value &json, const char *fileName, int indentCount=2 )
  { 
    std::ofstream outfile( fileName );
    if ( outfile.fail() )
      return false;
    OStreamWrapper osw(outfile);
    PrettyWriter<OStreamWrapper> writer(osw);
    writer.SetIndent( ' ', indentCount );
   
    json.Accept(writer);

    return true;
  }
  
  inline void addMember( Value &root, const char *key, Value json )
  { root.AddMember( StringRef(key), json, rapidjson::allocator );
  }

  inline std::string toString( Value &json, int indentCount=0 )
  { 
    StringBuffer buffer;
    if ( indentCount > 0 )
    { PrettyWriter<StringBuffer> writer(buffer);
      writer.SetIndent( ' ', indentCount );
      json.Accept(writer);
    }
    else
    { Writer<StringBuffer> writer(buffer);
      json.Accept(writer);
    }
    
    return( std::string( buffer.GetString() ) );
  }
  
  inline std::string toStringWithPrecision( Value &json, int precision, int indentCount=0 )
  { 
    StringBuffer buffer;
    if ( indentCount > 0 )
    { PrettyWriter<StringBuffer> writer(buffer);
      writer.SetIndent( ' ', indentCount );
      writer.SetMaxDecimalPlaces( precision );
      json.Accept(writer);
    }
    else
    { Writer<StringBuffer> writer(buffer);
      writer.SetMaxDecimalPlaces( precision );
      json.Accept(writer);
    }
    return( std::string( buffer.GetString() ) );
  }
  
  inline Value toJson( std::string string )
  { return Value(rapidjson::StringRef( string.c_str() ));
  }

  inline void setBool( Value &json, const char *key, bool value )
  { json.AddMember( StringRef(key), Value(value), rapidjson::allocator);
  }
  
  inline void setInt( Value &json, const char *key, int value )
  { json.AddMember( StringRef(key), Value(value), rapidjson::allocator);
  }
  
  inline void setInt64( Value &json, const char *key, int64_t value )
  { json.AddMember( StringRef(key), Value(value), rapidjson::allocator);
  }
  
  inline void setFloat( Value &json, const char *key, float value )
  { json.AddMember( StringRef(key), Value(value), rapidjson::allocator);
  }
  
  inline void setDouble( Value &json, const char *key, double value )
  { json.AddMember( StringRef(key), Value(value), rapidjson::allocator);
  }
  
  inline void setString( Value &json, const char *key, const std::string &string )
  { json.AddMember( StringRef(key), Value(StringRef(string.c_str())), rapidjson::allocator);
  }
  
  inline void setDoubleArray( Value &root, const char *key, double *array, int nelem )
  {
    Value json( rapidjson::kArrayType );
    
    for ( int i = 0; i < nelem; ++i )	
      json.PushBack(array[i], allocator);

    root.AddMember( StringRef(key), json, rapidjson::allocator);
  }

  inline void setAxis( Value &root, const char *key, float matrix[4][4] )
  {
    Value json( rapidjson::kArrayType );
    
    for ( int x = 0; x < 3; ++x )
      for ( int y = 0; y < 3; ++y )
	json.PushBack(matrix[x][y], allocator);

    root.AddMember( StringRef(key), json, rapidjson::allocator);
  }
  
  inline void setMatrix( Value &root, const char *key, float matrix[4][4] )
  {
    Value json( rapidjson::kArrayType );
    
    for ( int i = 0; i < 16; ++i )	
      json.PushBack(matrix[i/4][i%4], allocator);

    root.AddMember( StringRef(key), json, rapidjson::allocator);
  }
  
  inline void setVector3( Value &root, const char *key, double vec[3] )
  {
    Value json( rapidjson::kArrayType );
    
    for ( int i = 0; i < 3; ++i )	
      json.PushBack(vec[i], allocator);

    root.AddMember( StringRef(key), json, rapidjson::allocator);
  }

  inline void setVector2( Value &root, const char *key, double vec[2] )
  {
    Value json( rapidjson::kArrayType );
    
    for ( int i = 0; i < 2; ++i )	
      json.PushBack(vec[i], allocator);

    root.AddMember( StringRef(key), json, rapidjson::allocator);
  }

  inline void dump( Value &json, int indentCount=2 )
  { 
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    writer.SetIndent( ' ', indentCount );
    json.Accept(writer);
    std::cout << buffer.GetString() << std::endl;
  }
  
  inline Value toJson( double vec[3] )
  {
    Value json( rapidjson::kArrayType );
    
    for ( int i = 0; i < 3; ++i )	
      json.PushBack(vec[i], allocator);

    return json;
  }

  inline Value toJson( double *array, int nelem )
  {
    Value json( rapidjson::kArrayType );
    
    for ( int i = 0; i < nelem; ++i )	
      json.PushBack(array[i], allocator);

    return json;
  }

  inline Value toJson( float matrix[4][4] )
  {
    Value json( rapidjson::kArrayType );

    for ( int i = 0; i < 16; ++i )
      json.PushBack(matrix[i/4][i%4], allocator);

    return json;
  }
  
  inline bool fromJson( Value &json, const char *key, bool &value )
  {
    if ( !json.HasMember( key ) )
      return false;
    
    value = json[key].GetBool();

    return true;
  }

  inline bool fromJson( Value &json, const char *key, int &value )
  {
    if ( !json.HasMember( key ) )
      return false;
    
    value = json[key].GetInt();

    return true;
  }

  inline bool fromJson( Value &json, const char *key, uint64_t &value )
  {
    if ( !json.HasMember( key ) )
      return false;
    
    value = json[key].GetUint64();

    return true;
  }

  inline bool fromJson( Value &json, const char *key, float &value )
  {
    if ( !json.HasMember( key ) )
      return false;
    
    value = json[key].GetFloat();

    return true;
  }

  inline bool fromJson( Value &json, const char *key, double &value )
  {
    if ( !json.HasMember( key ) )
      return false;
    
    value = json[key].GetDouble();

    return true;
  }

  inline bool fromJson( Value &json, const char *key, std::string &value )
  {
    if ( !json.HasMember( key ) )
      return false;
    
    value = json[key].GetString();

    return true;
  }

  inline bool fromJson( Value &json, const char *key, double vec[3] )
  {
    if ( !json.HasMember( key ) )
      return false;
    
    Value vec_json = json[key].GetArray();

    for ( int i = 0; i < 3; ++i )
      vec[i] = vec_json[i].GetDouble();

    return true;
  }

  inline bool fromJson( Value &json, const char *key, float matrix[4][4] )
  {
    if ( !json.HasMember( key ) )
      return false;
    
    Value matrix_json = json[key].GetArray();

    for ( int i = 0; i < 16; ++i )
      matrix[i/4][i%4] = matrix_json[i].GetDouble();

    return true;
  }
  
  inline bool axisFromJson( Value &json, const char *key, float matrix[4][4] )
  {
    if ( !json.HasMember( key ) )
      return false;
    
    Value matrix_json = json[key].GetArray();

    for ( int x = 0; x < 3; ++x )
      for ( int y = 0; y < 3; ++y )
	matrix[x][y] = matrix_json[x*3+y].GetDouble();

    return true;
  }
  
  inline bool fromJson( Value &json, const char *key, double *array, int nelem )
  {
    if ( !json.HasMember( key ) )
      return false;
    
    Value array_json = json[key].GetArray();
    for ( int i = 0; i < nelem; ++i )
      array[i] = array_json[i].GetDouble();

    return true;
  }
  
}

#endif // JSON_TOOL_CPP
