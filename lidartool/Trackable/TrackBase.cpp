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

#include "TrackBase.h"
#include "Vector.cpp"
#include "jsonTool.cpp"
#include "webAPI.cpp"

#if USE_MARKER
#include "markerTool.h"
#endif

/***************************************************************************
*** 
*** DEFINES
***
****************************************************************************/

/***************************************************************************
*** 
*** GLOBALS
***
****************************************************************************/

using namespace rapidjson;

/***************************************************************************
*** 
*** TrackableRegion
***
****************************************************************************/
 
std::string g_RegionsEdgeName[] =
{
  "None",
  "Left",
  "Right",
  "Top",
  "Bottom"
};

std::string g_RegionsShapeName[] =
{
  "Rectangle",
  "Ellipse"
};

std::string TrackGlobal::configDir( "" );

/***************************************************************************
*** 
*** WebSocketServer
***
****************************************************************************/

#ifdef TRACKABLE_WEBSOCKET_OBSERVER_H
#include <cppWebSockets/Util.cpp>
#include <cppWebSockets/WebSocketServer.cpp>
#endif

/***************************************************************************
*** 
*** TrackGlobal
***
****************************************************************************/

static std::map<std::string,TrackableObserverCreator> g_ObserverFactory;
static std::string g_ReadCheckPoint;
static int	   g_Verbose = 0;

KeyValueMap	 TrackGlobal::defaults;
KeyValueMapDB	 TrackGlobal::observers;
TrackableRegions TrackGlobal::regions;

std::string	TrackGlobal::defaultsFileName( "defaults.json" );
std::string	TrackGlobal::observerFileName( "observer.json" );
std::string	TrackGlobal::regionsFileName ( "regions.json" );

    

/***************************************************************************
*** 
*** Log
***
****************************************************************************/

static std::string g_ErrorFileName;
static std::string g_LogFileName;
static std::string g_NotificationScript;

static void 
print( FILE *file, const char *msg, const char *format, va_list &args  )
{
  uint64_t timestamp = getmsec();
  time_t t = timestamp / 1000;
  struct tm timeinfo = *localtime( &t );

  const int maxLen = 2000;
  char buffer[maxLen+1];
  strftime( buffer, maxLen, "%c", &timeinfo );

  fprintf( file, "[%ld.%03ld] %s%s", (long)(timestamp/1000), (long)(timestamp%1000), buffer, msg );
  vfprintf( file, format, args); 
  fprintf( file, "\n");
  fflush( file );

  va_end(args);
}

static void 
printError( const char *format, ...  )
{
  static std::mutex mutex;
  
  va_list args;
  va_start( args, format );

  FILE *file = stderr;
  
  mutex.lock();
  
  if ( !g_ErrorFileName.empty() )
  {
    if ( g_ErrorFileName == "-" || g_ErrorFileName == "stdout" )
      file = stdout;
    else if ( g_ErrorFileName == "stderr" )
      file = stderr;
    else
    {
      file = fopen( g_ErrorFileName.c_str(), "a" );
      if ( file == NULL )
      {
	fprintf( stderr, "ERROR: can not open file \"%s\"\n", g_ErrorFileName.c_str() );
	exit( 1 );
      }
    }
  }
  
  if ( file != stderr && file != stdout )
  { va_list argsCopy;
    va_copy( argsCopy, args );
    print( stderr, ": [Error] ", format, argsCopy );
  }

  if ( file != NULL )
    print( file, ": [Error] ", format, args );

  if ( file != NULL && file != stderr && file != stdout )
    fclose( file );
  
  mutex.unlock();
  
  va_end(args);
}

static void 
printWarning( const char *format, ...  )
{
  static std::mutex mutex;
  
  va_list args;
  va_start( args, format );

  FILE *file = stderr;
  
  mutex.lock();
  
  if ( !g_ErrorFileName.empty() )
  {
    if ( g_ErrorFileName == "-" || g_ErrorFileName == "stdout" )
      file = stdout;
    else if ( g_ErrorFileName == "stderr" )
      file = stderr;
    else
      file = fopen( g_ErrorFileName.c_str(), "a" );
  }
  
  if ( file != stderr && file != stdout )
  { va_list argsCopy;
    va_copy( argsCopy, args );
    print( stderr, ": [Warning] ", format, argsCopy );
  }

  if ( file != NULL )
    print( file, ": [Warning] ", format, args );

  if ( file != stderr && file != stdout )
    fclose( file );
  
  mutex.unlock();
  
  va_end(args);
}

static void 
printLog( const char *format, ...  )
{
  static std::mutex mutex;
  
  va_list args;
  va_start( args, format );

  FILE *file = stdout;
  
  mutex.lock();
  
  if ( !g_LogFileName.empty() )
  {
    if ( g_LogFileName == "-" || g_LogFileName == "stdout" )
      file = stdout;
    else if ( g_LogFileName == "stderr" )
      file = stderr;
    else
    {
      file = fopen( g_LogFileName.c_str(), "a" );
      if ( file == NULL )
      { fprintf( stderr, "ERROR: can not open file \"%s\"\n", g_LogFileName.c_str() );
	exit( 1 );
      }
    }
  }
  
  if ( file != NULL )
    print( file, ": [Log] ", format, args );

  if ( file != NULL && file != stderr && file != stdout )
    fclose( file );
  
  mutex.unlock();
  
  va_end(args);
}

static void 
printInfo( const char *format, ...  )
{
  static std::mutex mutex;
  
  va_list args;
  va_start( args, format );

  mutex.lock();
  print( stdout, ": [INFO] ", format, args );
  mutex.unlock();
  
  va_end(args);
}

static void 
notification( const char *tags, const char *format, ...  )
{
  static std::mutex mutex;
  
  if ( !g_NotificationScript.empty() )
  {
    va_list args;
    va_start( args, format );

    const int maxLen = 2000;
    char buffer[maxLen+1];
    vsnprintf( buffer, maxLen, format, args );

    va_end(args);

    std::string cmd( "type=" );
    cmd.append( tags );
    cmd.append( " " );
    cmd.append( buffer );
    cmd.append( " " );
    cmd.append( g_NotificationScript );
    cmd.append( " 2>&1 &" );

    if ( g_Verbose )
      printf( "EXEC: '%s'\n", cmd.c_str() );

    system( cmd.c_str() );
  }
  else if ( g_Verbose > 0 )
  {
    va_list args;
    va_start( args, format );

    mutex.lock();
    const int maxLen = 2000;
    char buffer[maxLen+1];
    snprintf( buffer, maxLen, ": [INFO] notification(): type=%s ", tags );
    print( stdout, buffer, format, args );
    mutex.unlock();
  
    va_end(args);
  }
}

void (*TrackGlobal::error)	  ( const char *format, ...  ) = printError;
void (*TrackGlobal::warning)	  ( const char *format, ...  ) = printWarning;
void (*TrackGlobal::log)	  ( const char *format, ...  ) = printLog;
void (*TrackGlobal::info)	  ( const char *format, ...  ) = printInfo;
void (*TrackGlobal::notification) ( const char *tags, const char *format, ...  ) = ::notification;

void (*TrackableObserver::error)  ( const char *format, ...  ) = printError;
void (*TrackableObserver::warning)( const char *format, ...  ) = printWarning;
void (*TrackableObserver::log)	  ( const char *format, ...  ) = printLog;
void (*TrackableObserver::info)	  ( const char *format, ...  ) = printInfo;
void (*TrackableObserver::notification) ( const char *tags, const char *format, ...  ) = ::notification;

void
TrackGlobal::setErrorFileName( const char *fileName )
{
  g_ErrorFileName = fileName;
}

void
TrackGlobal::setLogFileName( const char *fileName )
{
  g_LogFileName = fileName;
}

void
TrackGlobal::setNotificationScript( const char *scriptFileName )
{
  if ( !fileExists( scriptFileName ) )
  { error( "setNotificationScript: %s does not exist", scriptFileName );
    return;
  }

  g_NotificationScript = scriptFileName;

  if ( g_NotificationScript[0] != '.' && g_NotificationScript[0] != '/' )
    g_NotificationScript = std::string("./") + g_NotificationScript;
}

/***************************************************************************
*** 
*** sigpipe
***
****************************************************************************/

static bool g_SigPipeInitialized = false;

static void 
sigpipehandler( int )
{
  if ( g_Verbose > 0 )
    TrackGlobal::error( "Got SIGPIPE signal" );
}

void
TrackGlobal::catchSigPipe()
{
  if ( g_SigPipeInitialized )
    return;
  
  signal( SIGPIPE, sigpipehandler );
//  signal( SIGPIPE, SIG_IGN );

  g_SigPipeInitialized = true;
}

/***************************************************************************
*** 
*** getConfigFileName
***
****************************************************************************/

void
TrackGlobal::setReadCheckPoint( const char *checkPoint )
{ g_ReadCheckPoint = std::filesystem::path( checkPoint );
}

static uint64_t cvttimestamp( uint64_t timestamp )
{
  std::string ts( timestampString( "%Y%m%d-%H:%M:%S", timestamp, false ) );
  struct tm tm;
  const char *s = strptime( ts.c_str(), "%Y%m%d-%H:%M:%S", &tm );
  
  if ( s != NULL )
    timestamp = mktime( &tm );
	
  return timestamp;
}


std::string
TrackGlobal::getConfigFileName( const char *fileName, const char *suffix, const char *path, CheckPointMode checkPointMode, uint64_t timestamp )
{
  std::string result;
  time_t maxstamp = 0;
  
  if ( checkPointMode & TrackGlobal::ReadCheckPoint )
  {
    timestamp = cvttimestamp( timestamp );
    
    for ( auto &dirEntry : std::filesystem::directory_iterator(TrackGlobal::configDir) )
    {
      std::string fn( dirEntry.path() );
      if ( fn.length() > 0 && fn[fn.length()-1] != '/' )
	fn += "/";
      fn += fileName;
      if ( suffix != NULL )
	fn += suffix;
      if ( fileExists( fn.c_str() ) )
      { std::vector<std::string> path( split(dirEntry.path(), '/') );
	if ( path[path.size()-1] == g_ReadCheckPoint )
        { result = fn;
	  break;
	}
	
	struct tm tm;
	const char *s = strptime( path[path.size()-1].c_str(), "%Y%m%d-%H:%M:%S", &tm );
        if ( s != NULL )
        { time_t stamp = mktime( &tm );
	  if ( g_ReadCheckPoint == "latest" )
          { if ( stamp > maxstamp )
            { maxstamp = stamp;
	      result   = fn;
	    }
	  }
	  else
          {
	    if ( stamp <= timestamp && (maxstamp == 0 || stamp > maxstamp) )
            { maxstamp = stamp;
	      result   = fn;
	    }
	  }
	}
      }
    }

    if ( !result.empty() )
      return result;
  }
  
  if ( checkPointMode & TrackGlobal::WriteCheckPoint )
  {
    if ( timestamp == 0 )
      timestamp = getmsec();

    std::filesystem::path path( TrackGlobal::configDir );
    path += timestampString( "%Y%m%d-%H:%M:%S/", timestamp, false );

    if ( (checkPointMode&TrackGlobal::CreateCheckPoint) && !fileExists( path.c_str() ) )
      std::filesystem::create_directories( path.c_str() );
      
    path += fileName;
    if ( suffix != NULL )
      path += suffix;

    return path;
  }

  if ( path != NULL )
  { std::string fn( path );
    if ( fn.length() > 0 && fn[fn.length()-1] != '/' )
      fn += "/";
    fn += fileName;
    if ( suffix != NULL )
      fn += suffix;

    if ( fileExists( fn.c_str() ) )
      return fn;
  }

  std::string fn( TrackGlobal::configDir );
  fn += fileName;
  if ( suffix != NULL )
    fn += suffix;
    
  return fn;
}

std::string
TrackGlobal::configFileName( std::string fileName )
{
  replace( fileName, "[conf]/", TrackGlobal::configDir );
  replace( fileName, "[conf]/", "" );
  return fileName;
}

std::string
TrackableObserver::configFileName( const char *fileName )
{
  return TrackGlobal::configFileName( fileName );
}


/***************************************************************************
*** 
*** KeyValue
***
****************************************************************************/

bool
TrackGlobal::WriteKeyValues( KeyValueMap &map, const char *fileName )
{ 
  if ( fileName == NULL || *fileName == '\0' )
    return true;
  
  if ( !writeKeyValues( map, fileName ) )
  { if ( g_Verbose > 0 )
      error( "failed to write keyValue file '%s'", fileName );
    return false;
  }
  
  if ( g_Verbose > 0 )
    info( "writing keyValue file %s", fileName );
  
  return true;
}

bool
TrackGlobal::ReadKeyValues( KeyValueMap &map, const char *fileName, bool reportError )
{ 
  if ( fileName == NULL || *fileName == '\0'  )
    return true;

  if ( !readKeyValues( map, fileName ) )
  { if ( reportError && g_Verbose > 0 )
      error( "failed to read keyValue file '%s'", fileName );
    return false;
  }
  
  if ( reportError && g_Verbose > 0 )
    info( "reading keyValue file %s", fileName );
  
  return true;
}

bool
TrackGlobal::WriteKeyValueMapDB( KeyValueMapDB &map, const char *fileName, const char *key, const char *mapName )
{ 
  if ( fileName == NULL || *fileName == '\0'  )
    return true;

  if ( !writeKeyValueMapDB( map, fileName, key, mapName ) )
  { if ( g_Verbose > 0 )
      error( "failed to write keyValue file '%s'", fileName );
    return false;
  }
  
  if ( g_Verbose > 0 )
    info( "writing keyValueDB file %s", fileName );
  
  return true;
}

bool
TrackGlobal::ReadKeyValueMapDB( KeyValueMapDB &map, const char *fileName, const char *key, const char *mapName )
{ 
  if ( fileName == NULL || *fileName == '\0'  )
    return true;

  if ( !readKeyValueMapDB( map, fileName, key, mapName ) )
  { if ( g_Verbose > 0 )
      error( "failed to read keyValue file '%s'", fileName );
    return false;
  }
  
  if ( g_Verbose > 0 )
    info( "reading keyValueDB file %s", fileName );
  
  return true;
}

bool
TrackGlobal::writeObservers()
{ return WriteKeyValueMapDB( observers, observerFileName.c_str(), "observer", "parameter" );
}

bool
TrackGlobal::readObservers()
{ return ReadKeyValueMapDB( observers, observerFileName.c_str(), "observer", "parameter" );
}

void
TrackGlobal::setObserverValue( const char *name, const char *key, const char *value )
{ observers.set( name, key, value );
}

void
TrackGlobal::removeObserverValue( const char *name, const char *key )
{ observers.remove( name, key );
}

void
TrackGlobal::removeObserver( const char *name )
{ observers.remove( name );
}

void
TrackGlobal::renameObserver( const char *name, const char*newName )
{ observers.rename( name, newName );
}

bool
TrackGlobal::writeDefaults()
{ return WriteKeyValues( defaults, defaultsFileName.c_str() );
}

bool
TrackGlobal::readDefaults()
{ 
  if ( !fileExists(defaultsFileName.c_str()) )
    return true;
  
  return ReadKeyValues( defaults, defaultsFileName.c_str() );
}

bool
TrackGlobal::removeDefault( const char *key )
{ defaults.remove( key );
  return writeDefaults();
}

bool
TrackGlobal::getDefault( const char *key, std::string &value )
{ return defaults.get( key, value );
}

bool
TrackGlobal::getDefault( const char *key, int &value )
{ return defaults.get( key, value );
}

bool
TrackGlobal::getDefault( const char *key, bool &value )
{ return defaults.get( key, value );
}

bool
TrackGlobal::setDefault( const char *key, const char *value )
{ defaults.set( key, value );
  return writeDefaults();
}

static std::string
argvToString( int argc, const char *argv[] )
{ 
  rapidjson::Value json( rapidjson::kArrayType );

  for ( int i = 0; i < argc; ++i )
    json.PushBack( rapidjson::StringRef(argv[i]), rapidjson::allocator );
  
  return rapidjson::toString( json );
}
  
static void
setCommandLineInDefaults( int argc, const char *argv[] )
{ 
  std::string entryName;
  const char *argV[argc-2];
  int j = 0;
      
  for ( int i = 1; i < argc; ++i )
  { if ( strcmp(argv[i],"+setDefaultArgs") == 0 )
    { entryName = argv[++i];
    }
    else
      argV[j++] = argv[i];
  }

  std::string value = argvToString( j, argV );

  TrackGlobal::readDefaults();
  TrackGlobal::setDefault( entryName.c_str(), value.c_str() );
}

static bool
addArgsFromDefaults( std::vector<std::string> &list, const char *entryName )
{ 
  std::string value;
  if ( !TrackGlobal::getDefault( entryName, value ) )
  { TrackGlobal::error( "entry %s not found in defaults", entryName );
    return false;
  }
  
  rapidjson::Document document;
  if ( document.Parse<0>(value.c_str()).HasParseError() )
  { TrackGlobal::error( "%s parse error", value.c_str() );
    return false;
  }
  
  for ( const auto &el : document.GetArray() )
  { std::string value( el.GetString() );
    list.push_back( value );
  }

  return true;
}

bool
TrackGlobal::parseDefaults( int &argc, const char **&argv )
{
  std::vector<std::string> argList;

  for ( int i = 0; i < argc; ++i )
  {
    if ( argv[i][0] == '^' )
    { readDefaults();
      if ( !addArgsFromDefaults( argList, &argv[i][1] ) )
	return false;
    }
    else 
      argList.push_back( std::string(argv[i]) );
  }

  const char **argV = new const char*[argList.size()+1];

  for ( int i = 0; i < argList.size(); ++i )
    argV[i] = strdup( argList[i].c_str() );
  
  argc = argList.size();
  argv = argV;

  return true;
}


bool
TrackGlobal::setDefaults( int &argc, const char **&argv )
{
  std::vector<std::string> argList;

  for ( int i = 0; i < argc; ++i )
  {
    if ( strcmp(argv[i],"+setDefaultArgs") == 0 )
    { setCommandLineInDefaults( argc, argv );
      return false;
    }
    else 
      argList.push_back( std::string(argv[i]) );
  }

  const char **argV = new const char*[argList.size()+1];

  for ( int i = 0; i < argList.size(); ++i )
    argV[i] = strdup( argList[i].c_str() );
  
  argc = argList.size();
  argv = argV;

  return true;
}


bool
TrackGlobal::parseArg( int &i, const char *argv[], int &argc ) 
{
  bool success = false;
 
  if ( strcmp(argv[i],"+setDefault") == 0 )
  { 
    std::string key( argv[++i] );
    TrackGlobal::readDefaults();
    TrackGlobal::setDefault( key.c_str(), argv[++i] );
    exit( 0 );
  }
  else if ( strcmp(argv[i],"+removeDefault") == 0 )
  { 
    TrackGlobal::readDefaults();
    TrackGlobal::removeDefault( argv[++i] );
    exit( 0 );
  }
  else if ( strcmp(argv[i],"+listDefaults") == 0 )
  {
    printf( "\ndefaultsFile=%s\n", TrackGlobal::defaultsFileName.c_str() );

    TrackGlobal::readDefaults();

    for ( std::map<std::string,std::string>::iterator iter( TrackGlobal::defaults.begin() );
	  iter != TrackGlobal::defaults.end();
	  iter++ )
    {
      printf( "\n" );
      printf( "key=%s\n", iter->first.c_str() );
      printf( " value=%s\n", iter->second.c_str() );
    }

    printf( "\n" );
    exit( 0 );
  }
  else if ( strcmp(argv[i],"+setObserverValue") == 0 || strcmp(argv[i],"+setObserverValues") == 0 )
  { 
    std::string observer( argv[++i] );
    KeyValueMap descr;
    ::parseArg( i, argv, argc, descr );

    if ( descr.size() > 0 )
    {
      TrackGlobal::readObservers();

      if ( observer == "all" )
      { for ( KeyValueMapDB::iterator dbiter( TrackGlobal::observers.begin() ); dbiter != TrackGlobal::observers.end(); dbiter++ )
        { KeyValueMap &map( dbiter->second );
	  map.set( descr );
	}
      }
      else
      {
	KeyValueMap map;
	TrackGlobal::observers.get( observer.c_str(), map );
	map.set( descr );
	TrackGlobal::observers.set( observer.c_str(), map );
      }
      
      TrackGlobal::writeObservers();
    }

    exit( 0 );
  }
  else if ( strcmp(argv[i],"+removeObserver") == 0 )
  { 
    std::string observer( argv[++i] );

    TrackGlobal::readObservers();
    TrackGlobal::removeObserver( observer.c_str() );
    TrackGlobal::writeObservers();

    exit( 0 );
  }
  else if ( strcmp(argv[i],"+renameObserver") == 0 )
  { 
    std::string oldName( argv[++i] );
    std::string newName( argv[++i] );
    
    TrackGlobal::readObservers();
    TrackGlobal::renameObserver( oldName.c_str(), newName.c_str() );
    TrackGlobal::writeObservers();

    exit( 0 );
  }
  else if ( strcmp(argv[i],"+removeObserverValue") == 0 )
  { 
    std::string observer( argv[++i] );
    std::string key     ( argv[++i] );

    TrackGlobal::readObservers();
    if ( observer == "all" )
    { for ( KeyValueMapDB::iterator dbiter( TrackGlobal::observers.begin() ); dbiter != TrackGlobal::observers.end(); dbiter++ )
      { KeyValueMap &map( dbiter->second );
	map.remove( key.c_str());
      }
    }
    else
      TrackGlobal::removeObserverValue( observer.c_str(), key.c_str() );

    TrackGlobal::writeObservers();

    exit( 0 );
  }
  else if ( strcmp(argv[i],"+listObserver") == 0 || strcmp(argv[i],"+listObservers") == 0 )
  {
    printf( "\nobserverFile=%s\n", TrackGlobal::observerFileName.c_str() );

    readObservers();
    loadRegions();

    for ( KeyValueMapDB::iterator dbiter( TrackGlobal::observers.begin() ); dbiter != TrackGlobal::observers.end(); dbiter++ )
    {
      KeyValueMap &map( dbiter->second );

      printf( "\n" );
      printf( "observer=%s\n", dbiter->first.c_str() );
      for ( KeyValueMap::iterator iter( map.begin() ); iter != map.end(); iter++ )
      {
	const std::string   &key( iter->first );
	const std::string &value( iter->second );
	printf( "  %s=\"%s\"   ", key.c_str(), value.c_str() );

	if ( (key == "region" || key == "regions") && value != "all" )
        { bool first = true;
	  std::vector<std::string> regionNames( split( value, ',' ) );
	  for ( int i = 0; i < regionNames.size(); ++i )
          { std::string regionName( trim( trim(regionNames[i]), " " ) );
	    TrackableRegion *rect = regions.get( regionName.c_str() );
	    if ( rect == NULL )
	    { if ( first )
	      { printf( "#" );
		first = false;
	      }
	      printf( " region \"%s\" undefined;", regionName.c_str() ); 
	    }
	  }
	}
	
	printf( "\n" );
      }
    }

    printf( "\n" );
    exit( 0 );
  }
  else if ( strcmp(argv[i],"+setRegionsFile") == 0 )
  { 
    regionsFileName = argv[++i];
  }
  else if ( strcmp(argv[i],"+setRegion") == 0 )
  { 
    float x 	 = 0.0;
    float y 	 = 0.0;
    float width  = 6.0;
    float height = 6.0;

    std::string name( argv[++i] );
    
    loadRegions();
    TrackableRegion *rect = regions.get( name.c_str() );
    
    if ( rect != NULL )
    { x = rect->x;
      y = rect->y;
      width  = rect->width;
      height = rect->height;
    }
    
    KeyValueMap descr;
    ::parseArg( i, argv, argc, descr );
    descr.get( "x", x );
    descr.get( "y", y );
    descr.get( "width",  width );
    descr.get( "height", height );

    regions.set( name.c_str(), x, y, width, height );

    rect = regions.get( name.c_str() );
    rect->setKeyValueMap( descr );

    saveRegions();

    exit( 0 );
  }
  else if ( strcmp(argv[i],"+setRegionEdge") == 0 )
  { 
    std::string name    ( argv[++i] );
    std::string edgeName( argv[++i] );

    loadRegions();
    regions.setEdge( name.c_str(), edgeName.c_str() );
    saveRegions();

    exit( 0 );
  }
  else if ( strcmp(argv[i],"+setRegionShape") == 0 )
  { 
    std::string name     ( argv[++i] );
    std::string shapeName( argv[++i] );

    loadRegions();
    regions.setShape( name.c_str(), shapeName.c_str() );
    saveRegions();

    exit( 0 );
  }
  else if ( strcmp(argv[i],"+setRegionTags") == 0 )
  { 
    std::string name( argv[++i] );
    std::string tags( argv[++i] );

    loadRegions();
    regions.setTags( name.c_str(), tags.c_str() );
    saveRegions();

    exit( 0 );
  }
  else if ( strcmp(argv[i],"+setRegionLayers") == 0 )
  { 
    std::string name  ( argv[++i] );
    std::string layers( argv[++i] );

    loadRegions();
    regions.setLayers( name.c_str(), layers.c_str() );
    saveRegions();

    exit( 0 );
  }
  else if ( strcmp(argv[i],"+renameRegion") == 0 )
  { 
    std::string name( argv[++i] );
    std::string newName( argv[++i] );
    loadRegions();
    regions.rename( name.c_str(), newName.c_str() );
    saveRegions();
    
    exit( 0 );
  }
  else if ( strcmp(argv[i],"+removeRegion") == 0 )
  { 
    std::string name( argv[++i] );
    loadRegions();
    regions.remove( name.c_str() );
    saveRegions();

    exit( 0 );
  }
  else if ( strcmp(argv[i],"+listRegion") == 0 || strcmp(argv[i],"+listRegions") == 0 )
  { 
    std::string name( i < strcmp(argv[i],"+listRegion") == 0 ? argv[++i] : "" );

    printf( "\nregionFile=%s\n", regionsFileName.c_str() );

    loadRegions();

    for ( int i = 0; i < regions.size(); ++i )
    {
      TrackableRegion &region( regions[i] );
	
      if ( name.empty() || region.name == name )
      {
	printf( "\n" );
	printf( "name=\"%s\"\n", region.name.c_str() );
	printf( " x=%g\n", region.x );
	printf( " y=%g\n", region.y );
	printf( " width=%g\n", region.width );
	printf( " height=%g\n", region.height );
	printf( " edge=%s\n", g_RegionsEdgeName[region.edge].c_str() );
	printf( " shape=%s\n", g_RegionsShapeName[region.shape].c_str() );
	printf( " tags=\"%s\"\n", region.tagsStr.c_str() );
	printf( " layers=\"%s\"\n", region.layersStr.c_str() );
      }
    }
    
    printf( "\n" );
    exit( 0 );
  }
  else if ( strcmp(argv[i],"+listRegionArgs") == 0 )
  { 
    std::string name( argv[++i] );

    loadRegions();

    for ( int i = 0; i < regions.size(); ++i )
    {
      TrackableRegion &region( regions[i] );
	
      if ( region.name == name )
      {
	printf( "@x=%g", region.x );
	printf( " @y=%g", region.y );
	printf( " @width=%g", region.width );
	printf( " @height=%g", region.height );
	printf( " @edge=%s", g_RegionsEdgeName[region.edge].c_str() );
	printf( " @shape=%s", g_RegionsShapeName[region.shape].c_str() );
	printf( " @tags=\"%s\"", region.tagsStr.c_str() );
	printf( " @layers=\"%s\"", region.layersStr.c_str() );
      }
    }
    
    printf( "\n" );
    exit( 0 );
  }

  return success;
}

bool
TrackGlobal::loadRegions()
{
  if ( g_Verbose > 0 )
    info( "reading Regions file %s", regionsFileName.c_str() );

  bool result = regions.readFromFile( regionsFileName.c_str() );

  if ( result )
  { regions.tags   = regions.collectTags();
    regions.layers = regions.collectLayers();
  }
  
  return result;
}

bool
TrackGlobal::saveRegions()
{
  if ( g_Verbose > 0 )
    info( "writing Regions file %s", regionsFileName.c_str() );

  return regions.writeToFile( regionsFileName.c_str() );
}

void
TrackGlobal::setVerbose( int level )
{
  g_Verbose = level;
}

int
TrackGlobal::verbose()
{
  return g_Verbose;
}

/***************************************************************************
*** 
*** TrackableRegion
***
****************************************************************************/

RegionEdge
TrackableRegion::regionEdgeByString( const char *edge )
{
  std::string edgeName( edge );
  std::for_each(edgeName.begin(), edgeName.end(), [](char & c) { c = ::tolower(c); });
  edgeName[0] = ::toupper(edgeName[0]);

  for ( int i = 0; i < sizeof(g_RegionsEdgeName)/sizeof(g_RegionsEdgeName[0]); ++i )
    if ( edgeName == g_RegionsEdgeName[i] )
      return (RegionEdge) i;
  
  return RegionEdgeNone;
}

const std::string
TrackableRegion::regionEdge_str( RegionEdge edge )
{
  return g_RegionsEdgeName[edge];
}

RegionShape
TrackableRegion::regionShapeByString( const char *shape )
{
  std::string shapeName( shape );
  std::for_each(shapeName.begin(), shapeName.end(), [](char & c) { c = ::tolower(c); });
  shapeName[0] = ::toupper(shapeName[0]);

  for ( int i = 0; i < sizeof(g_RegionsShapeName)/sizeof(g_RegionsShapeName[0]); ++i )
    if ( shapeName == g_RegionsShapeName[i] )
      return (RegionShape) i;
  
  return RegionShapeRect;
}

const std::string
TrackableRegion::regionShape_str( RegionShape shape )
{
  return g_RegionsShapeName[shape];
}


void
TrackableRegion::setCommaList( std::set<std::string> &set, const char *str )
{
  set.clear();

  std::string string( str );
  std::vector<std::string> list( split( string, ',' ) );

  for ( int i = (int)(list.size()-1); i >= 0; --i )
    if ( set.find(list[i]) == set.end() )
      set.emplace( list[i] );
}

void
TrackableRegion::setTags( const char *str )
{
  setCommaList( tags, str );
  tagsStr = str;
}

void
TrackableRegion::setLayers( const char *str )
{
  setCommaList( layers, str );
  layersStr = str;
}


bool
TrackableRegion::setKeyValueMap( KeyValueMap &descr )
{
  for ( auto &entry: descr )
    this->descr.set( entry.first.c_str(), entry.second.c_str() );

  std::string edgeName, shapeName, tagsName, layersName;

  descr.get( "x", x );
  descr.get( "y", y );
  descr.get( "w", width );
  descr.get( "width", width );
  descr.get( "h", height );
  descr.get( "height", height );
  descr.get( "edge", edgeName );
  descr.get( "shape", shapeName );
  descr.get( "tags", tagsName );
  descr.get( "layers", layersName );

  setTags  ( tagsName.c_str() );
  setLayers( layersName.c_str() );

  for ( int i = 0; i < sizeof(g_RegionsEdgeName)/sizeof(g_RegionsEdgeName[0]); ++i )
  { if ( edgeName == g_RegionsEdgeName[i] )
    { edge = i;
      break;
    }
  }

  for ( int i = 0; i < sizeof(g_RegionsShapeName)/sizeof(g_RegionsShapeName[0]); ++i )
  { if ( shapeName == g_RegionsShapeName[i] )
    { shape = i;
      break;
    }
  }

  return true;
}

bool
TrackableRegion::fromKeyValueMap( KeyValueMap &descr )
{
  this->descr.clear();
  
  return setKeyValueMap( descr );
}

bool 
TrackableRegion::toKeyValueMap( KeyValueMap &descr )
{
  descr = this->descr;
  
  descr.setDouble( "x",      x );
  descr.setDouble( "y",      y );
  descr.setDouble( "w",      width );
  descr.setDouble( "h",      height );
  
  descr.set      ( "edge",   g_RegionsEdgeName[edge].c_str() );
  descr.set      ( "shape",  g_RegionsShapeName[shape].c_str() );
  descr.set      ( "tags",   tagsStr.c_str() );
  descr.set      ( "layers", layersStr.c_str() );

  return true;
}

/***************************************************************************
*** 
*** TrackableRegions
***
****************************************************************************/

TrackableRegion *
TrackableRegions::get( const char *name, bool create )
{
  for ( int i = (int)(this->size()-1); i >= 0; --i )
    if ( (*this)[i].name == name )
      return &(*this)[i];

  if ( create )
    return &add( name );
    
  return NULL;
}
  
std::set<TrackableRegion *>
TrackableRegions::getByNameOrTag( const char *name )
{
  std::string nameS(name);
  
  std::set<TrackableRegion *> result;
  std::vector<std::string> tags( split( nameS, ',' ) );

  for ( int i = (int)(this->size()-1); i >= 0; --i )
  {
    TrackableRegion &region( (*this)[i] );
    
    bool success = (region.name == name || nameS == "all" );

    if ( !success )
    { for ( int i = 0; !success && i < tags.size(); ++i )
	if ( region.hasTag(tags[i].c_str()) )
	  success = true;
    }

    if ( success )
      result.emplace( &region );
  }
  
  return result;
}
std::set<TrackableRegion *>
TrackableRegions::getByLayer( const char *layer )
{
  std::string layerS(layer);
  
  std::set<TrackableRegion *> result;
  std::vector<std::string> layers( split( layerS, ',' ) );
    
  for ( int i = (int)(this->size()-1); i >= 0; --i )
  {
    TrackableRegion &region( (*this)[i] );
    
    bool success = (layerS == "all" );

    for ( int i = 0; !success && i < layers.size(); ++i )
      if ( region.hasLayer(layers[i].c_str()) )
	success = true;

    if ( success )
      result.emplace( &region );
  }
  
  return result;
}


TrackableRegion &
TrackableRegions::add( const char *name )
{
  std::string n( name );
  if ( name == NULL || *name == '\0' )
  { n = this->name;
    n += std::to_string( this->size() );
  }
    
  push_back( TrackableRegion( n.c_str() ) );
  return back();
}
  
TrackableRegion &
TrackableRegions::add( float x, float y, float width, float height, const char *name )
{
  std::string n( name );
  if ( name == NULL || *name == '\0' )
  { n = this->name;
    n += std::to_string( this->size() );
  }
    
  push_back( TrackableRegion( x, y, width, height, n.c_str() ) );
  return back();
}
  
void
TrackableRegions::remove( const char *name )
{
  if ( std::string(name) == "all" )
    clear();
  
  for ( int i = (int)(this->size()-1); i >= 0; --i )
    if ( (*this)[i].name == name )
    { this->erase( this->begin()+i );
      break;
    }
}

void
TrackableRegions::rename( const char *name, const char *newName )
{
  TrackableRegion *rect = get( name );
  remove( newName );
  rect->name = newName;
}

void
TrackableRegions::set( const char *name, float x, float y, float width, float height )
{
  TrackableRegion *rect = get( name, true );
  rect->x = x;
  rect->y = y;
  rect->width  = width;
  rect->height = height;
}
  
void
TrackableRegions::setEdge( const char *name, int edge )
{
  if ( std::string(name) == "all" )
  { for ( int i = (int)(this->size()-1); i >= 0; --i )
      (*this)[i].edge = edge;
    return;
  }

  TrackableRegion *rect = get( name );
  if ( rect == NULL )
    return;
  
  rect->edge = edge;
}

void
TrackableRegions::setEdge( const char *name, const char *edge )
{
  if ( std::string(name) == "all" )
  { for ( int i = (int)(this->size()-1); i >= 0; --i )
      (*this)[i].edge = TrackableRegion::regionEdgeByString( edge );
    return;
  }

  TrackableRegion *rect = get( name );
  if ( rect == NULL )
    return;
  
  rect->edge = TrackableRegion::regionEdgeByString( edge );
}

void
TrackableRegions::setShape( const char *name, int shape )
{
  if ( std::string(name) == "all" )
  { for ( int i = (int)(this->size()-1); i >= 0; --i )
      (*this)[i].shape = shape;
    return;
  }

  TrackableRegion *rect = get( name );
  if ( rect == NULL )
    return;
  
  rect->shape = shape;
}

void
TrackableRegions::setShape( const char *name, const char *shape )
{
  if ( std::string(name) == "all" )
  { for ( int i = (int)(this->size()-1); i >= 0; --i )
      (*this)[i].shape = TrackableRegion::regionShapeByString( shape );
    return;
  }

  TrackableRegion *rect = get( name );
  if ( rect == NULL )
    return;
  
  rect->shape = TrackableRegion::regionShapeByString( shape );
}

void
TrackableRegions::setTags( const char *name, const char *tags )
{
  if ( std::string(name) == "all" )
  { for ( int i = (int)(this->size()-1); i >= 0; --i )
      (*this)[i].setTags( tags );
    return;
  }

  TrackableRegion *rect = get( name );
  if ( rect == NULL )
    return;
  
  rect->setTags( tags );
}

void
TrackableRegions::setLayers( const char *name, const char *layers )
{
  if ( std::string(name) == "all" )
  { for ( int i = (int)(this->size()-1); i >= 0; --i )
      (*this)[i].setLayers( layers );
    return;
  }

  TrackableRegion *rect = get( name );
  if ( rect == NULL )
    return;
  
  rect->setLayers( layers );
}

std::set<std::string> 
TrackableRegions::collectTags()
{
  std::set<std::string> result;
  for ( int i = (this->size()-1); i >= 0; --i )
    for ( auto tag: (*this)[i].tags )
      result.emplace( tag );

  return result;
}

std::set<std::string> 
TrackableRegions::collectLayers()
{
  std::set<std::string> result;
  for ( int i = (int)(this->size()-1); i >= 0; --i )
    for ( auto layer: (*this)[i].layers )
      result.emplace( layer );
  
  return result;
}

static bool
compareRectName( const TrackableRegion& r1, const TrackableRegion& r2 )
{ 
  if ( r1.name == "heatmap" || r1.name == "flowmap" || r1.name == "tracemap" )
    return false;
  if ( r2.name == "heatmap" || r2.name == "flowmap" || r1.name == "tracemap" )
    return true;
  
  return r1.name < r2.name; }

/*
bool 
TrackableRegions::writeToFile( const char *fileName )
{ 
  std::ofstream outfile( fileName );
  if ( outfile.fail() )
    return false;

  std::sort( begin(), end(), compareRectName );

  OStreamWrapper osw(outfile);
  rapidjson::PrettyWriter<OStreamWrapper> writer(osw);
  rapidjson::Value json( toJson( true ) );

  writer.SetMaxDecimalPlaces( 3 );
  json.Accept(writer);

  return true;
}
*/

bool 
TrackableRegions::writeToFile( const char *fileName )
{ 
  KeyValueMapDB descr;
  
  for ( auto &region : TrackGlobal::regions )
  {
    KeyValueMap desc;
    
    if ( !region.toKeyValueMap( desc ) )
      return false;

    descr.set( region.name.c_str(), desc );
  }

  return TrackGlobal::WriteKeyValueMapDB( descr, fileName, "region", "parameter" );
}

bool
TrackableRegions::readFromFile( const char *fileName )
{ 
  KeyValueMapDB descr;
  if ( !TrackGlobal::ReadKeyValueMapDB( descr, fileName, "region", "parameter" ) )
    return false;

  resize( 0 );

  for ( auto &entry : descr )
  {
    TrackableRegion region;
    region.name = entry.first;

    if ( !region.fromKeyValueMap( entry.second ) )
      return false;
      
    push_back( region );
  }

  return true;
}

/***************************************************************************
*** 
*** TrackBase PackedPlayer
***
****************************************************************************/

static std::atomic<float>    g_PackedPlayerPlayPos      = -1.0;
static std::atomic<int64_t>  g_PackedPlayerCurrentTime  = -1;
static std::atomic<uint64_t> g_PackedPlayerTimeStamp    = 0;
static std::atomic<uint64_t> g_PackedPlayerTimeStampRef = 0;

static std::atomic<bool>     g_PackedPlayerPaused       = false;

static PackedPlayer	    *g_PackedPlayer             = NULL;

static std::mutex	     g_PackedPlayerMutex;
static std::thread	    *g_PackedPlayerThread = NULL;
static std::atomic<bool>     g_PackedPlayerExitThread = false;

PackedPlayer *
TrackBase::packedPlayer()
{ return g_PackedPlayer;
}

void
TrackBase::setPackedPlayer( PackedPlayer *packedPlayer )
{ g_PackedPlayer = packedPlayer;
  g_PackedPlayerTimeStamp    = 1;
}
    
float
TrackBase::packedPlayerPlayPos()
{ return g_PackedPlayerPlayPos;
}

int64_t
TrackBase::packedPlayerCurrentTime()
{ return g_PackedPlayerCurrentTime;
}

uint64_t
TrackBase::packedPlayerTimeStamp()
{
  if ( g_PackedPlayerTimeStamp == 0 )
    return 0;

  if ( g_PackedPlayerPaused )
    return g_PackedPlayerTimeStamp;
  
  return g_PackedPlayerTimeStamp + getmsec() - g_PackedPlayerTimeStampRef;
}

bool
TrackBase::packedPlayerIsPaused()
{ return g_PackedPlayerPaused;
}

bool
TrackBase::packedPlayerAtEnd()
{
  return g_PackedPlayer != NULL && g_PackedPlayer->is_eof();
}

void
TrackBase::setPackedPlayerPaused( bool paused )
{ 
  g_PackedPlayerPaused = paused;
  if ( !paused )
    setPackedPlayerPlayPos( g_PackedPlayerPlayPos );
}

void
TrackBase::setPackedPlayerPlayPos( float playPos )
{
  g_PackedPlayerPlayPos = playPos;
  
  uint64_t now = getmsec();
  
  uint64_t begin_time = 0;

  g_PackedPlayerCurrentTime   		= g_PackedPlayer->play( playPos );
  g_PackedPlayer->file->start_time 	= now - g_PackedPlayerCurrentTime;
  g_PackedPlayerPlayPos       		= g_PackedPlayer->playPos();
  g_PackedPlayerTimeStamp     		= g_PackedPlayer->timeStamp();
  g_PackedPlayerTimeStampRef  		= getmsec();

/*
  for ( int i = 0; i < g_DeviceList.size(); ++i )
  { g_DeviceList[i]->scanOnce = true;
    g_DeviceList[i]->unlock();
  }
*/
}

void
TrackBase::setPackedPlayerSyncTime( uint64_t timestamp )
{
  g_PackedPlayerPlayPos = 0.0;
}

static ObsvObjects *g_PackedPlayerObjects = NULL;

static void runPackedPlayerThread( TrackBase *trackBase )
{
  while ( !g_PackedPlayerExitThread )
  {
    ObsvObjects *objects = new ObsvObjects();
    PackedTrackable::HeaderType type = g_PackedPlayer->grabFrame( *objects );

    if ( (type&PackedTrackable::TypeBits) != PackedTrackable::Unknown )
    { g_PackedPlayerCurrentTime  = g_PackedPlayer->currentTime();
      g_PackedPlayerTimeStamp    = g_PackedPlayer->timeStamp();
      g_PackedPlayerTimeStampRef = getmsec();
      g_PackedPlayerPlayPos      = g_PackedPlayer->playPos();
    }
/*
    if ( (type&PackedTrackable::TypeBits) == PackedTrackable::StartHeader )
      trackBase->m_Stage->start( g_PackedPlayer->currentFrame.header.timestamp );
    else if ( (type&PackedTrackable::TypeBits) == PackedTrackable::StopHeader )
      trackBase->m_Stage->stop( g_PackedPlayer->currentFrame.header.timestamp );
    else 
*/
    if ( (type&PackedTrackable::TypeBits) == PackedTrackable::FrameHeader )
    {
      g_PackedPlayerMutex.lock();

      if ( g_PackedPlayerObjects != NULL )
	delete g_PackedPlayerObjects;
      g_PackedPlayerObjects = objects;

      g_PackedPlayerMutex.unlock();
    }
    else
      delete objects;
  }
}


void
TrackBase::trackObjects( ObsvObjects &objects )
{
  Trackables<BlobMarkerUnion> &current( *new Trackables<BlobMarkerUnion>() );

  for ( auto &iter: objects )
  { 
    ObsvObject &object( iter.second );

    current.push_back( Trackable<BlobMarkerUnion>::Ptr( m_Stage->createTrackable()) );
    Trackable<BlobMarkerUnion>::Ptr trackable = current.back();
//    trackable->touchTime( timestamp );

    trackable->type = BlobMarkerUnion::Blob;
      
    trackable->p[0] = object.x;
    trackable->p[1] = object.y;
    trackable->p[2] = NAN;
    trackable->size = object.size;
    trackable->init( objects.timestamp );

    trackable->Id   = std::to_string( object.id );
    trackable->uuid = object.uuid;
    trackable->isActivated = true;
      
    trackable->setTouched( object.isTouched() );
    trackable->setPrivate( object.isPrivate() );
    trackable->touchTime ( objects.timestamp );
  }

  m_Stage->latest = Trackables<BlobMarkerUnion>::Ptr( &current );
     
  m_Stage->frame_count = objects.frame_id;
  m_Stage->touchTime( objects.timestamp );
}
    

void
TrackBase::observe( PackedTrackable::Header &header )
{
  switch ( header.flags & PackedTrackable::TypeBits )
  {
    case PackedTrackable::StartHeader:
    { if ( m_Stage->observer != NULL )
	m_Stage->observer->start( header.timestamp );
      break;
    }

    case PackedTrackable::StopHeader:
    { if ( m_Stage->observer != NULL )
	m_Stage->observer->stop( header.timestamp );
      break;
    }

    default:
    {
      printf( "TrackBase::observe(): unknown header type\n" );
      break;
    }
  }
}


void
TrackBase::observe( PackedTrackable::BinaryFrame &frame )
{
  incFrameCount( m_Stage->frame_count );
     
  ObsvObjects objects;

  objects.frame_id  = m_Stage->frame_count;

  if ( !PackedPlayer::decodeFrame( objects, frame ) )
    return;	

//  if ( g_Verbose )
//    printf( "got %ld trackables\n", objects.size() );

  trackObjects( objects );

  if ( m_Stage->observer != NULL )
    m_Stage->observer->observe( objects );
}


void
TrackBase::packedPlayerTrack( uint64_t timestamp, bool waitForFrame )
{
  if ( g_PackedPlayer == NULL )
    return;

  if ( g_PackedPlayerThread == NULL )
    g_PackedPlayerThread = new std::thread( runPackedPlayerThread, this );  

  g_PackedPlayerMutex.lock();

  if ( waitForFrame && g_PackedPlayerObjects == NULL )
  { uint64_t now = getmsec();  
    while ( g_PackedPlayerObjects == NULL )
    { g_PackedPlayerMutex.unlock();
      if ( getmsec() - now > 500000 )
	return;
      usleep( 1000 );
      g_PackedPlayerMutex.lock();
    }
  }
  
  if ( g_PackedPlayerObjects == NULL )
  { g_PackedPlayerMutex.unlock();
    return;
  }
  
  ObsvObjects *objects = g_PackedPlayerObjects;
  g_PackedPlayerObjects = NULL;
  g_PackedPlayerMutex.unlock();
    
  trackObjects( *objects );

  if ( m_Stage->observer != NULL )
    m_Stage->observer->observe( *objects );

  delete objects;
}


/***************************************************************************
*** 
*** TrackBase
***
****************************************************************************/

TrackBase::TrackBase()
  : m_Stage   ( new TrackableMultiStage<BlobMarkerUnion>() ),
    uniteMethod  ( UniteObjects ),
    imageSpaceResolution( 0.125 ),
    logDistance	   ( 0.5 )
{
  m_Stage->trackFilterWeight = 0.125;
  m_Stage->uniteDistance     = 0.4;
  m_Stage->trackDistance     = 1.0;
}

void
TrackBase::setObserverRegion( TrackableObserver *observer, const char *regionName )
{
  std::string regionsString( regionName == NULL ? "" : regionName  );

  if ( !regionsString.empty() )
  { 
    std::vector<std::string> regionUnite( split( regionsString, '=', 2 ) );
    std::vector<std::string> regionNames( split( regionUnite[0], ',' ) );
    
    for ( int i = 0; i < regionNames.size(); ++i )
    {
      std::string regionName( trim( trim(regionNames[i]), " " ) );
      bool        invert = false;
      if ( regionName[0] == '~' )
      { invert = true;
	std::string tmp( &regionName.c_str()[1] );
	regionName = tmp;
      }

      std::set<TrackableRegion *> regions( TrackBase::regions.getByNameOrTag( regionName.c_str() ) );

      for ( auto region: regions )
      {
	float w2 = 0.5 * region->width;
	float h2 = 0.5 * region->height;
	
	ObsvRect *r = observer->setRect( region->name.c_str(), region->x-w2, region->y-h2, region->width, region->height, (ObsvRect::Edge) region->edge, (ObsvRect::Shape) region->shape );
	if ( invert )
	  r->invert = true;
      }
    }

    if ( regionUnite.size() == 2 )
      observer->rects.unite( regionUnite[1].c_str() );
  }
}

void
TrackBase::setObserverParam( TrackableObserver *observer, KeyValueMap &descr )
{
  observer->setParam( descr );

  std::string regionsString;
  if ( descr.get( "regions", regionsString ) || descr.get( "region", regionsString ) )
    TrackBase::setObserverRegion( observer, regionsString.c_str() );
}


bool
TrackBase::addObserver( TrackableObserver *observer )
{
  if ( m_Stage != NULL )
    m_Stage->addObserver( observer );

  return true;
}

bool
TrackBase::addObserver( KeyValueMap &descr )
{
  std::string type;
  if ( !descr.get( "type", type ) )
  { error( "add observer: missing observer type" );
    return false;
  }

  bool active;
  if ( descr.get( "active", active ) && !active )
    return true;
  
  std::string name;
  if ( !descr.get( "name", name ) )
    name = type + "_default";
  
  TrackableObserver *observer = NULL;
  
  std::map<std::string,TrackableObserverCreator>::iterator iter( g_ObserverFactory.find(type) );
  TrackableObserverCreator creator = NULL;
  if ( iter != g_ObserverFactory.end() )
    creator = iter->second;

  if ( type == "file" )
  { 
    std::string fileName;
    if ( !descr.get( "file", fileName ) )
    { error( "add %s observer: missing observer file", name.c_str() );
      return false;
    }

    if ( creator != NULL )
      observer = creator();
    else
      observer = new TrackableFileObserver();

    float dummy;
    if ( !descr.get( "logDistance", dummy ) )
      observer->reportDistance = this->logDistance;
    
    std::string filter;
    if ( !descr.get("filter",filter) && !logFilter.empty() )
      observer->obsvFilter.parseFilter( logFilter.c_str() );
  }
  else if ( type == "packedfile" )
  { 
    std::string fileName;
    if ( !descr.get( "file", fileName ) )
    { error( "add %s observer: missing observer file", name.c_str() );
      return false;
    }

    if ( creator != NULL )
      observer = creator();
    else
      observer = new TrackablePackedFileObserver();
  }
  else if ( type == "bash" )
  { observer = new TrackableBashObserver();
  }
  else if ( type == "udp" )
  { std::string url;
    if ( !descr.get( "url", url ) )
    { error( "add %s observer: missing observer url", name.c_str() );
      return false;
    }

    if ( creator != NULL )
      observer = creator();
    else
      observer = new TrackableUDPObserver();
  }
#ifdef TRACKABLE_WEBSOCKET_OBSERVER_H
  else if ( type == "websocket" )
  { 
    int port = 5000;
    descr.get( "port", port );
    if ( creator != NULL )
      observer = creator( port );
    else
      observer = new TrackableWebSocketObserver( port );
    catchSigPipe();
  }
  else if ( type == "packedwebsocket" )
  { 
    int port = 5000;
    descr.get( "port", port );
    if ( creator != NULL )
      observer = creator( port );
    else
      observer = new TrackablePackedWebSocketObserver( port );

    catchSigPipe();
  }
#endif
#ifdef TRACKABLE_OSC_OBSERVER_H
  else if ( type == "osc" )
  { std::string url;
    if ( !descr.get( "url", url ) )
    { error( "add %s observer: missing observer url", name.c_str() );
      return false;
    }

    if ( creator != NULL )
      observer = creator( url.c_str() );
    else
      observer = new TrackableOSCObserver( url.c_str() );
  }
#endif
#ifdef TRACKABLE_MQTT_OBSERVER_H
  else if ( type == "mqtt" )
  { std::string url;
    if ( !descr.get( "url", url ) )
    { error( "add %s observer: missing observer url", name.c_str() );
      return false;
    }
    
    static bool isInitialized = false;
    if ( !isInitialized )
    { mosquitto_lib_init();
      isInitialized = true;
    }

    if ( creator != NULL )
      observer = creator( url.c_str() );
    else
      observer = new TrackableMQTTObserver( url.c_str() );

    float dummy;
    if ( !descr.get( "logDistance", dummy ) )
      observer->reportDistance = this->logDistance;
  }
#endif
#ifdef TRACKABLE_LUA_OBSERVER_H
  else if ( type == "lua" )
  { std::string script;
    if ( !descr.get( "script", script ) )
    { error( "add %s observer: missing observer script", name.c_str() );
      return false;
    }
    
    if ( creator != NULL )
      observer = creator();
    else
      observer = new TrackableLuaObserver();
  }
#endif
#ifdef TRACKABLE_INFLUXDB_OBSERVER_H
  else if ( type == "influxdb" )
  { 
    if ( creator != NULL )
      observer = creator();
    else
      observer = new TrackableInfluxDBObserver();
  }
#endif
#ifdef TRACKABLE_IMAGE_OBSERVER_H 
  else if ( type == "heatmap" || type == "flowmap" || type == "tracemap" )
  { 
    float spaceResolution;
    if ( !descr.get( "spaceResolution",  spaceResolution) )
    { std::string spaceResolution( std::to_string(this->imageSpaceResolution) );
      descr.set( "spaceResolution", spaceResolution.c_str() );
    }
    
    if ( creator != NULL )
      observer = creator();
    else if ( type == "tracemap" )
      observer = new TrackableTraceMapObserver();
    else if ( type == "flowmap" )
      observer = new TrackableFlowMapObserver();
    else
      observer = new TrackableHeatMapObserver();
  }
#endif
  else if ( creator != NULL )
  {
    observer = creator( &descr );
  }
  else
  {
    error( "add %s observer: unknown observer type: %s", name.c_str(), type.c_str() );
    return false;
  }

  if ( observer->type&TrackableObserver::Image )
  { std::string fileName;
    if ( !descr.get( "file", fileName ) )
      descr.set( "file", "" );
  }

  observer->name = name;

  setObserverParam( observer, descr );

  addObserver( observer );

  return true;
}

void
TrackBase::finishObserver()
{
  if ( m_Stage == NULL )
    return;

  if ( m_Stage->observer == NULL )
    return;

  delete m_Stage->observer;
  m_Stage->observer = NULL;
}

void
TrackBase::registerObserverCreator( const char *type, TrackableObserverCreator creator )
{ 
  if ( g_ObserverFactory.count(type) )
    g_ObserverFactory[type] = creator;
  else
    g_ObserverFactory.emplace( std::make_pair(type,creator) );
}

bool
TrackBase::updateObserverRegion( const char *regionName )
{
  if ( m_Stage->observer == NULL )
    return false;
  
  TrackableRegion *rect = regions.get( regionName );
  if ( rect == NULL )
    return false;
  
  float w2 = 0.5 * rect->width;
  float h2 = 0.5 * rect->height;

  bool success = false;
  
  TrackableMultiObserver &multi( *m_Stage->observer );
  for ( int i = 0; i < multi.observer.size(); ++i )
  {
    ObsvRect *r = multi.observer[i]->getRect( regionName );
    if ( r != NULL )
    {
      r->set( rect->x-w2, rect->y-h2, rect->width, rect->height );
      success = true;
    }
  }
  
  return success;
}

void
TrackBase::reset()
{
  m_Stage->reset();
}

void
TrackBase::markUsedRegions()
{
  for ( int i = ((int)regions.size())-1; i >= 0; --i )
    regions[i].usedByObserver = "";

  if ( m_Stage->observer == NULL )
    return;
  
  TrackableMultiObserver &multi( *m_Stage->observer );
  for ( int i = 0; i < regions.size(); ++i )
  {
    TrackableRegion &region( regions[i] );
    
    for ( int o = 0; o < multi.observer.size(); ++o )
    { 
      ObsvRect *rect = multi.observer[o]->getRect( region.name.c_str() );
      if ( rect != NULL )
      {
	if ( !region.usedByObserver.empty() )
	  region.usedByObserver += ",";
	region.usedByObserver += multi.observer[o]->name;
      }	
    }
  }
}

