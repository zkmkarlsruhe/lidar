// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#include "lidarAdmin.h"

#include <curl/curl.h>
#include <httpserver/string_utilities.hpp>
#include <httpserver.hpp>

#include <limits.h>
#include <dirent.h>
#include <filesystem>

#include <cmath>
#include <iostream>
#include <fstream>
#include <mutex>

/***************************************************************************
*** 
*** DEFINES
***
****************************************************************************/

using namespace httpserver;
using namespace httpserver::string_utilities;

/***************************************************************************
*** 
*** GLOBALS
***
****************************************************************************/

static int 	   admin_port       = 8000;
static int 	   webserver_port   = 8080;
static int 	   hub_port         = 0;
static webserver  *webserv 	    = NULL;
static CURL 	  *curl;
static bool 	   verbose          = false;
static bool 	   isExpert         = false;
static bool 	   g_StartServer    = false;

static std::string g_Conf;
static std::string g_RunningMode;
static std::string g_InstallDir( "./" );
static std::string g_RealInstallDir( "./" );
static std::string g_HTMLDir( "./html/" );
static std::vector<std::string> g_FileSizeDirs = { "/" };

static std::mutex  webMutex;

/***************************************************************************
*** 
*** HELPER
***
****************************************************************************/

inline bool
replace( std::string &str, const std::string &from, const std::string &to )
{
  size_t start_pos = str.find( from );
  if ( start_pos == std::string::npos )
    return false;

  str.replace( start_pos, from.length(), to );

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

//////////////////////////
//// decodeURIComponent and encodeURIComponent from:
//// https://gist.github.com/arthurafarias/56fec2cd49a32f374c02d1df2b6c350f
/////////////////////////

static std::string
decodeURIComponent( std::string encoded )
{
  std::string decoded = encoded;
  std::smatch sm;
  std::string haystack;

  int dynamicLength = decoded.size() - 2;

  if (decoded.size() < 3) return decoded;

  for (int i = 0; i < dynamicLength; i++)
  {
    haystack = decoded.substr(i, 3);

    if (std::regex_match(haystack, sm, std::regex("%[0-9A-F]{2}")))
    {
      haystack = haystack.replace(0, 1, "0x");
      std::string rc = {(char)std::stoi(haystack, nullptr, 16)};
      decoded = decoded.replace(decoded.begin() + i, decoded.begin() + i + 3, rc);
    }

    dynamicLength = decoded.size() - 2;
  }

  return decoded;
}

static std::string
encodeURIComponent( std::string decoded )
{
  std::ostringstream oss;
  std::regex r("[!'\\(\\)*-.0-9A-Za-z_~]");

  for (char &c : decoded)
  {
    if (std::regex_match((std::string){c}, r))
    {
      oss << c;
    }
    else
      {
	oss << "%" << std::uppercase << std::hex << (0xff & c);
      }
  }
  return oss.str();
}


inline bool
fileExists( const std::string &Filename )
{
  return access( Filename.c_str(), 0 ) == 0;
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

static std::string
humanReadable( std::uintmax_t size )
{
  int i = 0;
  double mantissa = size;
  for (; mantissa >= 1024.; mantissa /= 1024., ++i)
  {}
  
  char buf[100];

  if ( mantissa < 100 )
  { int m = std::ceil(mantissa * 10.);
    sprintf( buf, "%d,%d ", m / 10, m % 10 );
  }
  else
    sprintf( buf, "%d ", (int)std::round(mantissa) );

  std::string result( buf );
  result += "BKMGTPE"[i];
      
  return i == 0 ? result : result + "B";
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

#ifdef _WIN32
static  char sep = '\\';
#else
static  char sep = '/';
#endif

static std::string
getPathName( std::string s )
{
  int length = s.length();
  if ( length == 0 )
    return s;
  
  size_t i = s.rfind( sep, length );

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

  if ( length > 0 && s[length-1] != sep )
    s = s + sep;
  
  return s;
}

static bool
valueFromConfigFile( std::string &match )
{
  std::string fileName( "config.txt" );

  std::ifstream stream( fileName );
  if ( !stream.is_open() )
    return false;

  std::string line;
  while (std::getline(stream, line ))
  {
    line = trim( line );

    std::vector<std::string> pair( split(line,'=') );

    if ( pair.size() >= 1 )
    { std::string key( pair[0] );
      key = trim( key );
      if ( key == match )
      { 
	if ( pair.size() == 1 )
        { match = "";
	  return true;
	}
	
	std::string value( pair[1] );
	value = trim( value );
	pair = split( value, '#' );
	value = pair[0];
	match = trim( value );
	return true;
      }
    }
  }

  return false;
}

  
static void
setInstallDir( const char *executable )
{
  g_InstallDir = getPathName( executable );

#ifdef PATH_MAX
  const int path_max = PATH_MAX;
#else
  int path_max = pathconf(path, _PC_PATH_MAX);
  if (path_max <= 0)
    path_max = 4096;
#endif
  
  char buf[path_max];
  char *res = realpath(executable, buf);
  if ( res != NULL )
  {
    g_RealInstallDir = getPathName( buf );
    g_HTMLDir        = g_RealInstallDir + "html/";
  }
}


static size_t
WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


static std::string
exec( std::string cmd, bool verbose=false )
{
  std::string result;
  FILE * stream;
  const int max_buffer = 256;
  char buffer[max_buffer];
  cmd.append(" 2>&1");

  if ( verbose )
    printf( "EXEC: '%s'\n", cmd.c_str() );

  stream = popen(cmd.c_str(), "r");
  if (stream) {
    while (!feof(stream))
      if (fgets(buffer, max_buffer, stream) != NULL)
	result.append(buffer);
    pclose(stream);
  }

  rtrim( result );
  
  return result;
}

static std::string 
startServerCmd( bool noSensors=false )
{
  std::ifstream stream( "LidarRunMode.txt" );
  
  std::string cmd;
      
  g_RunningMode = "";
  if ( stream.is_open() )
  { stream >> g_RunningMode;
    rtrim( g_RunningMode );
  }

  cmd = "./StartServer.sh";
  if ( verbose )
    cmd += " +v";

  if ( isExpert )
    cmd += " +expert";
	
  if ( noSensors )
    cmd += " -sensors";
  cmd += " &";

  return cmd;
}


static bool 
getBoolArg( const http_request& req, const char *label, bool &value )
{
  std::string string = req.get_arg( label );
  
  if ( string.empty() )
    return false;

  getBool( string.c_str(), value );

  return true;
}

static bool 
getIntArg( const http_request& req, const char *label, int &value )
{
  std::string string = req.get_arg( label );

  if ( string.empty() )
    return false;

  value = std::stoi( string );

  return true;
}

static bool 
getStringArg( const http_request& req, const char *label, std::string &value )
{
  std::string string = req.get_arg( label );

  if ( string.empty() )
    return false;

  value = string;

  value = decodeURIComponent( value );

  return true;
}


/***************************************************************************
*** 
*** HTML
***
****************************************************************************/

static std::shared_ptr<http_response> 
stringResponse( const char *string, const char *mimeType="text/plain", int errorCode=200 )
{
  std::shared_ptr<http_response> response = std::shared_ptr<http_response>(new string_response(string,errorCode,mimeType));
  response->with_header("Access-Control-Allow-Origin", "*");
  return response;
}

static std::shared_ptr<http_response> 
htmlResponse( std::string string )
{
  std::shared_ptr<http_response> response = std::shared_ptr<http_response>(new string_response(string,200,"text/html"));
  response->with_header("Access-Control-Allow-Origin", "*");
  return response;
}

static std::shared_ptr<http_response> 
jsonResponse( std::string string )
{
  std::shared_ptr<http_response> response = std::shared_ptr<http_response>(new string_response(string,200,"application/json"));
  response->with_header("Access-Control-Allow-Origin", "*");
  return response;
}

static std::shared_ptr<http_response> 
fileResponse( std::string path, const char *mimeType, int errorCode=200 )
{
  std::shared_ptr<http_response> response = std::shared_ptr<http_response>(new file_response(path,errorCode,mimeType));
  response->with_header("Access-Control-Allow-Origin", "*");
  return response;
}


class unused_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

      bool run      = false;
      bool kill     = false;
      bool start    = false;
      bool stop     = false;
      
      if ( getBoolArg( req, "run",   run ) ||
	   getBoolArg( req, "kill",  kill ) ||
	   getBoolArg( req, "start", start ) ||
	   getBoolArg( req, "stop",  stop ) )
      {
	std::string cmd( "./manageNodes.sh unused " );
	if ( run )
	  cmd += "run";
	else if ( start )
	  cmd += "start";
	else if ( stop )
	  cmd += "stop";
	else 
	  cmd += "kill";
	cmd += " &";

	if ( verbose )
	  printf( "COM: '%s'\n", cmd.c_str() );
	system( cmd.c_str() );
      }
      
      return stringResponse( "Unused" );
    }
};


class set_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

      std::string runMode, hubMode;

      webMutex.lock();
      
      if ( getStringArg( req, "runMode",  runMode ) )
      {
	std::ofstream stream( "LidarRunMode.txt"  );
  
	if ( stream.is_open() )
        {
	  if ( verbose )
	    printf( "Writing %s to LidarRunMode.txt\n", runMode.c_str() );
	  
	  stream << runMode;
	}
      }
     
      if ( getStringArg( req, "hubMode",  hubMode ) )
      {
	std::ofstream stream( "LidarHubMode.txt"  );
  
	if ( stream.is_open() )
        {
	  if ( verbose )
	    printf( "Writing %s to LidarHubMode.txt\n", hubMode.c_str() );
	  
	  stream << hubMode;
	}
      }
     
      webMutex.unlock();
      
      return stringResponse( "Set" );
    }
};


class get_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

 
      std::string region, group;
      
      std::string json = "{";

      bool runMode         = false;
      bool runningMode     = false;
      bool hubMode         = false;
      bool hasHUB          = false;
      bool useNodes        = false;
      bool hasNodes        = false;
      bool hasSensors      = false;
      bool hasSimulation   = false;
      bool sensorsChanged  = false;
      bool confName        = false;
      bool first           = true;
      
      webMutex.lock();
      
      getBoolArg( req, "runMode",        runMode );
      getBoolArg( req, "sensorsChanged", sensorsChanged );
      
      std::string mode( "setup" );

      if ( runMode || sensorsChanged )
      {
	std::ifstream stream( "LidarRunMode.txt" );
	if ( stream.is_open() )
	{ stream >> mode;
	  rtrim( mode );
	}
      }
	
      if ( runMode )
      {
	if ( first )
	  first = false;
	else
	  json += ", ";
	    
	json += "\"runMode\": \"";
	json += mode;
	json += "\"";
      }

      if ( sensorsChanged )
      {
	bool changed = false;
	
	std::string nikNames( g_Conf );
	nikNames += "/nikNames";
	if ( mode == "simulation" )
	  nikNames += "SimulationMode";
	nikNames += ".json";

	std::string sensorDB( "sensorDB.txt" );

	if ( fileExists( sensorDB.c_str() ) )
        { changed = true;
	  if ( fileExists( nikNames.c_str() ) && fileExists( sensorDB.c_str() ) )
	  { std::filesystem::file_time_type nikNamesTime = std::filesystem::last_write_time( nikNames );
	    std::filesystem::file_time_type sensorDBTime = std::filesystem::last_write_time( sensorDB );
	    changed = (sensorDBTime >= nikNamesTime);
	  }
	}
	
	if ( first )
	  first = false;
	else
	  json += ", ";
	    
	json += "\"sensorsChanged\": ";
	json += (changed ? "true":"false");
      }

      if ( getBoolArg( req, "runningMode", runningMode ) )
      {
	if ( first )
	  first = false;
	else
	  json += ", ";
	    
	json += "\"runningMode\": \"";
	json += g_RunningMode;
	json += "\"";
      }

      if ( getBoolArg( req, "confName", confName ) )
      {
	if ( first )
	  first = false;
	else
	  json += ", ";
	    
	json += "\"confName\": \"";
	json += g_Conf;
	json += "\"";
      }

      if ( getBoolArg( req, "hubMode", hubMode ) )
      {
	std::ifstream stream( "LidarHubMode.txt" );
	if ( stream.is_open() )
	{
	  std::string mode;
	  stream >> mode;
	  rtrim( mode );

	  if ( first )
	    first = false;
	  else
	    json += ", ";
	    
	  json += "\"hubMode\": \"";
	  json += mode;
	  json += "\"";
	}
      }

      if ( getBoolArg( req, "hasHUB",  hasHUB ) )
      {
	if ( first )
	  first = false;
	else
	  json += ",";
	    
	json += "\"hasHUB\": ";
	json += (hub_port > 0 ? "true" : "false");
      }

      if ( getBoolArg( req, "useNodes", useNodes ) )
      {
	if ( first )
	  first = false;
	else
	  json += ",";
	    
	useNodes = false;
	std::string match( "useNodes" );
	if ( valueFromConfigFile( match ) )
	  getBool( match.c_str(), useNodes );

	json += "\"useNodes\": ";
	json += (useNodes ? "true" : "false");
      }

      if ( getBoolArg( req, "hasNodes", hasNodes ) )
      {
	if ( first )
	  first = false;
	else
	  json += ",";
	    
	std::string cmd( "./manageNodes.sh hasNodes" );
	std::string result = exec( cmd, false );

	json += "\"hasNodes\": ";
	json += (std::atoi(result.c_str()) > 0 ? "true" : "false");
      }

      if ( getBoolArg( req, "hasSensors", hasSensors ) )
      {
	if ( first )
	  first = false;
	else
	  json += ",";
	    
	hasSensors = false;
	std::string match( "useNodes" );
	if ( valueFromConfigFile( match ) )
	  getBool( match.c_str(), hasSensors );

	if ( hasSensors )
        {
	  std::string cmd( "./manageSensors.sh hasSensors" );
	  std::string result = exec( cmd, false );
	  hasSensors = (std::atoi(result.c_str()) > 0);
	}
	
	json += "\"hasSensors\": ";
	json += (hasSensors ? "true" : "false");
      }

      if ( getBoolArg( req, "hasSimulation", hasSimulation ) )
      {
	if ( first )
	  first = false;
	else
	  json += ",";
	    
	hasSimulation = false;

	std::string blueprintSimulationFile( "blueprintSimulationFile" );
	if ( valueFromConfigFile( blueprintSimulationFile ) && !blueprintSimulationFile.empty() )
	  hasSimulation = true;

	std::string blueprintObstacleImageFile( "blueprintObstacleImageFile" );
	if ( valueFromConfigFile( blueprintObstacleImageFile ) && !blueprintObstacleImageFile.empty() )
	  hasSimulation = true;

	json += "\"hasSimulation\": ";
	json += (hasSimulation ? "true" : "false");
      }

      webMutex.unlock();

      json += " }";
      
//      printf( "json: '%s'\n", json.c_str() );

      return jsonResponse( json );
    }
};

class nodes_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

      bool yes      = false;
      bool run      = false;
      bool kill     = false;
      bool rerun    = false;
      bool reboot   = false;
      bool shutdown = false;
      bool setup    = false;
      bool enablePower = false;
      std::string entry;
      
      if ( getBoolArg( req, "run",   run ) ||
	   getBoolArg( req, "kill",  kill ) ||
	   getBoolArg( req, "rerun", rerun ) )
      {
	std::string nodeId, ip;
	std::string cmd( "./manageNodes.sh" );
	if ( getStringArg( req, "nodeId",  nodeId ) )
	  cmd += " node " + nodeId;
	if ( getStringArg( req, "ip",  ip ) )
	  cmd += " ip " + ip;
	if ( run )
	  cmd += " run";
	else if ( kill )
	  cmd += " kill";
	else 
	  cmd += " rerun";
	cmd += " &";

	if ( verbose )
	  printf( "COM: '%s'\n", cmd.c_str() );
	system( cmd.c_str() );
      }
      else if ( getBoolArg( req, "reboot", reboot ) || getBoolArg( req, "shutdown", shutdown ) || getBoolArg( req, "setup", setup ) || getBoolArg( req, "enablePower", enablePower ) )
      {
	std::string nodeId, ip;
	std::string cmd( "./manageNodes.sh" );
	if ( getStringArg( req, "nodeId",  nodeId ) )
	  cmd += " node " + nodeId;
	if ( getStringArg( req, "ip",  ip ) )
	  cmd += " ip " + ip;

	if ( reboot )
	  cmd += " reboot";
	else if ( shutdown )
	  cmd += " shutdown";
	else if ( getBoolArg( req, "enablePower", enablePower ) )
        {
	  cmd += " enablePower ";
	  cmd += (enablePower?"true":"false");
	}
	else 
	  cmd += " setup";

//	cmd += " &";

	if ( verbose )
	  printf( "COM: '%s'\n", cmd.c_str() );
	system( cmd.c_str() );
	if ( verbose )
	  printf( "RET: %s\n", cmd.c_str() );
      }
      else if ( getBoolArg( req, "enable", yes ) )
      {
	std::string name;

	if ( getStringArg( req, "name",  name ) )
	{
	  std::string cmd( "./manageSensors.sh +q " );
	  cmd += (yes ? "enable " : "disable ");
	  cmd += name;
	  cmd += " 2> /dev/zero &";
	  if ( verbose )
	    printf( "COM: '%s'\n", cmd.c_str() );
	  webMutex.lock();
	  system( cmd.c_str() );
	  webMutex.unlock();
	  if ( verbose )
	    printf( "RET: %s\n", cmd.c_str() );
	}
      }
      else if ( getBoolArg( req, "setNodeId", yes ) )
      {
	std::string nodeId, ip;
	std::string cmd( "./manageNodes.sh" );
	if ( getStringArg( req, "nodeId",  nodeId ) && getStringArg( req, "ip",  ip ) )
	{
	  cmd += " setNodeId";
	  cmd += " " + ip;
	  cmd += " " + nodeId;
	  cmd += "  &";
	  if ( verbose )
	    printf( "COM: '%s'\n", cmd.c_str() );
	  webMutex.lock();
	  system( cmd.c_str() );
	  webMutex.unlock();
	  if ( verbose )
	    printf( "RET: %s\n", cmd.c_str() );
	}
      }
      else if ( getBoolArg( req, "remove", yes ) )
      {
	std::string mac;
	std::string cmd( "./manageNodes.sh" );
	if ( getStringArg( req, "mac",  mac ) )
	{
	  cmd += " remove";
	  cmd += " " + mac;
	  if ( verbose )
	    printf( "COM: '%s'\n", cmd.c_str() );
	  webMutex.lock();
	  system( cmd.c_str() );
	  webMutex.unlock();
	  if ( verbose )
	    printf( "RET: %s\n", cmd.c_str() );
	}
      }
      else if ( getStringArg( req, "register", entry ) )
      {
	std::string cmd( "./manageNodes.sh" );

//	std::string requestor( req.get_requestor() );
//	printf( "requestor: %s\n", requestor.c_str() );

	cmd += " register \"" + entry + "\" ";
	webMutex.lock();
	std::string result = exec( cmd, verbose );
	webMutex.unlock();

	if ( verbose )
	  printf( "register result: '%s'\n", result.c_str() );
	return stringResponse( result.c_str() );
      }
       
      return stringResponse( "nodes" );
    }
};

static bool
compareLines(const std::string &s1, const std::string& s2)
{ 
  std::vector<std::string> f1( string_split(s1,' ') );
  std::vector<std::string> f2( string_split(s2,' ') );
  
  if ( f1.size() < 4 )
  { if ( f2.size() < 4 )
      return f1[1] < f2[1];

    return true;
  }

  if ( f2.size() < 4 )
    return true;

  if ( startsWithCaseInsensitive( f1[0], "up" ) && startsWithCaseInsensitive( f2[0], "down" ) )
    return false;
  
  if ( startsWithCaseInsensitive( f2[0], "up" ) && startsWithCaseInsensitive( f1[0], "down" ) )
    return true;

  if ( f1[3] == f2[3] )
  {
    if ( f1[2] == f2[2] )
      return f1[1] < f2[1];
    
    return f1[2] < f2[2];
  }
  
  return std::atoi( f1[3].c_str() ) < std::atoi( f2[3].c_str() );
}

static void
addNodeAction( std::string &result, std::string &ip, std::string &HWAddr )
{
  result +="<div class=\"dropdown\">"
    "<button class=\"btn btn-secondary dropdown-toggle btn-list\" type=\"button\" id=\"dropdownMenuButton-"	+ HWAddr +"\" data-toggle=\"dropdown\" aria-haspopup=\"true\" aria-expanded=\"false\">"
"Action"
"  <span class=\"caret\"></span></button>"
"  <ul class=\"dropdown-menu action-item\" aria-labelledby=\"dropdownMenuButton-"
    + HWAddr +"\">"
"    <li><a class=\"dropdown-item action-item btn-setup\" name=\""+ip+"\" href=\"#\">Setup</a></li>"
"    <li><a class=\"dropdown-item action-item btn-reboot\" name=\""+ip+"\" href=\"#\">Reboot</a></li>"
"    <li><a class=\"dropdown-item action-item btn-shutdown\" name=\""+ip+"\" href=\"#\">Shutdown</a></li>"
"  </ul>"
"</div>";

}

static void
addEntryMenu( std::string &result, std::string &HWAddr )
{
  result +="<div class=\"dropdown\">"
    "<button class=\"btn btn-secondary dropdown-toggle btn-list\" type=\"button\" id=\"entry-"	+ HWAddr +"\" data-toggle=\"dropdown\" aria-haspopup=\"true\" aria-expanded=\"false\">"
"  <span class=\"caret\"></span></button>"
"  <ul class=\"dropdown-menu action-item\" aria-labelledby=\"entry-"
    + HWAddr +"\">"
"    <li><a class=\"dropdown-item btn-list-menu-item btn-remove\" name=\""+HWAddr+"\" href=\"#\">Remove Entry</a></li>"
"  </ul>"
"</div>";
}

static void
addSINMenu( std::string &result, std::string &sin, std::string &ip )
{
  result +="<div class=\"dropdown\">"
    "<button class=\"btn btn-secondary dropdown-toggle btn-sin\" type=\"button\" id=\"sin-"	+ ip +"\" data-toggle=\"dropdown\" aria-haspopup=\"true\" aria-expanded=\"false\"style=\"background-color: #ffffff00; padding-right: 0p; padding-left: 0px; border-spacing: 0px;\">"
+ sin +
"  <span class=\"caret\"></span></button>"
"  <ul dropleft class=\"dropdown-menu action-item\" aria-labelledby=\"sin-"
    + ip +"\">"
"    <li><a class=\"dropdown-item btn-list-menu-item btn-edit-sin\" id=\""+ip+":"+std::to_string(webserver_port)+"/settings\" name=\""+ip+"\" href=\"#\">Edit SIN</a></li>"
"  </ul>"
"</div>";
}

static std::string
sensorFileName()
{
  bool useNodes = false;
  std::string match( "useNodes" );
  if ( valueFromConfigFile( match ) )
    getBool( match.c_str(), useNodes );

  std::string fileName( useNodes ? "sensorDB.txt" : "LidarSensors.txt" );

  return fileName;
}

class sensorDB_resource : public http_resource {
public:

  render_const std::shared_ptr<http_response> render_GET(const http_request& req) {

    std::string fileName( sensorFileName() );
    std::ifstream stream( fileName );
    if ( !stream.is_open() )
      return stringResponse( "" );

    std::string content;
    std::getline(stream, content, '\0' );
    
    return stringResponse( content.c_str() );
  }

  render_const std::shared_ptr<http_response> render_POST(const httpserver::http_request& req) {

    std::string post_response;

    auto content = req.get_arg( "sensorDB" );
    
    while ( replace( content, "\r", "" ) )
      ;

    std::string fileName( sensorFileName() );
    std::ofstream stream( fileName );
    if ( !stream.is_open() )
      return stringResponse( "error" );
  
    stream << content;

    return stringResponse( "ok" );
  }
};

  
class nodeList_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

      std::string match( "useNodes" );
      if ( valueFromConfigFile( match ) )
      {
	bool useNodes = false;
	if ( getBool( match.c_str(), useNodes ) && !useNodes )
	  return stringResponse( "<span/>" );
      }
      
      std::string cmd( "./manageNodes.sh list" );

      std::string result = exec( cmd );
      
      std::vector<std::string> lines( string_split(result,'\n') );
      std::sort( lines.begin(), lines.end(), compareLines );

      result = "";

      result += "<table><tr>";
      result += "<th class=\"t-nl\"></th>";
      result += "<th class=\"t-nl\">State</th>";
      result += "<th class=\"t-nl\"></th>";
      result += "<th class=\"t-nl\">MAC</th>";
      result += "<th class=\"t-nl\">IP</th>";
      result += "<th class=\"t-nl\">ActId</th>";
      result += "<th class=\"t-nl\">User</th>";
      result += "<th class=\"t-nl\">Model</th>";
      result += "<th class=\"t-nl\">Active</th>";
      result += "<th class=\"t-nl\">Name</th>";
      result += "<th class=\"t-nl\">Id</th>";
      result += "<th class=\"t-nl\">Type</th>";
      result += "<th class=\"t-nl\">SIN</th>";
      result += "<th class=\"t-nl\">PW</th>";
      result += "<th class=\"t-nl\"></th>";
      result += "<th class=\"t-nl\"></th>";
      result += "<th class=\"t-nl\"></th>";
      result += "<th class=\"t-nl\"></th>";
	  
      result += "</tr>";
      
      for ( int i = 0; i < lines.size(); ++i )
      {	
	const char *col;
	
	if ( startsWithCaseInsensitive( lines[i], "down" ) )
	  col = "ff0000";
	else
	  col = "00df00";

	result += "<tr class=\"t-nl\">";
	result += "<td class=\"t-nl\">";

	result += "<div><span class=\"dot\" style=\"background-color: #";
	result += col;
	result += ";\"></span>";
	result += "</td>";

	std::vector<std::string> f( string_split(lines[i],' ') );
	std::string &ip( f[2] );

	for ( int j = 0; j < f.size(); ++j )
	{ 
//	  printf( "f[%d]: '%s'\n", j, f[j].c_str() );
	  
	  if ( j == 10 )
	    result += "<td class=\"t-nl\" style=\"text-align: right; background-color: ";
	  else
	    result += "<td class=\"t-nl\" style=\"background-color: ";

	  if ( j > 0 && j < 12 )
          { if ( (i%2) )
            { if ( (j%2) )
		result += "#eeeeee";
	      else
		result += "#ffffff";
	    }
	    else
            { if ( (j%2) )
		result += "#dddddd";
	      else
		result += "#efefef";
	    }
	  }
	  result += "\">";

	  if ( j == 2  )
          {
	    result += "<a class=\"btn-lidarTool\" id=\"" + ip + ":" + std::to_string(webserver_port) + "\" href=\"#lidarui\">";
	    result += f[j];
	    result += "</a>";
	  }
	  else if ( j == 6  && f[j] != "unknown" )
          {
	    std::string cmd;
	    
	    if ( f[j] == "running" )
	      cmd = "stop";
	    else
	      cmd = "start";
	    
	    result += "<a class=\"btn-startStop\" id=\"" + ip + "\" href=\"#" + cmd + "\">";
	    result += f[j];
	    result += "</a>";
	  }
	  else if ( f[j] == "-" || f[j] == "+"  )
          {
	    result += "<button class=\"btn btn-list btn-enable ";
	    result += (f[j] == "-" ? "btn-danger" : "btn-success");
	    result += "\" type=\"button\" name=\"";
	    result += f[7];
	    result += "\" >";
	    result += f[j];
	    result += " ";
	  }
	  else if ( j == 10 )
          {
	    std::string sin( f[j] == "_" ? " - &nbsp; " : f[j] );
	      
	    addSINMenu( result, sin, ip );
	  }
	  else if ( j == 11 )
          {
	    std::string pw( f[j] == "pwEn" ? "true" : "false" );
	    result += "<center><input type=\"checkbox\" class=\"form-check-input me-1 btn-list btn-enablePower\" type=\"button\" name=\"" + ip + "\"";
	    if ( f[j] == "pwEn" )
	      result += "\" checked=\"true\">";
	    result += "</input></center>";
	  }
	  else
          {
	    result += "<center>";
	    result += f[j];
	    result += "</center>";
	  }
	    
	  result += "</td>";

	  if ( j == 0 )
          {
	    result += "<td class=\"t-nl\">";
	    addEntryMenu( result, f[1] );
	    result += "</td>";
	  }
	}

	for ( int j = f.size(); j < 12; ++j )
        { result += "<td class=\"t-nl\">";
	  result += "</td>";
	}

	result += "<td class=\"t-nl\">";

	if ( f.size() >= 9 && (f[6] == "running" || f[6] == "stopped") )
	  addNodeAction( result, ip, f[1] );

	result += "</td>";
	
	result += "</tr>";
	result += "</div>";
      }

      result += "</table>";

      return stringResponse( result.c_str() );
    }
};

class server_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

      bool noSensors = false;
      bool hub       = false;
      
      getBoolArg( req, "noSensors", noSensors );
      getBoolArg( req, "hub",  hub );

      std::ifstream stream( "LidarRunMode.txt" );

      std::string path( &req.get_path()[1] );
      std::string cmd;
      
      if ( path == "run" )
      {
	cmd = startServerCmd( noSensors );
      }
      else if ( path == "kill" )
      {
	g_RunningMode = "";
	
	cmd = "./StopServer.sh";
	if ( hub )
	  cmd += " hub";
	cmd += " &";
      }
      else if ( path == "rerun" )
      {
	g_RunningMode = "";
	if ( stream.is_open() )
        { stream >> g_RunningMode;
	  rtrim( g_RunningMode );
        }

	cmd = "./StartServer.sh rerun";
	if ( verbose )
	  cmd += " +v";
	if ( isExpert )
	  cmd += " +expert";
	if ( hub )
	  cmd += " hub";
	if ( noSensors )
	  cmd += " -sensors";
	cmd += " &";
      }
	
      if ( verbose )
	printf( "COM: %s\n",  cmd.c_str() );

      system( cmd.c_str() );

      return stringResponse( path.c_str() );
    }
};


static std::map<std::string,int> g_SpaceFailures;
static std::string g_SpaceFailureReportScript;

static void
checkSpaceFailures()
{
  if ( g_SpaceFailureReportScript.empty() || !fileExists( g_SpaceFailureReportScript ) )
    return;

  std::error_code ec;
  for (auto const& dir : g_FileSizeDirs) {
    
    const std::filesystem::space_info si = std::filesystem::space(dir, ec);
   
    int percent = (int) ((100 * (static_cast<std::intmax_t>(si.capacity) - static_cast<std::intmax_t>(si.available))) / static_cast<std::intmax_t>(si.capacity));

    int value = -1;
    
    std::map<std::string,int>::iterator iter = g_SpaceFailures.find( dir );
    if ( iter != g_SpaceFailures.end() )
      value = iter->second;
    
    if ( percent != value )
    {
      g_SpaceFailures[dir] = percent;

      const int limit = 95;

      std::string result;
      if ( percent > limit )
      {
	result  = "warning: ";
	if ( !g_Conf.empty() )
	{
	  result += "conf=";
	  result += g_Conf;
	  result += " ";
	}
	result += " filesystem ";
	result += dir;
	result += " ";
	result += std::to_string( percent );
	result += "% used";
      }
      else if ( value > limit )
      {
	result  = dir;
	result += " ";
	result += std::to_string( percent );
	result += "%";
      }

      if ( !result.empty() )
      {
	std::string cmd( g_SpaceFailureReportScript );
	cmd.append( " " );
	cmd.append( " \"" );
	cmd.append( result );
	cmd.append( "\" 2>&1 &" );

	if ( verbose )
	  printf( "EXEC: '%s'\n", cmd.c_str() );

	system( cmd.c_str() );
      }
    }
  }
}


class space_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {
 
      std::string result;

      result += "<table class=\"t-fs\"><tr>";
      result += "<th class=\"t-fs\" style=\"text-align:left\">Files</th>";
      result += "<th class=\"t-fs\" style=\"text-align:right\">Size</th>";
      result += "<th class=\"t-fs\" style=\"text-align:right\">Used</th>";
      result += "<th class=\"t-fs\" style=\"text-align:right\">Avail</th>";
      result += "<th class=\"t-fs\" style=\"text-align:right\">Use</th>";
      result += "</tr>";
      result += "\n";

      std::error_code ec;
      for (auto const& dir : g_FileSizeDirs) {
        const std::filesystem::space_info si = std::filesystem::space(dir, ec);
	
	result += "<tr class=\"t-fs\">";

	result += "<td class=\"t-fs\" style=\"text-align:left\">";
	result += dir;
	result += "</td>";

	result += "<td class=\"t-fs\" style=\"text-align:right\">";
	result += humanReadable( static_cast<std::intmax_t>(si.capacity) );
	result += "</td>";

	result += "<td class=\"t-fs\" style=\"text-align:right\">";
	result += humanReadable( static_cast<std::intmax_t>(si.capacity) - static_cast<std::intmax_t>(si.available) );
	result += "</td>";

	result += "<td class=\"t-fs\" style=\"text-align:right\">";
	result += humanReadable( static_cast<std::intmax_t>(si.available) );
	result += "</td>";

	int percent = (int) ((100 * (static_cast<std::intmax_t>(si.capacity) - static_cast<std::intmax_t>(si.available))) / static_cast<std::intmax_t>(si.capacity));
	result += "<td class=\"t-fs";
	if ( percent >= 95 )
	  result += " fs-warning";
	result += "\" style=\"text-align:right\">";
	result += std::to_string( percent );
	result += "%";
	result += "</td>";
	result += "</tr>";
	result += "\n";
      }
 
      result += "</table>";

      std::shared_ptr<http_response> response = stringResponse( result.c_str() );
  
      return response;
    }
};

class lidarTool_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render_GET(const http_request& req) {

      std::string path("." );
      path += req.get_path();

      std::string url = "http://localhost:";
      url += std::to_string( webserver_port );
      
      url += &req.get_path()[strlen("lidarTool/")];

      url += req.get_querystring();

//      printf( "url: %s\n", url.c_str() );

      webMutex.lock();
      
      CURLcode res;
      std::string result, header;
      char errbuf[CURL_ERROR_SIZE];
 
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str() );
      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
      errbuf[0] = 0;
 
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA,    &result);

      curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_HEADERDATA,    &header);
      res = curl_easy_perform(curl);

      webMutex.unlock();

      if ( res != CURLE_OK )
      {
/*
	size_t len = strlen(errbuf);
	fprintf(stderr, "\nlibcurl: (%d) ", res);
	if ( len )
	  fprintf(stderr, "%s%s", errbuf,
		  ((errbuf[len - 1] != '\n') ? "\n" : ""));
	else
	  fprintf(stderr, "%s\n", curl_easy_strerror(res));
*/
	return stringResponse("", "", 404 );
      }

      std::shared_ptr<http_response> response = stringResponse( result.c_str() );

      std::vector<std::string> headers( string_split(header,'\n') );

      for ( int i = 0; i < headers.size(); ++i )
      {
	std::string &h( headers[i] );
	h[h.length()-1] = '\0';
    
	std::vector<std::string> fields( string_split(h,':') );

	if ( fields.size() > 1 )
        {
	  std::string &f0( fields[0] );
	  std::string f1( &fields[1][1] );
	  f0[f0.length()] = '\0';
      
	  if ( f0 != "Content-Length" )
	    response->with_header( f0, f1 );
	}
      }
  
      return response;
    }
};

class lidarHUB_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render_GET(const http_request& req) {

      std::string path("." );
      path += req.get_path();

      std::string url = "http://localhost:";
      url += std::to_string( hub_port );
      
      url += &req.get_path()[strlen("lidarHUB/")];

      url += req.get_querystring();

//      printf( "url: %s\n", url.c_str() );

      webMutex.lock();
      
      CURLcode res;
      std::string result, header;
      char errbuf[CURL_ERROR_SIZE];
 
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str() );
      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
      errbuf[0] = 0;
 
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA,    &result);

      curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_HEADERDATA,    &header);
      res = curl_easy_perform(curl);

      webMutex.unlock();

      if ( res != CURLE_OK )
      {
/*
	size_t len = strlen(errbuf);
	fprintf(stderr, "\nlibcurl: (%d) ", res);
	if ( len )
	  fprintf(stderr, "%s%s", errbuf,
		  ((errbuf[len - 1] != '\n') ? "\n" : ""));
	else
	  fprintf(stderr, "%s\n", curl_easy_strerror(res));
*/
	return stringResponse("", "", 404 );
      }

      std::shared_ptr<http_response> response = stringResponse( result.c_str() );

      std::vector<std::string> headers( string_split(header,'\n') );

      for ( int i = 0; i < headers.size(); ++i )
      {
	std::string &h( headers[i] );
	h[h.length()-1] = '\0';
    
	std::vector<std::string> fields( string_split(h,':') );

	if ( fields.size() > 1 )
        {
	  std::string &f0( fields[0] );
	  std::string f1( &fields[1][1] );
	  f0[f0.length()] = '\0';
      
	  if ( f0 != "Content-Length" )
	    response->with_header( f0, f1 );
	}
      }
  
      return response;
    }
};

class status_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render_GET(const http_request& req) {

      std::string started( "started" );
      std::string stopped( "stopped" );
      std::string damaged( "damaged" );

      std::string status ( damaged );

      std::string url = "http://localhost:";
      url += std::to_string( webserver_port );
      
      url += "/get?isStarted=true&numDevices=true&numFailedDevices=true";

//      printf( "url: %s\n", url.c_str() );

      webMutex.lock();
      
      CURLcode res;
      std::string result, header;
      char errbuf[CURL_ERROR_SIZE];
 
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str() );
      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
      errbuf[0] = 0;
 
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA,    &result);

      curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_HEADERDATA,    &header);
      res = curl_easy_perform(curl);

      webMutex.unlock();

      int numDevices = -1;
      int numFailedDevices = -1;
      std::string appStartDate;
      
      if ( res != CURLE_OK )
      {
/*
	size_t len = strlen(errbuf);
	fprintf(stderr, "\nlibcurl: (%d) ", res);
	if ( len )
	  fprintf(stderr, "%s%s", errbuf,
		  ((errbuf[len - 1] != '\n') ? "\n" : ""));
	else
	  fprintf(stderr, "%s\n", curl_easy_strerror(res));
*/
	status = damaged;
      }
      else
      {
	std::vector<std::string> lines( string_split(result,',') );

	for ( int i = 0; i < lines.size(); ++i )
        {
	  size_t start_pos = lines[i].find( "isStarted" );
	  if ( lines[i].find( "isStarted" ) != std::string::npos )
	  {
	    if ( lines[i].find( "true" ) != std::string::npos )
	      status = started;
	    else
	      status = stopped;
	  }
	  else if ( lines[i].find( "numDevices" ) != std::string::npos )
	  {
	    std::vector<std::string> fields( string_split(lines[i],'\"') );
	    numDevices = std::atoi( fields[2].c_str() );
	  }
	  else if ( lines[i].find( "numFailedDevices" ) != std::string::npos )
	  {
	    std::vector<std::string> fields( string_split(lines[i],'\"') );
	    numFailedDevices = std::atoi( fields[2].c_str() );
	  }
	  else if ( lines[i].find( "appStartDate" ) != std::string::npos )
	  {
	    std::vector<std::string> fields( string_split(lines[i],'\"') );
	    appStartDate = fields[2];
	  }
	}

	if ( numFailedDevices >= 2 || (numDevices > 0 && numFailedDevices/(float)numDevices > 0.5 ) )	   
	  status = damaged;
      }

      std::string json = "{ \"status\": \"";
      json += status;
      json += "\"";
      
      if ( numDevices > 0 )
      {
	json += ", \"numDevices\": ";
	json += std::to_string(numDevices);
      }

      if ( numFailedDevices > 0 )
      {
	json += ", \"numFailedDevices\": ";
	json += std::to_string(numFailedDevices);
      }

      if ( status == started )
      { json += ", \"runningMode\": \"";
	json += g_RunningMode;
	json += "\"";
      }
      
      if ( !appStartDate.empty() )
      { json += ", \"appStartDate\": \"";
	json += appStartDate;
	json += "\"";
      }
      
      json += " }";

      return jsonResponse( json );
     }
};

class html_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render_GET(const http_request& req) {

      std::string path("." );
      path += req.get_path();
 
//      printf( "Path: %s\n", path.c_str() );

      if ( path == "./" || path == "./index.html" )
      {
	std::string html;
	std::string doc( "admin.html" );
	std::getline(std::ifstream((g_HTMLDir+doc).c_str()), html, '\0');

	std::string webport( std::to_string(webserver_port) );
	std::string hubport( std::to_string(hub_port) );

       	replace( html, "8080", webport.c_str() );
       	replace( html, "8081", hubport.c_str() );

	return stringResponse( html.c_str(), "text/html" );
      }
      else if ( endsWithCaseInsensitive( path, ".html" ) )
	return fileResponse( g_HTMLDir+path, "text/html" );
      else if ( endsWithCaseInsensitive( path, ".js" ) )
	return fileResponse( g_HTMLDir+path, "text/javascript" );
      else if ( endsWithCaseInsensitive( path, ".css" ) )
	return fileResponse( g_HTMLDir+path, "text/css" );
      else if ( fileExists( g_HTMLDir+path ) )
      {
	if ( endsWithCaseInsensitive( path, ".jpg" ) ||
	     endsWithCaseInsensitive( path, ".jpeg" ) )
	  return fileResponse( g_HTMLDir+path, "image/jpeg" );
	else if ( endsWithCaseInsensitive( path, ".png" ) )
	  return fileResponse( g_HTMLDir+path, "image/png" );

	return fileResponse( g_HTMLDir+path, "tex/plain" );
      }
      else if ( endsWithCaseInsensitive( path, ".jpg" ) ||
		endsWithCaseInsensitive( path, ".jpeg" ) )
	return fileResponse( g_HTMLDir+path, "image/jpeg" );

      return stringResponse("File not Found", "text/plain", 404 );
    }
};

static get_resource     	get_r;
static set_resource     	set_r;
static status_resource     	status_r;
static lidarTool_resource     	lidarTool_r;
static lidarHUB_resource     	lidarHUB_r;
static html_resource          	html_r;
static server_resource        	server_r;
static space_resource        	space_r;
static nodeList_resource      	nodeList_r;
static sensorDB_resource      	sensorDB_r;
static nodes_resource         	nodes_r;
static unused_resource         	unused_r;

static void
runWebServer()
{
  const int max_threads = 32;

  webserv = new webserver(create_webserver(admin_port).max_threads(max_threads));

  webserv->register_resource("get", &get_r);
  webserv->register_resource("set", &set_r);
  webserv->register_resource("status", &status_r);
  webserv->register_resource("run", &server_r);
  webserv->register_resource("kill", &server_r);
  webserv->register_resource("rerun", &server_r);
  webserv->register_resource("space", &space_r);
  webserv->register_resource("unused", &unused_r);
  webserv->register_resource("nodes", &nodes_r);
  webserv->register_resource("nodeList", &nodeList_r);
  webserv->register_resource("sensorDB", &sensorDB_r);
  webserv->register_resource("lidarTool/{*.html}", &lidarTool_r);
  webserv->register_resource("lidarHUB/{*.html}", &lidarHUB_r);
  webserv->register_resource("/{*.html}", &html_r);
  webserv->register_resource("/", &html_r);
  
  webserv->start(false);
}

/***************************************************************************
*** 
*** Main
***
****************************************************************************/

static std::string
readConf()
{
  std::string dir( "conf" );

  if ( valueFromConfigFile( dir ) )
    return dir;
 
  dir = "";
  
  const char *env = std::getenv( "LIDARCONF" );
  if ( env != NULL && env[0] != '\0' )
    dir = env;
  
  return dir;
}


static void printHelp( int argc, const char *argv[] )
{
  printf( "usage: %s [-h|-help] [+v|+verbose] [+adminport|+ap port(default=%d)] [+webport|+wp port(default=%d)] [+hubport|+hp port(default=%d)] [+startServer] [+fileSystem|+lp filePath(default=%s)|+spaceFailureReportScript scriptFile]\n", argv[0], admin_port, webserver_port, hub_port, g_FileSizeDirs[0].c_str() );
  printf( "  \n" );
}

int main( int argc, const char *argv[] )
{
  setInstallDir( argv[0] );

  readConf();

  for ( int i = 1; i < argc; ++i )
  {
    if ( strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"-help") == 0 || strcmp(argv[i],"--help") == 0 || strcmp(argv[i],"+h") == 0 || strcmp(argv[i],"+help") == 0 )
    { printHelp( argc, argv );
      exit( 0 );
    }
    else if ( strcmp(argv[i],"+adminport") == 0 || strcmp(argv[i],"+ap") == 0 )
    { admin_port = atoi( argv[++i] );
    }
    else if ( strcmp(argv[i],"+webport") == 0 || strcmp(argv[i],"+wp") == 0 )
    { webserver_port = atoi( argv[++i] );
    }
    else if ( strcmp(argv[i],"+hubport") == 0 || strcmp(argv[i],"+hp") == 0 )
    { hub_port = atoi( argv[++i] );
    }
    else if ( strcmp(argv[i],"+fileSystem") == 0 || strcmp(argv[i],"+fs") == 0 )
    { g_FileSizeDirs.push_back( std::string( argv[++i] ) );
    }
    else if ( strcmp(argv[i],"+spaceFailureReportScript") == 0 || strcmp(argv[i],"+sfrs") == 0 )
    { g_SpaceFailureReportScript = argv[++i];

      if ( fileExists( g_SpaceFailureReportScript ) && g_SpaceFailureReportScript[0] != '.' && g_SpaceFailureReportScript[0] != '/' )
	g_SpaceFailureReportScript = std::string("./") + g_SpaceFailureReportScript;
    }
    else if ( strcmp(argv[i],"+v") == 0 || strcmp(argv[i],"+verbose") == 0 )
    { verbose = true;
    }
    else if ( strcmp(argv[i],"+startServer") == 0 )
    { g_StartServer = true;
    }
    else if ( strcmp(argv[i],"+expert") == 0 )
    { isExpert = true;
    }
    else if ( strcmp(argv[i],"+conf") == 0 )
    { g_Conf = argv[++i];
    }
    else
    {
      printf( "unknown option: %s\n", argv[i] );
      printHelp( argc, argv );
      
      exit( 2 );
    }
  }
  
  curl = curl_easy_init();

  if ( !curl )
  { fprintf( stderr, "ERROR initializing libcurl !!!!\n" );
    exit( 0 );
  }

  runWebServer();

  while ( webserv->is_running() )
  {
    if ( g_StartServer )
    {
      g_StartServer = false;
      std::string cmd = startServerCmd();
	
      if ( verbose )
	printf( "COM: %s\n",  cmd.c_str() );

      system( cmd.c_str() );
    }

    usleep( 1000000 );
    checkSpaceFailures();
  }
  
  return 0;
}



