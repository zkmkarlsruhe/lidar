// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef _PV_HELPER_H
#define _PV_HELPER_H

#ifdef _WIN32
#include <io.h> 
#define access    _access_s
#else
#if !__APPLE__
   #define __LINUX__ 1
#endif
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h> /* exit() */

#include <chrono>
#include <unistd.h>

#include <math.h>

#include <fstream>
#include <sstream>

#include <string.h>
#include <assert.h>
#include <regex>
#include <map>

/***************************************************************************
*** 
*** pv::Helper
***
****************************************************************************/

  
inline double 
mix( double x, double a, double b )
{ return( (1-x) * a + x * b ); 
}


inline uint64_t
getmsec() 
{ return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline uint64_t
getnsec() 
{ return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline bool
fileExists( const std::string &Filename )
{
  return access( Filename.c_str(), 0 ) == 0;
}

inline std::string timestampString( const char *templat, uint64_t timestamp, bool addParanthesis=true )
{
  if ( templat == NULL || strchr( templat, '\%' ) == NULL )
    return std::to_string( timestamp );
     
  time_t t = timestamp / 1000;
  struct tm timeinfo = *localtime( &t );

  const int maxLen = 2000;
  char buffer[maxLen+1];
  strftime( buffer, maxLen, templat, &timeinfo );
  
  std::string result;

  if ( addParanthesis )
    result += "\"";
  result += buffer;
  if ( addParanthesis )
    result += "\"";
  
  return result;
}

inline bool
replace( std::string &str, const std::string &from, const std::string &to )
{
  if ( from == to )
    return false;

  size_t start_pos = str.find( from );
  if ( start_pos == std::string::npos )
    return false;

  str.replace( start_pos, from.length(), to );

  replace( str, from, to );

  return true;
}

inline std::vector<std::string>
split(const std::string &s, char delim, int num=-1 )
{
  std::vector<std::string> result;
  std::stringstream ss (s);
  std::string item;

  while ( (num < 0 || --num > 0) && getline (ss, item, delim))
    result.push_back (item);
  
  while ( num == 0 && getline (ss, item))
    result.push_back (item);
 
  return result;
}


inline std::string &
rtrim(std::string& s, const char* t=NULL)
{
  if ( t == NULL )
    t = " \t\n\r\f\v";
  s.erase(s.find_last_not_of(t) + 1);
  return s;
}

inline std::string &
ltrim(std::string& s, const char* t=NULL)
{
  if ( t == NULL )
    t = " \t\n\r\f\v";
  s.erase(0, s.find_first_not_of(t));
  return s;
}

inline std::string &
trim(std::string& s, const char* t=NULL)
{
  return ltrim(rtrim(s, t), t);
}
inline bool
startsWith( const std::string& s, const std::string &t )
{
  return strncmp( s.c_str(), t.c_str(), t.size() ) == 0;
}

inline bool
endsWith(std::string str, std::string suffix)
{ return str.find(suffix, str.size() - suffix.size()) != std::string::npos;
}

inline bool
startsWithCaseInsensitive(const std::string& value, const std::string& starting)
{
  if (starting.size() > value.size())
    return false;

  return std::equal(starting.begin(), starting.end(), value.begin(),
		    [](const unsigned char a, const unsigned char b) {
		      return std::tolower(a) == std::tolower(b);
		    }
		    );
}

inline bool
endsWithCaseInsensitive(const std::string& value, const std::string& ending)
{
  if (ending.size() > value.size())
    return false;

  return std::equal(ending.crbegin(), ending.crend(), value.crbegin(),
		    [](const unsigned char a, const unsigned char b) {
		      return std::tolower(a) == std::tolower(b);
		    }
		    );
}

inline void tolower( std::string& s )
{ std::for_each(s.begin(), s.end(), [](char & c) { c = ::tolower(c); });
}

inline bool 
getBool( const char *stringValue, bool &value )
{
  std::string string( stringValue );
  std::for_each(string.begin(), string.end(), [](char & c) { c = ::tolower(c); });
  
  if ( string == "true" || string == "yes" || string == "1" )
  { value = true;
    return true;
  }
  else if ( string == "false" || string == "no" || string == "0" )
  { value = false;
    return true;
  }

  return false;
}

inline bool 
getValue( const char *stringValue, float &value )
{
  value = std::atof( stringValue );
  return true;
}

inline bool 
getBool( const char *stringValue )
{
  bool value;
  if ( getBool( stringValue, value ) )
    return value;
  
  return false;
}

#ifndef PATHSEPARATOR
#define PATHSEPARATOR
#ifdef _WIN32
#define pathSeparator '\\'
#else
#define pathSeparator '/'
#endif
#endif

inline std::string
filePath( std::string s )
{
  int length = s.length();
  if ( length == 0 )
    return s;
  
  size_t i = s.rfind( pathSeparator, length );

  if ( i == std::string::npos )
    return "";

  std::string result( s.substr( 0, i+1 ) );
  
#ifdef _WIN32
  if ( result == ".\\" )
#else
  if ( result == "./" )
#endif
    return "";

  s = result;

  length = s.length();

  if ( length > 0 && s[length-1] != pathSeparator )
    s = s + pathSeparator;
  
  return s;
}

#endif // 
