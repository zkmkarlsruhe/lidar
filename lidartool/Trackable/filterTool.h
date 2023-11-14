// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef FILTER_TOOL_H
#define FILTER_TOOL_H

#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <unistd.h>

#include <string.h>
#include <assert.h>
#include <regex>
#include <helper.h>

#include <map>

namespace Filter  {

enum FilterFlag : uint64_t  {
  FRAME		= (1ULL<<0),
  FRAME_ID	= (1ULL<<1),
  FRAME_END	= (1ULL<<2),
  TIMESTAMP	= (1ULL<<3),

  ID	   	= (1ULL<<4),
  NUM_OBJECTS   = (1ULL<<5),
  OBJECTS   	= (1ULL<<6),
  OBJECT    	= (1ULL<<7),
  POSITION  	= (1ULL<<8),
  POS_X	   	= (1ULL<<9),
  POS_Y	   	= (1ULL<<10),
  POS_Z	   	= (1ULL<<11),
  SIZE	   	= (1ULL<<12)
};

static inline const char *Frame       = "frame";
static inline const char *FrameId     = "frame_id";
static inline const char *FrameEnd    = "frame_end";
static inline const char *Timestamp   = "timestamp";
 
static inline const char *NumObjects  = "num_objects";
static inline const char *Objects     = "objects";
static inline const char *Object      = "object";
static inline const char *Id 	       = "id";
static inline const char *Position    = "position";
static inline const char *PosX        = "x";
static inline const char *PosY        = "y";
static inline const char *PosZ        = "z";
static inline const char *Size        = "size";

class Filter
{
public:

  std::map<std::string, std::string> KeyMap;
  std::map<std::string, FilterFlag>  FlagMap;
  std::map<std::string, std::string> PersistentMap; // rapidjson needs persistent c pointers 
  uint64_t 					 filter;

  std::string					 object_id;
  bool						 initialized;
  
  Filter()
    : KeyMap(),
      FlagMap(),
      PersistentMap(),
      filter( 0 )
  { 
    initialized = false;
  }

  void addFilter( uint64_t flag, const char *name )
  {
    KeyMap.emplace (name, name);
    FlagMap.emplace(name, (FilterFlag)flag);
    initialized = true;
  }

  virtual std::string km( std::string key, std::string label="", uint64_t frame_count=0, uint64_t timestamp=0, int id=0 )
  {
    std::map<std::string,std::string>::iterator iter( KeyMap.find(key) );
    if ( iter == KeyMap.end() )
      return key;

    std::string result = iter->second;

    result = std::regex_replace(result, std::regex("%frame_id"),  std::to_string(frame_count) );
    result = std::regex_replace(result, std::regex("%timestamp"), std::to_string(timestamp)  );

    if ( !object_id.empty() )
      result = std::regex_replace(result, std::regex(object_id), std::to_string(id) );

    return result;
  }

  const char *kmc( std::string key, std::string label="", uint64_t frame_count=0, uint64_t timestamp=0, int id=0 )
  {
    std::string result = km( key, label, frame_count, timestamp, id );

    std::map<std::string,std::string>::iterator iter( PersistentMap.find(result) );
    
    if ( iter == PersistentMap.end() )
    { PersistentMap.emplace( result, result );
      iter = PersistentMap.find(result);
    }

    return iter->second.c_str();
  }

  std::string kmprefix( const char *prefix, std::string key )
  {
    return std::string(prefix) + km( key );
  }

  bool	filterEnabled( uint64_t filt ) const 
  {    
    return ( filter == 0 || (filter&filt) );
  }

  void printFilterHelp( const char *arg ) const
  {
    for (auto& it: KeyMap)
      printf( "	%s\n", it.first.c_str() );
  }

  void parseFilter( const char *filter )
  {
    this->filter = 0;
    
    std::stringstream s_stream(filter); //create string stream from the string
    while(s_stream.good()) {
      std::string substr;
      getline(s_stream, substr, ','); //get first string delimited by comma

//		    std::cout << substr  << std::endl;

      std::vector<std::string> pair( split( substr, '=', 2 ) );
      std::string key = pair[0];
	    
      if ( FlagMap.find(key) == FlagMap.end() )
      { std::cerr << "unknown filter type " << key << "\n";
	exit( 2 );
      }

      if ( pair.size() > 1 )
	KeyMap.find(key)->second = pair[1];
      else if ( substr.find("=") != std::string::npos )
	KeyMap.find(key)->second = "";
	    
      this->filter |= FlagMap.find(key)->second;
    }
  }
  
  bool isInitialized() const
  { 
    return initialized;
  }
  

};


} // namespace Filter 

#endif // FILTER_TOOL_H
