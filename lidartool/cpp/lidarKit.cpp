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

#include <filesystem>
#include <atomic>
#include <stdarg.h>

#include "helper.h"
#include "keyValueMap.h"
#include "lidarKit.h"
#include "scanData.h"

#include "rplidar_driver.cpp"

using namespace rp::standalone::rplidar;

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

static _u32         baudrateArray[]    = {115200, 256000};
static const int    ld06MinQuality     = 12;
static const int    ld06EnvMinQuality  = 12;

static std::vector<LidarDevice*> g_DeviceList;

static int   g_Verbose			= false;
static bool  g_Debug	 		= false;
static bool  g_Shutdown			= false;
static bool  g_IsInitialized 	  	= false;
static bool  g_IsSimulationMode		= false;
static bool  g_UseSimulationRange  	= false;

static std::string hardwareDir( "./" );
std::string LidarDevice::installDir( "./" );
std::string LidarDevice::configDir( "" );
std::string LidarDevice::configDirAlt( "" );
std::string LidarDevice::defaultDeviceType( "" );
std::string LidarDevices::message;

float LidarObject::maxMarkerDistance = 2.5;

const char *LidarDevice::RPLidarTypeName  = "rplidar";
const char *LidarDevice::YDLidarTypeName  = "ydlidar";
const char *LidarDevice::LDLidarTypeName  = "ldlidar";
const char *LidarDevice::LSLidarTypeName  = "lslidar";
const char *LidarDevice::MSLidarTypeName  = "mslidar";
const char *LidarDevice::UndefinedTypeName = "UNDEFINED";


static std::string g_Model;
static std::string g_ReadCheckPoint;


static std::filesystem::file_time_type g_PoweringSupportedTimeStamp;
static std::string g_PoweringEnabledFileName( "./hardware/LidarPower.enable" );

static bool	   g_PoweringSupported = false;
static bool	   g_StatusIndicatorSupported = false;
static bool	   g_UseStatusIndicator = false;

static int g_RockPiSDefaultSerialId = 1;

static int	             g_FileDriverSyncIndex    = -1;
static uint64_t 	     g_FileDriverSyncTime     = 0;
static std::atomic<float>    g_FileDriverPlayPos      = -1.0;
static std::atomic<int64_t>  g_FileDriverCurrentTime  = -1;
static std::atomic<uint64_t> g_FileDriverTimeStamp    = 0;
static std::atomic<uint64_t> g_FileDriverTimeStampRef = 0;

static std::atomic<bool>     g_FileDriverPaused       = false;

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
      file = fopen( g_ErrorFileName.c_str(), "a" );
  }
  
  if ( file != stderr && file != stdout )
  { va_list argsCopy;
    va_copy( argsCopy, args );
    print( stderr, ": [Error] ", format, argsCopy );
  }

  if ( file != NULL )
    print( file, ": [Error] ", format, args );

  if ( file != stderr && file != stdout )
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
  print( stdout, ": [Info] ", format, args );
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
  else
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

void (*Lidar::error)  ( const char *format, ...  ) = printError;
void (*Lidar::warning)( const char *format, ...  ) = printWarning;
void (*Lidar::log)    ( const char *format, ...  ) = printLog;
void (*Lidar::info)   ( const char *format, ...  ) = printInfo;
void (*Lidar::notification)  ( const char *tags, const char *format, ...  ) = ::notification;

void
Lidar::setErrorFileName( const char *fileName )
{
  g_ErrorFileName = fileName;
}

void
Lidar::setLogFileName( const char *fileName )
{
  g_LogFileName = fileName;
}

void
Lidar::setNotificationScript( const char *scriptFileName )
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
*** Helper
***
****************************************************************************/

static std::string
exec( std::string cmd, bool verbose=false )
{
  std::string result;
  FILE * stream;
  const int max_buffer = 256;
  char buffer[max_buffer];
  cmd.append(" 2>&1");

  if ( verbose )
    Lidar::info( "EXEC: '%s'", cmd.c_str() );

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

static void
readPoweringSupported()
{
  std::string cmd( hardwareDir );

  cmd += "lidarPower.sh isSupported";
  std::string isSupported = exec( cmd.c_str() );
  
  g_PoweringSupported = (isSupported == "true");

  if ( fileExists(g_PoweringEnabledFileName.c_str() ) )
    g_PoweringSupportedTimeStamp = std::filesystem::last_write_time( g_PoweringEnabledFileName );
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

bool
LidarDevice::isPoweringSupported()	  
{ 
  if ( isVirtualDevice() )
    return false;
  
  static uint64_t last_timestamp = 0;

  uint64_t timestamp = getmsec();
  
  if ( timestamp - last_timestamp < 1000 ) 
    return g_PoweringSupported;

  bool changed = false;
  if ( fileExists(g_PoweringEnabledFileName.c_str() ) )
  { std::filesystem::file_time_type time = std::filesystem::last_write_time( g_PoweringEnabledFileName );
    changed = (time > g_PoweringSupportedTimeStamp);
  }
  
  if ( changed )
    readPoweringSupported();
  
  return g_PoweringSupported;
}

bool
LidarDevice::devicePoweringSupported()
{
  if ( isVirtualDevice() )
    return inVirtSensorPower;
  
  bool isUART = (getConnectionType( deviceName.c_str() ) == UART);
  if ( isUART && isPoweringSupported() )
    return true;

  if ( driverType == RPLIDAR )
    return true;
  
  if ( driverType == YDLIDAR )
    return true;

  if ( driverType == MSLIDAR )
    return true;

  if ( driverType == LDLIDAR )
    return false;

  if ( driverType == LSLIDAR )
    return false;

  return true;
}


std::string
LidarDevice::getConfigFileName( const char *fileName, const char *suffix, const char *path, CheckPointMode checkPointMode, uint64_t timestamp )
{
  std::string result;
  time_t maxstamp = 0;
  
  if ( checkPointMode & CheckPointMode::ReadCheckPoint )
  {
    timestamp = cvttimestamp( timestamp );
    
    for ( auto &dirEntry : std::filesystem::directory_iterator(LidarDevice::configDir) )
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
  
  if ( checkPointMode & WriteCheckPoint )
  {
    if ( timestamp == 0 )
      timestamp = getmsec();

    std::filesystem::path path( LidarDevice::configDir );
    path += timestampString( "%Y%m%d-%H:%M:%S/", timestamp, false );

    if ( (checkPointMode&CreateCheckPoint) && !fileExists( path.c_str() ) )
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

  if ( !LidarDevice::configDirAlt.empty() )
  {
    std::string fn( LidarDevice::configDirAlt );
    fn += fileName;
    if ( suffix != NULL )
      fn += suffix;

    if ( fileExists( fn.c_str() ) )
      return fn;
  }
  
  std::string fn( LidarDevice::configDir );
  fn += fileName;
  if ( suffix != NULL )
    fn += suffix;
    
  return fn;
}


static std::string
applyDateToString( const char *string, uint64_t timestamp=0 )
{
  if ( strchr( string, '\%' ) == NULL )
    return std::string( string );
     
  if ( timestamp == (uint64_t)0 )
    timestamp = getmsec();
  
  time_t t = timestamp / 1000;
  struct tm timeinfo = *localtime( &t );

  const int maxLen = 2000;
  char buffer[maxLen+1];
  strftime( buffer, maxLen, string, &timeinfo );

//    printf( "templateToFileName %s %s %ld\n", buffer, logFileTemplate.c_str(), timestamp );

  return std::string( buffer );
}


float
LidarDevice::fileDriverPlayPos()
{ return g_FileDriverPlayPos;
}

int64_t
LidarDevice::fileDriverCurrentTime()
{ return g_FileDriverCurrentTime;
}

uint64_t
LidarDevice::fileDriverTimeStamp()
{
  if ( g_FileDriverTimeStamp == 0 )
    return 0;

  if ( g_FileDriverPaused )
    return g_FileDriverTimeStamp;
  
  return g_FileDriverTimeStamp + getmsec() - g_FileDriverTimeStampRef;
}

bool
LidarDevice::fileDriverIsPaused()
{ return g_FileDriverPaused;
}

bool
LidarDevice::fileDriverAtEnd()
{
  bool isOpen = false;
  bool isEof  = true;
  
  for ( int i = 0; i < g_DeviceList.size(); ++i )
  { LidarDevice &device( *g_DeviceList[i] );
    if ( device.isOpen() && device.inFile != NULL )
    {
      isOpen = true;
      if ( !device.inFile->is_eof() )
	isEof = false;
    }
  }

  return isOpen && isEof;
}

void
LidarDevice::setFileDriverPaused( bool paused )
{ g_FileDriverPaused = paused;
  if ( !paused )
    setFileDriverPlayPos( g_FileDriverPlayPos );
}

void
LidarDevice::setFileDriverPlayPos( float playPos )
{
  g_FileDriverPlayPos = playPos;

  uint64_t now = getmsec();
  
  for ( int i = ((int)g_DeviceList.size())-1; i >= 0; --i )
    g_DeviceList[i]->lock();

  uint64_t begin_time = 0;

  for ( int i = 0; i < g_DeviceList.size(); ++i )
  { LidarDevice &device( *g_DeviceList[i] );
    if ( device.isOpen(false) && device.inFile != NULL )
    { if ( i == g_FileDriverSyncIndex )
      { g_FileDriverCurrentTime   = device.inFile->play( playPos );
	g_FileDriverSyncTime      = now - g_FileDriverCurrentTime;
	device.inFile->start_time = g_FileDriverSyncTime;
	g_FileDriverPlayPos       = device.inFile->playPos();
	g_FileDriverTimeStamp     = device.inFile->timeStamp();
	g_FileDriverTimeStampRef  = getmsec();
	begin_time	   	  = device.inFile->begin_time;
      }
    }
  }
  
  for ( int i = 0; i < g_DeviceList.size(); ++i )
  { LidarDevice &device( *g_DeviceList[i] );
    if ( device.isOpen(false) && device.inFile != NULL )
    { if ( i != g_FileDriverSyncIndex )
      { device.inFile->start_time = g_FileDriverSyncTime;
	device.inFile->begin_time = begin_time;
	device.inFile->sync( g_FileDriverCurrentTime );
      }
    }
  }

  for ( int i = 0; i < g_DeviceList.size(); ++i )
  { g_DeviceList[i]->scanOnce = true;
    g_DeviceList[i]->unlock();
  }
}

void
LidarDevice::setFileDriverSyncTime( uint64_t timestamp )
{
  if ( timestamp == 0 )
    timestamp = getmsec();
  
  g_FileDriverSyncTime = timestamp;
  if ( g_FileDriverPlayPos >= 0.0 )
    g_FileDriverPlayPos = 0.0;
}

std::string
LidarDevice::getFileDriverFileName( const char *outFileTemplate, uint64_t timestamp )
{
  std::string fileName( outFileTemplate );
  
  std::string time( "%Y%m%d-%H:%M:%S" );
  time = applyDateToString( time.c_str(), g_FileDriverSyncTime );

  replace( fileName, "%default", "%time/%time_%nikname.lidar" );
  replace( fileName, "%time", time );
  replace( fileName, "%nikname", getNikName() );

  fileName = applyDateToString( fileName.c_str(), g_FileDriverSyncTime );
    
  return fileName;
}


static std::string
realPath( const char *path )
{
#ifdef PATH_MAX
  const int path_max = PATH_MAX;
#else
  int path_max = pathconf(path, _PC_PATH_MAX);
  if (path_max <= 0)
    path_max = 4096;
#endif
  
  char buf[path_max];
  char *res = realpath(path, buf);
  
  std::string result;
  if ( res != NULL )
    result = std::string( buf );
  
  return result;
  
}
/***************************************************************************
*** 
*** LIBUDEV
***
****************************************************************************/

#if USE_LIBUDEV
#include "libudev.h"
#endif

std::string
getUSBSerialNumber( const char *deviceName )
{
  std::string serialNumber;
  std::string devName( deviceName );
  
#if USE_LIBUDEV
#include "libudev.h"

  struct udev_device *dev;
  struct udev_enumerate *enumerate;
  struct udev_list_entry *list, *node;
  const char *path;

  struct udev *udev = udev_new();
  if (!udev) {
    Lidar::error( "can not create udev" );
    return 0;
  }

  enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "tty");
  udev_enumerate_scan_devices(enumerate);

  list = udev_enumerate_get_list_entry(enumerate);

  udev_list_entry_foreach(node, list)
  {
    path = udev_list_entry_get_name(node);
    dev = udev_device_new_from_syspath(udev, path);

    const char *serial      = udev_device_get_property_value(dev, "ID_SERIAL");
    const char *serialShort = udev_device_get_property_value(dev, "ID_SERIAL_SHORT");

    if ( serial != NULL || serialShort != NULL )
    {
      const char *devNode     = udev_device_get_devnode( dev );

      if ( devName == devNode )
      {
	serialNumber = serialShort;
	
	if ( g_Verbose )
	  printf("(%s) %s:   ID_SERIAL=%s ID_SERIAL_SHORT=%s\n", deviceName, devNode, serial, serialShort  );

	if ( serialNumber == "0000" || serialNumber == "0001" )
	  serialNumber = "";
      }
    }
    
    udev_device_unref(dev);
  }
  
  udev_unref( udev );

  return serialNumber;
#endif
}



/***************************************************************************
*** 
*** LidarDeviceList
***
****************************************************************************/

void
LidarDeviceList::addMember( LidarDevice *device )
{
  for ( int i = ((int)size())-1; i >= 0; --i )
    if ( device == (*this)[i] )
      return;
  
  push_back( device );
}

bool
LidarDeviceList::isMember( LidarDevice *device ) const
{
  for ( int i = ((int)size())-1; i >= 0; --i )
    if ( device == (*this)[i] )
      return true;
  
  return false;
}


/***************************************************************************
*** 
*** LidarDeviceGroup
***
****************************************************************************/

KeyValueMapDB LidarDeviceGroup::groups;
static KeyValueMapDB resolvedGroups;

bool
LidarDeviceGroup::write( const char *fileName )
{ 
  if ( !writeKeyValueMapDB( groups, fileName, "group", "member" ) )
  { Lidar::error( "failed to write LidarDeviceGroups file '%s'", fileName );
    return false;
  }
  
  if ( g_Verbose > 0 )
    Lidar::info( "writing LidarDeviceGroups file %s\n", fileName );
  
  return true;
}

bool
LidarDeviceGroup::read( const char *fileName, bool reportError )
{ 
  if ( !readKeyValueMapDB( groups, fileName, "group", "member" ) )
  { if ( reportError )
      Lidar::error( "failed to read LidarDeviceGroups file '%s'\n", fileName );
    return false;
  }
  
  if ( g_Verbose > 0 )
    Lidar::info( "reading LidarDeviceGroups file %s", fileName );
  
  return true;
}


void
LidarDeviceGroup::addDevice( const char *groupName, const char *deviceName )
{
  KeyValueMap map;
  groups.get( groupName, map );
  map.set( deviceName, "device" );
  groups.set( groupName, map );
}

void
LidarDeviceGroup::removeDevice( const char *groupName, const char *deviceName )
{
  std::string gn( groupName );
  
  if ( gn == "all" )
  {
    for ( KeyValueMapDB::iterator dbiter( LidarDeviceGroup::groups.begin() ); dbiter != LidarDeviceGroup::groups.end(); dbiter++ )
    { KeyValueMap &map( dbiter->second );
      map.remove( deviceName );
    }
  }
  else
  {
    KeyValueMap map;
    groups.get( groupName, map );
    map.remove( deviceName );
    groups.remove( groupName );
    groups.set( groupName, map );
  }
}

void
LidarDeviceGroup::removeDevice( const char *deviceName )
{ removeDevice( "all", deviceName );
}

void
LidarDeviceGroup::removeGroup( const char *groupName )
{ groups.remove( groupName );
}

void
LidarDeviceGroup::renameGroup( const char *oldName, const char *newName )
{ groups.rename( oldName, newName );
}

void
LidarDeviceGroup::clearGroups()
{ groups = KeyValueMapDB();
}

void
LidarDeviceGroup::renameDevice( const char *groupName, const char *oldName, const char *newName )
{ 
  std::string gn( groupName );
  
  if ( gn == "all" )
  {
    for ( KeyValueMapDB::iterator dbiter( LidarDeviceGroup::groups.begin() ); dbiter != LidarDeviceGroup::groups.end(); dbiter++ )
    { KeyValueMap &map( dbiter->second );
      map.rename( oldName, newName );
    }
  }
  else
  {
    KeyValueMap *map;
    if ( groups.get( groupName, map ) )
      map->rename( oldName, newName );
  }
}

void
LidarDeviceGroup::renameDevice( const char *oldName, const char *newName )
{ renameDevice( "all", oldName, newName );
}

void
LidarDeviceGroup::resolveDevices( void(*resolveDevice)( LidarDevice *device, std::string &deviceName ) )
{
  resolvedGroups.clear();
  
  for ( KeyValueMapDB::iterator dbiter( groups.begin() ); dbiter != groups.end(); dbiter++ )
  {
    KeyValueMap &map( dbiter->second );
    
    std::string groupName( dbiter->first );

    for ( KeyValueMap::iterator iter( map.begin() ); iter != map.end(); iter++ )
    {
      const std::string &value( iter->second );
      if ( value == "device" )
      {
	const std::string   &key( iter->first );
	std::string deviceName( key );
	std::string devName( key );

	LidarDevice *device = new LidarDevice();
 	resolveDevice( device, deviceName );
	std::string baseName( device->getBaseName() );
	delete device;

	if ( g_Verbose )
	{ std::string value;
	  if ( !map.get( baseName.c_str(), value ) )
	    Lidar::info( "groups: add alias: %s -> %s", devName.c_str(), baseName.c_str() );
	}
	resolvedGroups.set( groupName.c_str(), baseName.c_str(), "device" );
      }
    }
  }
}

/***************************************************************************
*** 
*** Image
***
****************************************************************************/

#ifdef VISUAL_DEBUG

#define cimg_use_jpeg
#define cimg_use_png
#include "CImg/CImg.h"
typedef cimg_library::CImg<unsigned char> rpImg;

static const unsigned char red[] = { 255,0,0,255,255 }, darkRed[] = { 160,0,0,255 }, green[] = { 0,255,0,255 }, darkGreen[] = { 0,160,0,255 }, blue[] = { 0,0,255,255 }, violet[] = { 128,0,255,255 }, yellow[] = { 255,255,0,255 }, white[] = { 255,255,255,255 }, darkerGray[] = { 50,50,50,255 }, darkGray[] = { 72,72,72,255 }, midGray[] = { 128,128,128,255 }, lightGray[] = { 192,192,192,255 };
  

static std::string title;
static rpImg *img = NULL; 
static Matrix3H matrix;
static float cx = 0, cy = 0;
static float extent = 6;
static float extent_x = 3, extent_y = 3;
static int   width = 800, height = 800;
static int   imgCount = 0;

inline void
imageGetCoord( int &x, int &y, float sx, float sy )
{
  Vector3D p( matrix * Vector3D(sx,sy,0.0) );
  
  x =  (p.x / extent_x) * width  + width/2;
  y = -(p.y / extent_y) * height + height/2;
}

static void
imageObjectColor( int objectId, unsigned char *color )
{
  float r = 255 * (((objectId>>0)&1)+0.5);
  float g = 255 * (((objectId>>1)&1)+0.5);
  float b = 255 * (((objectId>>2)&1)+0.5);

  color[0] = (r > 255 ? 255 : r);
  color[1] = (g > 255 ? 255 : g);
  color[2] = (b > 255 ? 255 : b);
  color[3] = 255;
}

static void
imagePaintObject( LidarObject &object, int objectId, const char *l=NULL )
{
  char label[100];

  unsigned char objColor[4];
  int x0, y0;
  int x1, y1;

  const int objectRadius = 4;

  imageObjectColor( objectId, objColor );
  
  imageGetCoord( x0, y0, object.lowerCoord[0], object.lowerCoord[1] );    
  img->draw_circle( x0, y0, objectRadius*1.5, objColor );
	
  imageGetCoord( x1, y1, object.higherCoord[0], object.higherCoord[1] );    
  img->draw_circle( x1, y1, objectRadius*1.5, objColor );

  img->draw_line( x0, y0, x1, y1, objColor );

  if ( l != NULL )
  {
    sprintf( label, "%s%d", l, objectId );
	
    img->draw_text( x0+4, y0+4, label, objColor, 0, 1, 16 );
  }
}


static void
imagePaintGrid()
{
  char label[100];

  int x, y;
  int x1, y1;
  int x0, y0;

  const int ticks = 20;

  imageGetCoord( x0, y0, 0.0, 0.0 );

  for ( int i = -ticks; i <= ticks; ++i )
  {
    imageGetCoord( x,  y,  -ticks*.5, i*.5 );
    imageGetCoord( x1, y1,  ticks*.5, i*.5 );
    if ( i == 0 )
    { img->draw_line( x,  y,  x0, y0, darkGreen );
      img->draw_line( x0, y0, x1, y1, green  );
    }
    else
      img->draw_line( x, y, x1, y1, (i%2 ? darkerGray : darkGray), 1, (i%2 ? 0xf9f9f9f9 : -1) );

    imageGetCoord( x,  y,  i*.5, -ticks*.5 );
    imageGetCoord( x1, y1, i*.5,  ticks*.5 );
    if ( i == 0 )
    { img->draw_line( x,  y,  x0, y0, darkRed );
      img->draw_line( x0, y0, x1, y1, red  );
    }
    else
      img->draw_line( x, y, x1, y1, (i%2 ? darkerGray : darkGray), 1, (i%2 ? 0xf9f9f9f9 : -1) );

    if ( i != 0 && (extent < 7 || i%2 == 0) )
    {
      imageGetCoord( x,  y,  i*.5, i*.5 );
      sprintf( label, "%gm", i * 0.5 );
	
      if ( i > 0 )
	img->draw_text( x+4, y+4, label, lightGray, 0, 1, 16 );
      else
	img->draw_text( x+4, y+4, label, midGray, 0, 1, 16 );
    }
  }
}

static void
imagePaintAxis()
{
  int x, y;

  const int axisLength  = 6;
  
  img->draw_line( width/2-axisLength, height/2, width/2+axisLength, height/2, violet );
  img->draw_line( width/2, height/2-axisLength, width/2, height/2+axisLength, red );
  
  imageGetCoord( x, y, 0.0, 0.0 );   

  img->draw_line( x-axisLength, y, x+axisLength, y, green  );
  img->draw_line( x, y-axisLength, x, y+axisLength, yellow );
}

static void
imageBegin()
{
  if ( img != NULL )
    delete img;
  
  img = new rpImg( width, height, 1, 3, 0);;

  if ( width > height )
  {
    extent_x = extent;
    extent_y = extent * height / (double) width;
  }
  else
  {
    extent_x = extent * width / (double) height;
    extent_y = extent;
  }

  imagePaintGrid();
  imagePaintAxis();
}

static void
imageEnd( const char *name )
{
  img->draw_text( 4, 4, LidarDevices::message.c_str(), white, 0, 1, 16 );

  char fileName[1000];
  sprintf( fileName, "%s_%03d.jpg", name, imgCount++ );

  Lidar::info( "saving %s", fileName );

  img->save( fileName );
}
  

#endif

/***************************************************************************
*** 
*** LidarObject
***
****************************************************************************/

float
LidarObject::lineScatter( LidarSampleBuffer &sampleBuffer ) const
{
  LidarSample &lowerSample( sampleBuffer[lowerIndex%sampleBuffer.size()] );

  Vector3D vec( sampleBuffer[higherIndex%sampleBuffer.size()].coord - lowerSample.coord );
  float lineLength = vec.length();

  vec /= lineLength;
  
  float sum = 0;
  int count = 0;
  
  for ( int angIndex = higherIndex-1; angIndex > lowerIndex; --angIndex )
  {
    LidarSample &sample( sampleBuffer[angIndex%sampleBuffer.size()] );

    if ( sample.isValid() )
    {
      Vector3D p( sample.coord - lowerSample.coord );
      float distance = p.product( vec ).length();
    
      sum   += distance;
      count += 1;
    }
  }

  if ( count > 0 )
  { sum /= count;
    sum /= lineLength;
  }
  
  return sum;
}

static bool
calcCurvature( float &curvature, LidarSampleBuffer &sampleBuffer, int lowerIndex, int higherIndex, float countWeight=0.5, std::vector<Vector2D> *curvePoints=NULL )
{
  curvature = 0;
  
  const int numSmoothed = 3;

  Vector2D sum;
  int count = 0;
  
  std::vector<Vector2D> smoothed;
  smoothed.resize( numSmoothed );
  int smoothedIndex = 0;
  
  for ( int angIndex = lowerIndex; angIndex <= higherIndex; ++angIndex )
  { LidarSample &sample( sampleBuffer[angIndex%sampleBuffer.size()] );
    if ( sample.isValid() )
    { 
      smoothed[smoothedIndex] = Vector2D(sample.coord.x, sample.coord.y);

      sum += smoothed[smoothedIndex];
      
      count += 1;
      
      if ( smoothedIndex == 0 )
	smoothedIndex = numSmoothed - 1;
    }
  }
  
  if ( count < 2 )
    return false;

  sum /= count;

  smoothed[1] = sum;

  if ( count > 2 )
  {
    Vector2D v0( smoothed[1] - smoothed[0] );
    Vector2D v1( smoothed[2] - smoothed[1] );

    v0.normalize();
    v1.normalize();

    Vector3D V0( v0 );
    Vector3D V1( v1 );

    Vector3D prod = V0.product( V1 );

    double  angle = prod.length();
  
    if ( prod.z < 0 )
      angle *= -1.0;

//  printf( "curv1: %g\n", angle );
    double curv = asin( angle ) / M_PI_2;
  
    const double maxCurvature = 0.75;
    curv /= maxCurvature;

    if ( curv > 1.0 )
      curv = 1.0;
    else if ( curv < 0.0 )
      curv = 0.0;

    curvature = curv;
  }
  else
    curvature = 0.0;
  
  if ( curvePoints != NULL )
    *curvePoints = smoothed;

  return true;
}

void
LidarObject::calcCurvature( LidarSampleBuffer &sampleBuffer )
{
  ::calcCurvature( curvature, sampleBuffer, lowerIndex, higherIndex, 0.5, &curvePoints );
}

/***************************************************************************
*** 
*** LidarObjects
***
****************************************************************************/

LidarObjects &
LidarObjects::operator +=( const Vector3D &offset )
{
  for ( int i = ((int)size())-1; i >= 0; --i )
    (*this)[i] += offset;
  
//  sortByAngle();

  return *this;
}

LidarObjects &
LidarObjects::operator *=( const Matrix3H &matrix )
{
  if ( matrix.isIdentity() )
    return *this;
  
  for ( int i = ((int)size())-1; i >= 0; --i )
    (*this)[i] *= matrix;

//  sortByAngle();

  return *this;
}


float
LidarObjects::angleOfMostDistantCoord() const
{
  float angle = 0;
  
  float maxDistance = 0.0;
  float distance;
  
  for ( int i = ((int)size())-1; i >= 0; --i )
  {
    const LidarObject &object( (*this)[i] );
    
    distance = object.lowerCoord.length();
    if ( distance > maxDistance )
    { angle = ((Vector2D&)object.lowerCoord).angle();
      maxDistance = distance;
    } 
    
    distance = object.higherCoord.length();
    if ( distance > maxDistance )
    { angle = ((Vector2D&)object.higherCoord).angle();
      maxDistance = distance;
    } 
  }

  return angle;
}

  
float 
LidarObjects::distance( const LidarObjects &other ) const
{
  float minDistance = 1000.0;

  if ( size() != other.size() )
    return minDistance;

  int index[other.size()];
  for ( int i = ((int)other.size())-1; i >= 0; --i )
    index[i] = i;

  std::sort( index, index+other.size() );

  do {
    float distance = 0.0;

    for ( int i = ((int)size())-1; i >= 0; --i )
      distance += (*this)[i].distance( other[index[i]] );
    
    if ( distance < minDistance )
      minDistance = distance;

  } while ( std::next_permutation(index,index+other.size()) );

  return minDistance;
}

Vector3D
LidarObjects::calcCenter() const
{
  Vector3D center;
  
  for ( int i = ((int)size())-1; i >= 0; --i )
    center += (*this)[i].center;
  
  if ( size() > 0 )
    center /= size();
  
  return center;
}

LidarObjects
LidarObjects::unscatter( LidarSampleBuffer &sampleBuffer ) const
{
  LidarObjects objects;
  
  const float maxLineScatter = 0.75;

  for ( int i = ((int)size())-1; i >= 0; --i )
  { float lineScatter = (*this)[i].lineScatter( sampleBuffer );
    if ( lineScatter <= maxLineScatter )
      objects.push_back( LidarObject( (*this)[i] ) );
    else if ( g_Verbose > 0 )
      Lidar::info( "removing object %d with linescatter %g > %g", i, lineScatter, maxLineScatter );
  }

  return objects;
}

void
LidarObjects::calcCurvature( LidarSampleBuffer &sampleBuffer )
{
  for ( int i = ((int)size())-1; i >= 0; --i )
    (*this)[i].calcCurvature( sampleBuffer );
}


void
LidarObjects::setTimeStamp( uint64_t timestamp )
{
  for ( int i = ((int)size())-1; i >= 0; --i )
    (*this)[i].timeStamp = timestamp;
}


static Matrix3H 
rotZMatrix( float angle )
{
  Matrix3H matrix;
  
  float cz = cos( -angle );
  float sz = sin( -angle );
  
  matrix.x.x =  cz;
  matrix.x.y =  sz;
  matrix.y.x = -sz;
  matrix.y.y =  cz;

  return matrix;
}

bool
LidarObjects::calcRotationTo( const LidarObjects &other, float &minAngle, float &minDistance, float angleOffset ) const
{
#ifdef VISUAL_DEBUG
  imageBegin();
#endif

  minDistance = 1000.0;
  
  if ( size() != other.size() )
    return false;

  LidarObjects me( *this );
  LidarObjects ot( other );

  float otAngle = ((Vector2D&)ot[0].center).angle();

  ot *= rotZMatrix( -otAngle );
  me *= rotZMatrix( -otAngle );

#ifdef VISUAL_DEBUG
  for ( int o = 0; o < ot.size(); ++o )
    imagePaintObject( ot[o], 0, "ot" );
#endif

  float angle       = angleOffset;
  
  for ( int i = 0; i < size(); ++i )
  {
    float meAngle = ((Vector2D&)me[i].center).angle();
    angle -= meAngle;
   
    me *= rotZMatrix( -meAngle+angleOffset );
    
#ifdef VISUAL_DEBUG
    for ( int m = 0; m < me.size(); ++m )
      imagePaintObject( me[m], i+1, "me" );
#endif

    float distance = me.distance( ot );
    distance *= distance;
    
    me *= rotZMatrix( -angleOffset );

    if ( distance < minDistance )
    { minAngle    = angle;
      minDistance = distance;
    }  
  }

/*
//  printf( "min1: %g %g\n", minAngle, minDistance );

  me = *this;
  me *= rotZMatrix( -otAngle + minAngle + M_PI );

  float distance = me.distance( other );
  distance *= distance;
    
  if ( distance < minDistance )
  { minAngle   += M_PI;
    minDistance = distance;
  }  
  
//  printf( "min2: %g %g\n", minAngle, distance );

  while ( minAngle < 0 )
    minAngle += 2.0 * M_PI;
  while ( minAngle > 2.0 * M_PI )
    minAngle -= 2.0 * M_PI;

#ifdef VISUAL_DEBUG
  me = *this;
  me *= rotZMatrix( -otAngle + minAngle );
  for ( int m = 0; m < me.size(); ++m )
    imagePaintObject( me[m], 3, "x" );

  char label[1000];
  sprintf( label, "min distance: %g", minDistance );
  img->draw_text( 4, height-15, label, white, 0, 1, 16 );

  imageEnd( "calcRotationTo" );
#endif
*/

  return true;
}

bool
LidarObjects::calcRotationRangeTo ( const LidarObjects &other, float &minAngle, float &minDistance, float angleRange, float &angleOffset, int numSamples ) const
{
  if ( size() != other.size() )
    return false;

  float offset = angleOffset;

  for ( int i = numSamples-1; i >= 0; --i )
  {
    float sampleAngleOffset = offset + -0.5 * angleRange + i * angleRange / numSamples;
    float angle, distance;
    
    calcRotationTo( other, angle, distance, sampleAngleOffset );
    if ( distance < minDistance )
    { minAngle    = angle;
      minDistance = distance;
      angleOffset = sampleAngleOffset;
    }  
    
    calcRotationTo( other, angle, distance, sampleAngleOffset+M_PI );
    if ( distance < minDistance )
    { minAngle    = angle;
      minDistance = distance;
      angleOffset = sampleAngleOffset+M_PI;
    }  
  }

  return true;
}

inline double PHI( double x )
{ return( x*(1.0 + sqrt(5.0)) / 2.0 ); }
   

bool
LidarObjects::calcTransformTo( const LidarObjects &other, Matrix3H &meMatrix, Matrix3H &otMatrix, float &minDistance ) const
{
  if ( size() != other.size() )
    return false;

  Vector3D meCenter( calcCenter() );
  Vector3D otCenter( other.calcCenter() );
  
  float minAngle;
  float maxRadius = 0.025;

  const int numSamples = 125;

  double radiusWeight = (numSamples > 1 ? maxRadius / sqrt( numSamples-1 ) : 0.0);

  for ( int i = 0; i < numSamples; ++i )
  {
    LidarObjects me( *this );
    LidarObjects ot( other );
  
    float angle  = PHI( i );
    float radius = sqrt(i) * radiusWeight;
    
    Matrix3H rotMatrix( rotZMatrix( angle ) );

    Vector3D meOffset( radius * cos(angle), radius * sin(angle), 0.0 );

    me += -meCenter + meOffset;
    ot += -otCenter;
    
    const float angleRange = 20.0 / 180.0 * M_PI;
//    const float angleRange = 0.0;
    float angleOffset = 0;
    float distance    = 1000.0;

    const int numSamples1 = 51;
    const int numSamples2 = 27;
    
    if ( me.calcRotationRangeTo( ot, angle, distance, angleRange, angleOffset, numSamples1 ) )
    { if ( me.calcRotationRangeTo( ot, angle, distance, angleRange/30, angleOffset, numSamples2 ) && distance < minDistance )
      { minAngle    = angle;
	minDistance = distance;

	otMatrix.w  = -otCenter;

	Matrix3H meCenterMatrix( -meCenter );
	Matrix3H rotMatrix( rotZMatrix( minAngle ) );
	meMatrix = rotMatrix * meCenterMatrix;
      }  
    }  
  }
  
  return true;
}

float
LidarObjects::Marker::calcTransformTo( const LidarObjects::Marker &other, Matrix3H &meMatrix, Matrix3H &otMatrix ) const
{
  float minDistance = 1000.0;

  std::string lastMessage = LidarDevices::message;

  for ( int me = 0; me < size(); ++me )
  {
    const LidarObjects &meObjects( (*this)[me] );

    for ( int ot = 0; ot < other.size(); ++ot )
    {
      const LidarObjects &otObjects( other[ot] );

      meObjects.calcTransformTo( otObjects, meMatrix, otMatrix, minDistance );

      char msg[200];
      sprintf( msg, "    marker(%d) -> marker(%d): %g\n", me, ot, minDistance );
      if ( g_Verbose > 0 )
	Lidar::info( "%s", msg );
      
      LidarDevices::message = lastMessage;
      LidarDevices::message += msg;
    }
  }

  LidarDevices::message = lastMessage;

  return minDistance;
}

LidarObjects::Marker
LidarObjects::getMarker( LidarSampleBuffer &sampleBuffer ) const
{
  LidarObjects meObjects( unscatter(sampleBuffer) );
  
  LidarObjects::Marker marker;
  
  for ( int me0 = 0; me0 < ((int)meObjects.size())-1; ++me0 )
  {
    for ( int me1 = me0+1; me1 < meObjects.size(); ++me1 )
    {
      float distance = (*this)[me0].center.distance( (*this)[me1].center );
      
//      printf( "me markerDistance: %g\n", distance );
      
      if ( distance < LidarObject::maxMarkerDistance )
      {
	marker.push_back( LidarObjects() );
	
	marker.back().push_back( LidarObject( (*this)[me0] ) );
	marker.back().push_back( LidarObject( (*this)[me1] ) );
	marker.back().sortByAngle();
      }
    }
  }
  
  return marker;
}


/***************************************************************************
*** 
*** LidarSample
***
****************************************************************************/

bool
LidarSample::read( std::ifstream &stream )
{
  stream >> coord.x >> coord.y >> angle >> distance >> quality;

  return true;
}


bool
LidarSample::write( std::ofstream &stream )
{
  stream << coord.x << " " << coord.y << " " << angle << " " << distance << " " << quality << "\n";

  return true;
}

/***************************************************************************
*** 
*** LidarSampleBuffer
***
****************************************************************************/

LidarSampleBuffer &
LidarSampleBuffer::operator +=( const Vector3D &offset )
{
  for ( int i = size()-1; i >= 0; --i )
    (*this)[i].coord += offset;

  return *this;
}

LidarSampleBuffer &
LidarSampleBuffer::operator *=( const Matrix3H &matrix )
{
  if ( matrix.isIdentity() )
    return *this;
  
  for ( int i = size()-1; i >= 0; --i )
    (*this)[i].coord *= matrix;

  return *this;
}

bool
LidarSampleBuffer::read( std::ifstream &stream )
{
  for ( int angIndex = 0; angIndex < size(); ++angIndex )
    if ( !(*this)[angIndex].read( stream ) )
      return false;

  return true;
}


bool
LidarSampleBuffer::read( const char *fileName )
{
  std::ifstream stream( fileName );
  if ( !stream.is_open() )
    return false;
  
  return read( stream );
}


bool
LidarSampleBuffer::write( std::ofstream &stream )
{
  for ( int angIndex = 0; angIndex < size(); ++angIndex )
    if ( !(*this)[angIndex].write( stream ) )
      return false;

  return true;
}

bool
LidarSampleBuffer::write( const char *fileName )
{
  std::ofstream stream( fileName );
  
  if ( !stream.is_open() )
    return false;
  
  return write( stream );
}

/***************************************************************************
*** 
*** LidarDevice
***
****************************************************************************/

static void runScanThread( LidarDevice *device )
{
  device->ThreadFunction();
}

LidarDevice::LidarDevice()
  : connectionType      ( UNKNOWN ),  
    driverType		( UNDEFINED ),  
    deviceType		(),  
    deviceName		(),  
    envFileName		(),  
    matrixFileName	(),
    baudrateOrPort	( 0 ),  
    motorPWM		( 0 ),  
    currentMotorPWM	( defaultMotorPWM ),
    motorSpeed		( 0 ),  
    currentMotorSpeed	( defaultMotorSpeed ),
    usePWM		( false ),  
    pwmChip		( 0 ),  
    pwmChannel		( 0 ),  
    ready		( false ),  
    isSimulationMode	( g_IsSimulationMode ),  
    motorState		( false ),  
    motorCtrlSupport	( false ),
    isPoweringUp	( false ),
    powerOff		( false ),
    dataReceived	( false ),
    errorMsg		(),
    sampleBufferIndex	( 0 ),
    samples   		( numSampleBuffers, LidarSampleBuffer(numSamples) ),
    objects   		(),
    oidCount  		( 1 ),
    oidMax    		( 99 ),
    envSamples		( numSamples ),
    envRawSamples	( numSamples ),
    envErodedSamples	( numSamples ),
    envDSamples		( numSamples ), 
    envTimeStamps       ( new uint64_t[numSamples] ),
    accumSamples	( numSamples ),
    rpSerialDrvStopped	( NULL ),
    rpSerialDrv		( NULL ),
    ydSerialDrv		( NULL ),
    ldSerialDrv		( NULL ),
    msSerialDrv		( NULL ),
    lsSerialDrv		( NULL ),
    inDrv		( NULL ),
    outDrv		( NULL ),
    inVirtSensorPower   ( false ),
    inFile		( NULL ),
    outFile		( NULL ),
    deviceId		( -1 ),
    char1		( 1.0 ),
    char2		( 0.0 ),
    matrix		(),
    matrixInverse	(),
    deviceMatrix	(),
    viewMatrix		(),
    mutex		(),
    thread		( NULL ),
    exitThread		( false ),
    shouldOpen		( false ),
    openFailed		( false ),
    useEnv    		( true ),
    useOutEnv		( true ),
    envOutDirty		( true ),
    dataValid 		( false ),
    envValid  		( false ),
    useTemporalDenoise	( true ),
    isAccumulating	( false ),
    isEnvScanning	( false ),
    envScanSec		( 15.0 ),
    envAdaptSec         ( 30.0 ),
    envFilterSize	( 0.75 ),
    envFilterMinDistance( 0.5 ),
    envThreshold 	( 0.2 ),
    objectMaxDistance 	( 0.35 ),
    objectMinExtent 	( 0.1 ),
    objectMaxExtent 	( 0.0 ),
    objectTrackDistance	( 0.5 ),
    doObjectDetection   ( false ),
    doObjectTracking    ( false ),
    doEnvAdaption       ( false ),
    scanOnce		( false ),
    reopenTime          ( 0 ),
    startTime           ( getmsec() )

{
  info.spec.maxRange = 100;
  
  g_DeviceList.push_back( this );
}


LidarDevice::~LidarDevice()
{
  bool is_open = isOpen();
  
  if ( thread != NULL )
  { 
    exitThread = true;
    thread->join();
    delete thread;
  }

  closeDevice();

  if ( rpSerialDrvStopped != NULL )
  { rpSerialDrvStopped->disconnect();
    RPlidarDriver::DisposeDriver( rpSerialDrvStopped );
    rpSerialDrvStopped = NULL;
  }
  
  delete envTimeStamps;

  g_DeviceList.erase( std::find(g_DeviceList.begin(), g_DeviceList.end(), this) );
  
  if ( is_open && g_Verbose >= 0 )
    Lidar::info( "LidarDevice(%s): shutting down %s", driverTypeString(), getBaseName().c_str() );

  if ( inDrv != NULL )
    delete inDrv;

  if ( outDrv != NULL )
    delete outDrv;

  if ( inFile != NULL )
    delete inFile;

  if ( outFile != NULL )
    delete outFile;
}

void
LidarDevice::setVerbose( int level )
{
  g_Verbose = level;
}

int
LidarDevice::verbose()
{
  return g_Verbose;
}

void
LidarDevice::lock()
{
  mutex.lock();
}


void
LidarDevice::unlock()
{
  mutex.unlock();
}

bool
LidarDevice::isLocalDevice() const
{
  return rpSerialDrv != NULL || ydSerialDrv != NULL || ldSerialDrv != NULL || msSerialDrv != NULL || lsSerialDrv != NULL;
}


bool
LidarDevice::isVirtualDevice() const
{
  return inDrv != NULL && inFile == NULL;
}


bool  
LidarDevice::isOpen( bool lockIt )
{
  if ( lockIt )
    lock();

  bool isOpen = ((inDrv != NULL && inDrv->isOpen) || (inFile != NULL && inFile->is_open()) || rpSerialDrv != NULL || ydSerialDrv != NULL || ldSerialDrv != NULL || msSerialDrv != NULL  || lsSerialDrv != NULL );

  if ( lockIt )
    unlock();
  
  return isOpen;
}

bool  
LidarDevice::isReady( bool lockIt )
{
  if ( lockIt )
    lock();

  bool isReady = ready;

  if ( lockIt )
    unlock();
  
  return isReady;
}

LidarSampleBuffer &
LidarDevice::sampleBuffer( int i ) const
{
  if ( i < 0 )
  {
    if ( isAccumulating )
      return (LidarSampleBuffer &) accumSamples;
    i = 0;
  }
  
  return (LidarSampleBuffer &) samples[(sampleBufferIndex+i)%numSampleBuffers];
}

bool
LidarDevice::setDeviceParam( std::map<std::string, std::string> map )
{
  bool success = true;
  
  std::map<std::string,std::string>::iterator iter;

  if ( (iter=map.find("baudrate")) != map.end() )
    baudrateOrPort = std::atoi( iter->second.c_str() );
  
  if ( (iter=map.find("deviceType")) != map.end() )
    success = (setDeviceDefaultParam( iter->second.c_str() ) && success);
  
  if ( (iter=map.find("pwm")) != map.end() )
    motorPWM = std::atoi( iter->second.c_str() );
  
  if ( (iter=map.find("speed")) != map.end() )
    motorSpeed = std::atof( iter->second.c_str() );
  
  getValue( map, "mode",     rplidar.scanMode );

  getValue( map, "char1", char1 );
  getValue( map, "char2", char2 );

  return success;
}

bool
LidarDevice::setDeviceDefaultParam( const char *type )
{
  deviceType = type;
  
  if ( deviceType.empty() )
    return true;
  
  if ( deviceType == "slamtec" )
    deviceType = RPLidarTypeName;
  else if ( deviceType == "ldrobot" )
    deviceType = LDLidarTypeName;

  bool success = true;

  if ( deviceType == RPLidarTypeName ||
       deviceType == "a1m8" ||
       deviceType == "a2m8" || 
       deviceType == "a2m7" ||
       deviceType == "a3m1" )
  {
    driverType = RPLIDAR;
    if ( deviceType == "a1m8" || deviceType == "a2m6" || deviceType == "a2m8" )
      baudrateOrPort = 115200;
    else if ( deviceType == "a3m1" || deviceType == "a2m7" )
      baudrateOrPort = 256000;
  }
  else if ( deviceType == LDLidarTypeName || 
	    deviceType == "st27" || 
	    deviceType == "ld06" || 
	    deviceType == "ld19" || 
	    deviceType == "ldp6" || 
	    deviceType == "lds6" )
  {
    driverType = LDLIDAR;
//    char1 = 0.999;
//    char2 = 0.0025;
    usePWM = (deviceType=="ldp6");
  }
  else if ( deviceType == MSLidarTypeName || 
	    deviceType == "ms200" )
  {
    driverType = MSLIDAR;
  }
  else if ( deviceType == LSLidarTypeName || 
	    deviceType == "m10" || 
	    deviceType == "n10" )
  {
    driverType = LSLIDAR;
  }
  else if ( deviceType == YDLidarTypeName ) // or the driver in general and ping the device
  {
    driverType = YDLIDAR;
  }
  else if ( YDLidarDriver::getSpec(deviceType.c_str()) != NULL ) // search for all ydlidar devices
  {
    driverType = YDLIDAR;
    ydlidar = YDLidarParam( deviceType.c_str() );
    if ( ydlidar.isSerial() )
      baudrateOrPort = ydlidar.baudrate;
  }
  else
  {
    success = false;
  }
  
  return success;
}

std::map<std::string, std::string> 
LidarDevice::readDeviceParam( const char *type )
{
  std::string fileName( getConfigFileName(type,".txt") );

  std::ifstream stream( fileName );
  if ( !stream.is_open() )
  { std::map<std::string, std::string> result;
    return result;
  }
  
  return readKeyValuePairs( stream );
}

void
LidarDevice::setSpec( const char *type, float maxRange, int numSamples, float scanFreq )
{
  info.spec.maxRange   = maxRange;
  info.spec.numSamples = numSamples;
  info.spec.scanFreq   = scanFreq;

  std::map<std::string, std::string> map( readDeviceParam( type ) );

  float value;
  if ( getValue( map, "maxRange", value ) )
    info.spec.maxRange   = value;

  if ( g_Verbose > 0 )
    Lidar::info( "LidarDevice::setSpec(%s): type=%s range=%g samples=%d freq=%g minQ=%d envMinQ=%d", info.detectedDeviceType.c_str(), type, info.spec.maxRange, info.spec.numSamples, info.spec.scanFreq, info.spec.minQuality, info.spec.envMinQuality );

  if ( envValid )
  { updateEnv();
    processEnv();
    envChanged();
  }
}


void
LidarDevice::setSpec( int driverType, const char *deviceType )
{
  switch ( driverType )
  {
    case LDLIDAR:
    {
      if ( strcmp(deviceType,"st27") == 0 )
	setSpec( deviceType, 22.0, 2160, 10 );
      else
      {
	setSpec( deviceType, 9.0, usePWM ? 580:455, usePWM ? 7.1:10 );

//	info.spec.minQuality    = ld06MinQuality;
//	info.spec.envMinQuality = ld06EnvMinQuality;
      }
      
      break;
    }
    case MSLIDAR:
    {
      motorSpeed              = 10.0;
      setSpec( deviceType, 9.0, 448, 10 );
      break;
    }
    case LSLIDAR:
    {
      if ( strcmp(deviceType,"m10") == 0 )
	setSpec( info.detectedDeviceType.c_str(), 15.0, 1008, 10 );
      else if ( strcmp(deviceType,"n10") == 0 )
      {
	info.detectedDeviceType = "n10";
	info.spec.numSamples = 450;
	info.spec.scanFreq   = 6;
	setSpec( deviceType, 6.0, 450, 7 );
      }
      break;
    }
    
    case YDLIDAR:
    {
      YDLidarDeviceSpec *spec = YDLidarDriver::getSpec( deviceType );
      if ( spec != NULL )
      { 
	float maxRange = spec->maxRange * 0.75;
	setSpec( deviceType, maxRange, spec->defaultSampleRate * 1000 / spec->defaultFrequency, spec->defaultFrequency );

	if ( deviceType == "tmini" )
        { info.spec.minQuality    = ld06MinQuality;
	  info.spec.envMinQuality = ld06EnvMinQuality;
	}
      }
      break;
    }  
    case RPLIDAR:
    {
      if ( strcmp(deviceType,"a1m8") == 0 )
	setSpec( deviceType, 12.0, 1000, 7.5 );
      else if ( strcmp(deviceType,"a3m1") == 0 )
	setSpec( deviceType, 13.0, 1250, 17 );
      else 
	setSpec( deviceType, 10.0, 1000, 7.5 );
      break;
    }
      
    default:
      break;
  }
}


void
LidarDevice::setSpec( const char *deviceType )
{
  YDLidarDeviceSpec *spec = YDLidarDriver::getSpec( deviceType );
  if ( spec != NULL )
    setSpec( YDLIDAR, deviceType );
  else if ( strcmp(deviceType,"st27") == 0 ||
	    strcmp(deviceType,"ld06") == 0 ||
	    strcmp(deviceType,"ld19") == 0 ||
	    strcmp(deviceType,"ldp6") == 0 ||
	    strcmp(deviceType,"lds6") == 0 )
    setSpec( LDLIDAR, deviceType );
  else if ( strcmp(deviceType,"ms200") == 0 )
    setSpec( MSLIDAR, deviceType );
  else if ( strcmp(deviceType,"m10") == 0 || 
	    strcmp(deviceType,"n10") == 0 )
    setSpec( LSLIDAR, deviceType );
  else
    setSpec( RPLIDAR, deviceType );
}


bool
LidarDevice::readDeviceParam( std::ifstream &stream )
{
  std::map<std::string, std::string> map( readKeyValuePairs( stream ) );

  bool success = setDeviceParam( map );

  return success;
}

bool
LidarDevice::setDeviceType( const char *type )
{
  bool success = setDeviceDefaultParam( type );
  
  std::string fileName( getConfigFileName(getDeviceType().c_str(),".txt") );

  std::ifstream stream( fileName );
  if ( stream.is_open() )
    success = readDeviceParam( stream );
  
  if ( !success )
    Lidar::error( "setting device type: unknown device type: '%s'", deviceType.c_str() );

  return success;
}

std::string
LidarDevice::getDeviceType() const
{
  if ( !info.detectedDeviceType.empty() )
    return info.detectedDeviceType;

  if ( !deviceType.empty() )
    return deviceType;
 
  return defaultDeviceType;
}

const char *
LidarDevice::driverTypeString( int type )
{
  switch( type )
  {
    case RPLIDAR:
      return RPLidarTypeName;
    case YDLIDAR:
      return YDLidarTypeName;
    case LDLIDAR:
      return LDLidarTypeName;
    case MSLIDAR:
      return MSLidarTypeName;
    case LSLIDAR:
      return LSLidarTypeName;
    default:
      return UndefinedTypeName;
  }
}

const char *
LidarDevice::driverTypeString() const
{
  return driverTypeString( driverType );
}

std::string &
LidarDevice::getVirtualHostName()
{
  if ( !inVirtHostName.empty() )
  {
    Url url( inVirtHostName );
    if ( url.isOk() )
      inVirtHostName = url.hostname;
  }
  
  return inVirtHostName;
}


std::string
LidarDevice::getBaseName( std::string &deviceName, const std::string inVirtUrl, bool asFileName )
{
#ifdef _WIN32
  std::string delimiter = "\\";
#else
  std::string delimiter = "/";
#endif

  if ( deviceName.empty() && inVirtUrl.empty() )
    deviceName = getDefaultSerialDevice();

  if ( deviceName.empty() )
  {
    if ( !inVirtUrl.empty() )
    {
      LidarUrl url( inVirtUrl.c_str() );
  
      std::string delimiter( asFileName ? "_" : ":" );

      if ( url.isOk() )
      {
	std::string baseName( asFileName ? "virtual_" : "" );
      
	if ( !asFileName && !url.hostname.empty() )
	  baseName += url.hostname + delimiter;
      
	baseName += std::to_string( url.port );

	return baseName;
      }
    }

    deviceName = getDefaultSerialDevice();
  }
  else 
  {
    if ( std::isdigit( deviceName[0] ) )
    { int id = std::atoi( deviceName.c_str() );
      deviceName = getDefaultSerialDevice( id );
    }
  }
  
  std::string baseName = deviceName;

  size_t pos = 0;
  std::string token;
  while ((pos = baseName.find(delimiter)) != std::string::npos) {
    token = baseName.substr(0, pos);
    baseName.erase(0, pos + delimiter.length());
  }

  return baseName;
}


std::string
LidarDevice::getBaseName( std::string &deviceName, bool asFileName )
{ const std::string inVirtUrl;
  return getBaseName( deviceName, inVirtUrl, asFileName );
}


std::string
LidarDevice::getBaseName( bool asFileName )
{
  if ( !baseName.empty() )
    return baseName;
  
  return getBaseName( deviceName, inVirtUrl, asFileName );
}


std::string
LidarDevice::getNikName( bool asFileName )
{
  if ( !nikName.empty() )
    return nikName;
  
  return getBaseName( asFileName );
}

std::string
LidarDevice::getIdName()
{
#ifdef _WIN32
  std::string delimiter = "\\";
#else
  std::string delimiter = "/";
#endif

  if ( !idName.empty() )
    return idName;
  
  if ( !deviceName.empty() || (inVirtUrl.empty() && inFileName.empty()) )
  { idName = getNikName();
    return idName;
  }
  
  LidarUrl url( inVirtUrl.c_str() ); 
  if ( !url.isOk() )
  { idName = getNikName();
    return idName;
  }

  idName = std::to_string( url.port );

  return idName;
}


std::string
LidarDevice::getEnvFileName()
{
  if ( !envFileName.empty() )
    return envFileName;

  envFileName = "LidarEnv_" + getNikName(true);
  
  if ( isSimulationMode )
    envFileName += "_Simulation";
  
  envFileName += ".txt";

//  std::cout << envFileName << std::endl;

  return envFileName;
}


std::string
LidarDevice::getMatrixFileName()
{
  if ( !matrixFileName.empty() )
    return matrixFileName;

  matrixFileName = "LidarMatrix_" + getNikName(true) + ".txt";

//  std::cout << matrixFileName << std::endl;

  return matrixFileName;
}

void
LidarDevice::setInstallDir( const char *path )
{
  installDir = path;
}


LDLidarDriver *
LidarDevice::openLDLidarDriver( const char *model )
{
  LDLidarDriver *ldSerialDrv = new LDLidarDriver( usePWM, pwmChip, pwmChannel );

  if ( !isSimulationMode && !ldSerialDrv->connect( deviceName.c_str(), model ) )
  { delete ldSerialDrv;
    return NULL;
  }

  uint64_t startMSec = getmsec();
  ScanData laserScan;

  bool success = isSimulationMode;
  while ( !success && getmsec()-startMSec < 250 )
  { usleep( 10000 );
    success = ldSerialDrv->grabScanData( laserScan );
  }

  if ( !success )
  { ldSerialDrv->disconnect();
    delete ldSerialDrv;
    return NULL;
  }
  
  return ldSerialDrv;
}

LidarDevice::ConnectionType
LidarDevice::getConnectionType( const char *devName )
{
  if ( connectionType != UNKNOWN )
    return connectionType;
  
  if ( devName == NULL )
    devName = deviceName.c_str();

  std::string deviceName = resolveDeviceName( devName );

  if ( deviceId == -1 )
    deviceId = deviceName.c_str()[deviceName.length()-1]-'0';

  std::string devicePath( realPath(deviceName.c_str()) );

  if ( startsWith( devicePath, "/dev/ttyS" ) )
    connectionType = UART;
  else if ( startsWith( devicePath, "/dev/ttyUSB" ) || startsWith( devicePath, "/dev/ttyACM" ))
    connectionType = USB;
  
  return connectionType;
}

bool 
LidarDevice::setUARTPower( bool on, const char *devName )
{
//  printf( "setUARTPower 1 %d\n", on );
  
  if ( isSimulationMode || !isPoweringSupported() )
    return false;
    
  if ( getConnectionType( devName ) != UART )
    return false;
  
  isPoweringUp = on;

  std::string cmd( hardwareDir );
  cmd += "lidarPower.sh ";
  cmd += (on ? "on" : "off");
  system( cmd.c_str() );

  isPoweringUp = false;

  return true;
}
  
bool 
LidarDevice::openDeviceLDLidar( bool tryOpen )
{
  deviceName = resolveDeviceName( deviceName.c_str() );

  if ( deviceId == -1 )
    deviceId = deviceName.c_str()[deviceName.length()-1]-'0';

  if ( g_Verbose > 0 )
    Lidar::info( "LidarDevice(%s): opening %s device %s", LDLidarTypeName, isSimulationMode?"simulation":"serial", deviceName.c_str() );

  std::string pwmDir( "/sys/class/pwm/pwmchip" );
  pwmDir += std::to_string( pwmChip );

  std::string gpioPowerDir( "/sys/class/gpio/pwmchip" );
  pwmDir += std::to_string( pwmChip );

  bool isUART = (getConnectionType( deviceName.c_str() ) == UART);

  if ( isUART )
  {
    if ( fileExists( pwmDir.c_str() ) )
    {
    //    usePWM = true;
      if ( g_Model == "RockPiS" )
	pwmChip = 2;
    }
  }
  
  LDLidarDriver::setVerbose( g_Verbose );

  ldSerialDrv = openLDLidarDriver( deviceType.c_str() );

  if ( ldSerialDrv == NULL && tryOpen && (deviceType.empty() || deviceType == LDLidarTypeName) )
    ldSerialDrv = openLDLidarDriver( "st27" );

  if ( ldSerialDrv == NULL )
  {
    if ( !tryOpen )
      Lidar::error( "LidarDevice(%s)::open(%s) failed !!!", LDLidarTypeName, deviceName.c_str() );

    errorMsg = "open failed";
    return false;
  }

  if ( g_Verbose > 0 )
  { std::string version( ldSerialDrv->sdkVersion() );
    Lidar::info("LDLIDAR Version: %s", version.c_str() );
    Lidar::info( "LidarDevice(%s)::open(%s) succeeded", LDLidarTypeName, deviceName.c_str() );
  }
  
  motorState = true;

  info.detectedDeviceType = ldSerialDrv->model;
  if ( info.detectedDeviceType != "st27" && info.detectedDeviceType != "stl27l" )
  { if ( usePWM )
      info.detectedDeviceType = "ldp6";
    else if ( isUART )
      info.detectedDeviceType = "lds6";
  }
  
  info.detectedDriverType = driverType = LDLIDAR;

  if ( deviceType.empty() )
    setDeviceType( info.detectedDeviceType.c_str() );

  setSpec( LDLIDAR, info.detectedDeviceType.c_str() );

  errorMsg = "";

  return true;  
}

void 
LidarDevice::closeDeviceLDLidar()
{
  if ( ldSerialDrv == NULL )
    return;

  if ( g_Verbose > 0 )
    Lidar::info( "LidarDevice(%s): closing device %s", LDLidarTypeName, deviceName.c_str() );

  lock();
  delete ldSerialDrv;
  ldSerialDrv = NULL;
  unlock();
}

bool 
LidarDevice::openDeviceLSLidar( bool tryOpen )
{
  deviceName = resolveDeviceName( deviceName.c_str() );

  if ( deviceId == -1 )
    deviceId = deviceName.c_str()[deviceName.length()-1]-'0';

  if ( g_Verbose > 0 )
  { Lidar::info("LSLidar Version: %s", "lsm10_v1_0" );
    Lidar::info( "LidarDevice(%s): opening %s device %s", LSLidarTypeName, isSimulationMode?"simulation":"serial", deviceName.c_str() );
  }

  lsSerialDrv = new LSLidarDriver();

  if ( deviceType == "m10" )
    lsSerialDrv->model = LSLidarDriver::M10;
  else if ( deviceType == "n10" )
    lsSerialDrv->model = LSLidarDriver::N10;
  else
    lsSerialDrv->model = LSLidarDriver::Undefined;
  
  if ( !isSimulationMode && !lsSerialDrv->connect( deviceName.c_str(), tryOpen ) )
  { 
    if ( !tryOpen )
    { Lidar::error( "LidarDevice(%s)::open(%s) failed !!!", LSLidarTypeName, deviceName.c_str() );
      errorMsg = "open failed";
    }
    delete lsSerialDrv;
    lsSerialDrv = NULL;
    return false;
  }

  uint64_t startMSec = getmsec();
  ScanData laserScan;

  bool success = isSimulationMode;
  while ( !success && getmsec()-startMSec < 300 )
  { usleep( 10000 );
    success = lsSerialDrv->grabScanData( laserScan );
  }

  if ( !success )
  {
    lsSerialDrv->disconnect();
    delete lsSerialDrv;
    lsSerialDrv = NULL;
    if ( !tryOpen )
    { if ( g_Verbose > 0 )
	Lidar::error( "LidarDevice(%s)::open(%s) failed ", LSLidarTypeName, deviceName.c_str());
      errorMsg = "open failed";
    }
    return false;
  }
  
  if ( g_Verbose > 0 )
    Lidar::info( "LidarDevice(%s)::open(%s) succeeded", LSLidarTypeName, deviceName.c_str() );

  motorState = true;

  if ( lsSerialDrv->model == LSLidarDriver::M10 )
  {
    info.detectedDeviceType = "m10";
//    setSpec( info.detectedDeviceType.c_str(), 15.0, 1008, 10 );
  }
  else if ( lsSerialDrv->model == LSLidarDriver::N10 )
  {
    info.detectedDeviceType = "n10";
//    info.spec.numSamples = 450;
//    info.spec.scanFreq   = 6;
//    setSpec( info.detectedDeviceType.c_str(), 6.0, 450, 7 );
  }
  setSpec( LSLIDAR, info.detectedDeviceType.c_str() );

  deviceType = info.detectedDeviceType;

  info.detectedDriverType = driverType = LSLIDAR;

  errorMsg = "";
  
  return true;  
}

void 
LidarDevice::closeDeviceLSLidar()
{
  if ( lsSerialDrv == NULL )
    return;

  if ( g_Verbose > 0 )
    Lidar::info( "LidarDevice(%s): closing device %s", LSLidarTypeName, deviceName.c_str() );

  lock();
  delete lsSerialDrv;
  lsSerialDrv = NULL;
  unlock();
}

bool 
LidarDevice::openDeviceMSLidar( bool tryOpen )
{
  deviceName = resolveDeviceName( deviceName.c_str() );

  if ( deviceId == -1 )
    deviceId = deviceName.c_str()[deviceName.length()-1]-'0';

  if ( g_Verbose > 0 )
  { // Lidar::info("MSLidar Version: %s", "lsm10_v1_0" );
    Lidar::info( "LidarDevice(%s): opening %s device %s", MSLidarTypeName, isSimulationMode?"simulation":"serial", deviceName.c_str() );
  }

  msSerialDrv = new MSLidarDriver();

  if ( !isSimulationMode && !msSerialDrv->connect( deviceName.c_str() ) )
  { 
    if ( !tryOpen )
    { Lidar::error( "LidarDevice(%s)::open(%s) failed !!!", MSLidarTypeName, deviceName.c_str() );
      errorMsg = "open failed";
    }
    delete msSerialDrv;
    msSerialDrv = NULL;
    return false;
  }


  if ( g_Verbose > 0 )
    Lidar::info( "LidarDevice(%s)::open(%s) succeeded", MSLidarTypeName, deviceName.c_str() );

  currentMotorSpeed = 7.0;

  if ( !isSimulationMode )
  { 
    bool success = false;

    if ( motorSpeed > 0.0 )
    { success = msSerialDrv->setRotationSpeed( currentMotorSpeed=motorSpeed );
      if ( !success )
      { if ( !tryOpen )
        { Lidar::error( "LidarDevice(%s)::open(%s) set motor speed failed !!!", MSLidarTypeName, deviceName.c_str() );
	  errorMsg = "open start motor failed";
	}

	delete msSerialDrv;
	msSerialDrv = NULL;
	return false;
      }

     motorSpeed = 0;
    }
    
    success = msSerialDrv->startMotor();

    if ( !success )
    { if ( !tryOpen )
      { Lidar::error( "LidarDevice(%s)::open(%s) start motor failed !!!", MSLidarTypeName, deviceName.c_str() );
	errorMsg = "open start motor failed";
      }

      delete msSerialDrv;
      msSerialDrv = NULL;
      return false;
    }
  }
  
  info.detectedDeviceType = "ms200";
  setSpec( MSLIDAR, info.detectedDeviceType.c_str() );
  setDeviceType( info.detectedDeviceType.c_str() );

  info.detectedDriverType = driverType = MSLIDAR;

  motorStartTime = getmsec();

  motorState = true;

  errorMsg = "";
  
  return true;  
}

void 
LidarDevice::closeDeviceMSLidar()
{
  if ( msSerialDrv == NULL )
    return;

  if ( g_Verbose > 0 )
    Lidar::info( "LidarDevice(%s): closing device %s", MSLidarTypeName, deviceName.c_str() );

  lock();
  msSerialDrv->stopMotor();
  delete msSerialDrv;
  msSerialDrv = NULL;
  unlock();
}

static void
YDLidarGetInfo( YDLidarDriver *ydSerialDrv, rplidar_response_device_info_t &devinfo )
{
  std::memcpy( devinfo.serialnum, ydSerialDrv->getSerialNumber().c_str(), 16 );
  devinfo.model            = ydSerialDrv->getModel();
  devinfo.firmware_version = ydSerialDrv->getFirmwareVersion();
  devinfo.hardware_version = ydSerialDrv->getHardwareVersion();
}

bool 
LidarDevice::openDeviceYDLidar( bool tryOpen )
{
  bool pingDevice = tryOpen;
  if ( deviceType == YDLidarTypeName )
    pingDevice = true;

  deviceName = resolveDeviceName( deviceName.c_str() );

  if ( deviceId == -1 )
    deviceId = deviceName.c_str()[deviceName.length()-1]-'0';

  ydSerialDrv = new YDLidarDriver();
  ydSerialDrv->setVerbose( g_Verbose > 0 );
  ydSerialDrv->param = ydlidar;

  if ( baudrateOrPort != 0 )
    ydSerialDrv->param.baudrate = baudrateOrPort;

  if ( !isSimulationMode && pingDevice )
  { int model, firmware_version, hardware_version;
    if ( ydSerialDrv->pingDeviceInfo( deviceName.c_str(), model, firmware_version, hardware_version, info.devinfo.serialnum ) )
    { YDLidarDeviceSpec *spec = YDLidarDriver::getSpec( model );
      if ( spec != NULL )
      { info.detectedDeviceType = spec->model;
	tolower( info.detectedDeviceType );
	setDeviceType( info.detectedDeviceType.c_str() );
      }
    }
    else
    {
      delete ydSerialDrv;
      ydSerialDrv = NULL;
      return false;
    }

    ydSerialDrv->param = ydlidar;
  }
  
  bool success = (isSimulationMode || ydSerialDrv->connect( deviceName.c_str() ));

  if ( !success )
  { if ( !tryOpen )
      Lidar::error( "LidarDevice(%s)::open(%s) failed !!!", YDLidarTypeName, deviceName.c_str() );
    errorMsg = "open failed";

    delete ydSerialDrv;
    ydSerialDrv = NULL;
    return false;
  }
  
  if ( !isSimulationMode )
  {
    YDLidarGetInfo( ydSerialDrv, info.devinfo );

    YDLidarDeviceSpec *spec = YDLidarDriver::getSpec( info.devinfo.model );
    if ( spec != NULL )
    { info.detectedDeviceType = spec->model;
      tolower( info.detectedDeviceType );
    }
    else
      info.detectedDeviceType == YDLidarTypeName;

    info.detectedDriverType = driverType = YDLIDAR;

    if ( g_Verbose > 0 )
    { Lidar::info("YDLIDAR Version: %s", ydSerialDrv->getSDKVersion().c_str());
      Lidar::info( "LidarDevice(%s): opening %s device %s", YDLidarTypeName, isSimulationMode?"simulation":"serial", deviceName.c_str() );
      dumpInfo( info );
    }
  }
  
  if ( powerOff )
  {
    usleep( 500 * 1000 );
    ydSerialDrv->stopMotor();
    
    if ( g_Verbose > 0 )
      Lidar::info( "%s POWEROFF", deviceName.c_str() );

    ydSerialDrv->disconnect();

    delete ydSerialDrv;
    ydSerialDrv = NULL;
	
    return false;
  }

  if ( g_Verbose > 0 )
    Lidar::info( "LidarDevice(%s)::open(%s) succeeded with baudrate %d", YDLidarTypeName, deviceName.c_str(), ydSerialDrv->param.baudrate );

  if ( !isSimulationMode )
    ydSerialDrv->startMotor();

  motorStartTime = getmsec();

  motorState = true;

  if ( deviceType.empty() )
//    deviceType = info.detectedDeviceType;
    setDeviceType( info.detectedDeviceType.c_str() );

  YDLidarDeviceSpec *spec = YDLidarDriver::getSpec( deviceType.c_str() );
  if ( spec != NULL )
  { 
    info.detectedDeviceType = spec->model;
    tolower( info.detectedDeviceType );
    setDeviceType( info.detectedDeviceType.c_str() );

//    float maxRange = spec->maxRange * 0.75;
//    setSpec( info.detectedDeviceType.c_str(), maxRange, spec->defaultSampleRate * 1000 / spec->defaultFrequency, spec->defaultFrequency );

    setSpec( YDLIDAR, info.detectedDeviceType.c_str() );
  }

  errorMsg = "";
  
  return true;  
}

void 
LidarDevice::closeDeviceYDLidar()
{
  if ( ydSerialDrv == NULL )
    return;

  if ( g_Verbose > 0 )
    Lidar::info( "LidarDevice(%s): closing device %s", YDLidarTypeName, deviceName.c_str() );

  lock();
  if ( !isSimulationMode )
    ydSerialDrv->stopMotor();
  delete ydSerialDrv;
  ydSerialDrv = NULL;
  unlock();
}

static void
printScanMode( rp::standalone::rplidar::RplidarScanMode &scanMode )
{
  printf( "Mode: %d\n"
	  "	us per sample: %g\n"
	  "	max distance:  %g\n"
	  "	answer type:   %d\n"
	  "	%s\n"
	  , scanMode.id
	  , scanMode.us_per_sample
	  , scanMode.max_distance
	  , scanMode.ans_type
	  , scanMode.scan_mode
    );
}

void
LidarDevice::dumpInfo( Info &info )
{
  char devType[100] = "";
  char driverType[100] = "";

  if ( !info.detectedDeviceType.empty() )
    sprintf( devType, " (%s)", info.detectedDeviceType.c_str() );

  if ( info.detectedDriverType != UNDEFINED )
    sprintf( driverType, " (%s)", driverTypeString( info.detectedDriverType ) );
  
  if ( info.detectedDriverType == YDLIDAR || info.detectedDriverType == RPLIDAR )
    printf("S/N          : " );

  if ( info.detectedDriverType == YDLIDAR )
  { for (int pos = 0; pos < 16 ;++pos)
      printf("%c", info.devinfo.serialnum[pos]);
  }
  else if ( info.detectedDriverType == RPLIDAR )
  { for (int pos = 0; pos < 16 ;++pos)
      printf("%02X", info.devinfo.serialnum[pos]);
  }

  if ( info.detectedDriverType == LDLIDAR || info.detectedDriverType == LSLIDAR || info.detectedDriverType == MSLIDAR )
    printf( "Model        : %s%s\n", devType, driverType );
  else if ( info.detectedDriverType == YDLIDAR || info.detectedDriverType == RPLIDAR )
    printf("\n"
	   "Model        : %d%s%s\n"
	   "Firmware Ver : %d.%02d\n"
	   "Hardware Rev : %d\n"
	   , info.devinfo.model
	   , devType
	   , driverType
	   , info.devinfo.firmware_version>>8
	   , info.devinfo.firmware_version & 0xFF
	   , (int)info.devinfo.hardware_version);
}

bool
LidarDevice::guessDeviceTypeRplidar( Info &info )
{
  if ( info.devinfo.model == 1*16+8 )
    info.detectedDeviceType = "a1m8";
  else if ( info.devinfo.model == 2*16+6 )
    info.detectedDeviceType = "a2m6";
  else if ( info.devinfo.model == 2*16+7 )
    info.detectedDeviceType = "a2m7";
  else if ( info.devinfo.model == 2*16+8 )
    info.detectedDeviceType = "a2m8";
  else if ( info.devinfo.model == 3*16+1 )
    info.detectedDeviceType = "a3m1";
  else
    info.detectedDeviceType = RPLidarTypeName;

  return true;
}

bool 
LidarDevice::openDeviceRplidar( bool tryOpen )
{
  bool pingDevice = tryOpen;
  if ( deviceType == RPLidarTypeName || deviceType == "slamtec" )
    pingDevice = true;

  deviceName = resolveDeviceName( deviceName.c_str() );

  if ( deviceId == -1 )
    deviceId = deviceName.c_str()[deviceName.length()-1]-'0';

  u_result     op_result;

  if ( g_Verbose > 0 )
    Lidar::info("RPLIDAR Version: %d.%d.%d", SL_LIDAR_SDK_VERSION_MAJOR, SL_LIDAR_SDK_VERSION_MINOR, SL_LIDAR_SDK_VERSION_PATCH );

  bool connectSuccess = false;

  int size = (pingDevice ? sizeof(baudrateArray)/sizeof(baudrateArray[0]) : 0);
  if ( g_Verbose > 0 )
    Lidar::info( "LidarDevice(%s): opening %s device %s", RPLidarTypeName, isSimulationMode?"simulation":"serial", deviceName.c_str() );

  for ( int i = size-1 + (baudrateOrPort>0); !connectSuccess && i >= 0; --i )
  {
	// create the driver instance
    rpSerialDrv = RPlidarDriver::CreateDriver(CHANNEL_TYPE_SERIALPORT);

    if (!rpSerialDrv) {
      Lidar::error( "LidarDevice(%s)::open(%s) insufficent memory, exit", RPLidarTypeName, deviceName.c_str() );
      errorMsg = "insufficent memory";
      exit(-2);
    }

    if ( isSimulationMode )
      connectSuccess = true;
    else
    {
      _u32 brate = (i == size ? baudrateOrPort : baudrateArray[i]);
      int readPerm = access (deviceName.c_str(), R_OK);
      if ( readPerm == 0 && IS_OK(rpSerialDrv->connect(deviceName.c_str(), brate)) )
      {
	if ( powerOff )
        {
	  usleep( 500 * 1000 );
	  rpSerialDrv->stopMotor();

	  if ( g_Verbose > 0 )
	    Lidar::info( "%s POWEROFF", deviceName.c_str() );

	  if ( rpSerialDrv->isConnected() )
	    rpSerialDrv->disconnect();
	  delete rpSerialDrv;
	  rpSerialDrv = NULL;
	
	  return false;
	}

	op_result = rpSerialDrv->getDeviceInfo(info.devinfo);

	if (IS_OK(op_result))
	{ baudrateOrPort = brate;
	  connectSuccess = true;
	}
      }
    
      if ( !connectSuccess )
      { 
	if ( readPerm == 0 && rpSerialDrv->isConnected() )
	  rpSerialDrv->disconnect();
	delete rpSerialDrv;
        rpSerialDrv = NULL;
        if ( !tryOpen && g_Verbose > 0 )
	  Lidar::error( "LidarDevice(%s)::open(%s) failed with baudrate %d", RPLidarTypeName, deviceName.c_str(), brate);
      }
      else if ( g_Verbose > 0 )
	Lidar::info( "LidarDevice(%s)::open(%s) succeeded with baudrate %d", RPLidarTypeName, deviceName.c_str(), brate);
    }
  }
  
  if ( !connectSuccess )
  {
    if ( !tryOpen )
      Lidar::error( "LidarDevice(%s)::open(%s): can not bind to serial port.", RPLidarTypeName, deviceName.c_str() );
    closeDevice();
    errorMsg = "open failed";
    return false;
  }
  
  info.detectedDriverType = RPLIDAR;
  guessDeviceTypeRplidar( info );

  if ( !isSimulationMode && g_Verbose > 0 )
  {
    dumpInfo( info );
    std::vector<RplidarScanMode> outModes;

    if ( IS_OK(rpSerialDrv->getAllSupportedScanModes( outModes )) )
    {
      for ( int i = 0; i < outModes.size(); ++i )
      {
	printf("\n");
	printScanMode( outModes[i] );
      }
      printf("\n");
    }
  }
  
  DriverType driverTypeBak = driverType;
  driverType = RPLIDAR;
  
  if ( !isSimulationMode && !checkHealth() )
  {
    driverType = driverTypeBak;
    close();
    errorMsg = "health check failed";
    if ( g_Verbose > 0 )
      Lidar::error( "%s", errorMsg.c_str() );
    return false;
  }
  
//  if ( powerOff )
//    rpSerialDrv->reset();

//  printf( "PWM = %d\n", DEFAULT_MOTOR_PWM );

  rplidar.scanModeId = -1;

  if ( !rplidar.scanMode.empty() )
  {
    if ( isdigit(rplidar.scanMode[0]) )
      rplidar.scanModeId = atoi( rplidar.scanMode.c_str() );
    else
    {
      std::vector<RplidarScanMode> outModes;

      if ( IS_OK(rpSerialDrv->getAllSupportedScanModes( outModes )) )
      { for ( int i = 0; i < outModes.size(); ++i )
        { if ( rplidar.scanMode == outModes[i].scan_mode )
          { rplidar.scanModeId = outModes[i].id;
	    if ( g_Verbose > 0 )
	      Lidar::info( "using scan mode %d %s", outModes[i].id, outModes[i].scan_mode );
	    break;
	  }
	}
      }
    }
  }
  else if ( info.devinfo.model == 24 && ((int)info.devinfo.hardware_version) == 5 ) // set faster sefault mode for old A1M8
  {
    rplidar.scanModeId = 2;
  }

  if ( isSimulationMode )
    motorState = true;
  else
  {
    if ( !rpSerialDrv->checkMotorCtrlSupport( motorCtrlSupport ) )
      motorCtrlSupport = false;
  
    if ( g_Verbose > 0 )
      Lidar::info( "Motor Ctrl Support: %d", motorCtrlSupport );
      
    rpSerialDrv->startMotor();
  
    motorStartTime = getmsec();

    usleep( 250*1000 );
    if ( rplidar.scanModeId >= 0 )
      rpSerialDrv->startScanExpress( 0, rplidar.scanModeId, 0, &rplidar.outUsedScanMode );
    else
      rpSerialDrv->startScan( 0, 1, 0, &rplidar.outUsedScanMode );
  
    motorState = true;
  
    if ( g_Verbose > 0 )
    { Lidar::info( "used scan mode: " );
      printScanMode( rplidar.outUsedScanMode );
    }
  }
  
  if ( deviceType.empty() )
    deviceType = info.detectedDeviceType.c_str();

/*
  if ( isSimulationMode )
  {
    if ( deviceType == "a1m8" )
      setSpec( info.detectedDeviceType.c_str(), 12.0, 1000, 7.5 );
    else if ( deviceType == "a3m1" )
      setSpec( info.detectedDeviceType.c_str(), 13.0, 1250, 17 );
    else 
      setSpec( info.detectedDeviceType.c_str(), 10.0, 1000, 7.5 );
  }
*/

  setSpec( RPLIDAR, info.detectedDeviceType.c_str() );

  errorMsg = "";
  
  return true;
}

bool 
LidarDevice::openLocalDevice()
{
  if ( rpSerialDrv != NULL || ydSerialDrv != NULL || ldSerialDrv != NULL || msSerialDrv != NULL || lsSerialDrv != NULL || g_Shutdown )
    return true;

  if ( rpSerialDrvStopped != NULL )
  { rpSerialDrvStopped->disconnect();
    RPlidarDriver::DisposeDriver( rpSerialDrvStopped );
    rpSerialDrvStopped = NULL;
  }
  
  bool result = false;
  std::error_code ec;
  std::string path( std::filesystem::canonical( deviceName, ec ) );

  setUARTPower( true );

  if ( driverType == RPLIDAR )
    result = openDeviceRplidar();
  else if ( driverType == YDLIDAR )
    result = openDeviceYDLidar();
  else if ( driverType == MSLIDAR )
    result = openDeviceMSLidar( true );
  else if ( driverType == LDLIDAR )
    result = openDeviceLDLidar( true );
  else if ( driverType == LSLIDAR )
    result = openDeviceLSLidar( false );
  else if ( openDeviceYDLidar( true ) )
    result = true;
  else if ( openDeviceMSLidar( true ) )
    result = true;
  else if ( openDeviceLDLidar( true ) )
    result = true;
  else if ( openDeviceLSLidar( true ) )
    result = true;
  else if ( openDeviceRplidar( true ) )
    result = true;
 
  if ( !result )
  {
    errorMsg = "open failed";

    if ( g_StatusIndicatorSupported )
    { std::string cmd( hardwareDir );
      cmd += "setStatusIndicator.sh failure";
      system( cmd.c_str() );
    }
    setUARTPower( false );
  }
  
  return result;
}

void
LidarDevice::setMotorPWM( int pwm )
{
  motorPWM 	  = pwm;
  currentMotorPWM = pwm;
}
  
void
LidarDevice::setMotorSpeed( float speed )
{
  motorSpeed 	    = speed;
  currentMotorSpeed = speed;
}
  
void
LidarDevice::setMotorState( bool state )
{
  if ( motorState == state )
    return;
  
  if ( !isSimulationMode && isLocalDevice() )
  {
    lock();

    if ( state )
    {
      if ( driverType == RPLIDAR )
	rpSerialDrv->setMotorPWM( currentMotorPWM );
      else if ( driverType == MSLIDAR )
	msSerialDrv->startMotor();
      
      motorStartTime = getmsec();

      if ( g_Verbose > 0 )
	Lidar::info( "LidarDevice(%s): %s start", driverTypeString(), deviceName.c_str() );
    }
    else
    {
      if ( driverType == RPLIDAR )
	rpSerialDrv->setMotorPWM( 0 );
      else if ( driverType == MSLIDAR )
	msSerialDrv->stopMotor();

      if ( g_Verbose > 0 )
	Lidar::info( "LidarDevice(%s): %s stop", driverTypeString(), deviceName.c_str() );
    }

    unlock();
  }
  
  motorState = state;
}

bool
LidarDevice::isSpinning()
{
  if ( isVirtualDevice() )
    return false;

  bool isUART = (getConnectionType( deviceName.c_str() ) == UART);
  if ( isUART && isPoweringSupported() )
    return isOpen();

  if ( driverType == RPLIDAR )
    return rpSerialDrv != NULL;
  
  if ( driverType == YDLIDAR )
    return ydSerialDrv != NULL;

  if ( driverType == MSLIDAR )
    return msSerialDrv != NULL;

  if ( driverType == LDLIDAR )
    return true;

  if ( driverType == LSLIDAR )
    return true;

  return true;
}


void 
LidarDevice::closeDeviceRplidar()
{
  if ( rpSerialDrv == NULL )
    return;

  if ( g_Verbose > 0 )
    Lidar::info( "LidarDevice(%s): closing device %s", RPLidarTypeName, deviceName.c_str() );

  lock();

  if ( !isSimulationMode )
  { rpSerialDrv->stop();
    usleep( 20*1000 );
    rpSerialDrv->stopMotor();
  }

  if ( startsWith( info.detectedDeviceType, "a1m" ) ) // close a1m later, because the motor runs again when serial is disconnected
    rpSerialDrvStopped = rpSerialDrv;
  else
  { rpSerialDrv->disconnect();
    RPlidarDriver::DisposeDriver( rpSerialDrv );
  }

  rpSerialDrv = NULL;
  
  unlock();
}

void 
LidarDevice::closeLocalDevice()
{
  if ( driverType == RPLIDAR )
    closeDeviceRplidar();
  else if ( driverType == YDLIDAR )
    closeDeviceYDLidar();
  else if ( driverType == LDLIDAR )
    closeDeviceLDLidar();
  else if ( driverType == MSLIDAR )
    closeDeviceMSLidar();
  else if ( driverType == LSLIDAR )
    closeDeviceLSLidar();
  
  if ( g_StatusIndicatorSupported )
  { std::string cmd( hardwareDir );
    cmd += "setStatusIndicator.sh lidarOff";
    system( cmd.c_str() );
  }

  setUARTPower( false );
}

bool
LidarDevice::openVirtualDevice( LidarVirtualDriver *&virtDrv, const char *deviceName, bool isInDevice )
{
  if ( virtDrv != NULL )
  { virtDrv->isOpen = true;
    return true;
  }
  
  LidarUrl url( deviceName );
  
  if ( !url.isOk() )
    return false;

  virtDrv = new LidarVirtualDriver( isInDevice );
  
  bool success = virtDrv->connect( url.hostname, url.port );

  if ( !success )
  { delete virtDrv;
    virtDrv = NULL;
    return false;
  }
  
  if ( isInDevice )
    virtDrv->setMotorState( motorState=true );

  virtDrv->isOpen = true;

  return true;
}


void
LidarDevice::closeVirtualDevice( LidarVirtualDriver *&virtDrv, std::string &url )
{
  if ( virtDrv == NULL )
    return;
  
  if ( g_Verbose > 0 )
    Lidar::info( "LidarDevice: closing virtual device %s", url.c_str() );

  virtDrv->isOpen = false;
}


std::string
LidarDevice::getDefaultSerialDevice( int id )
{
  if ( g_IsSimulationMode )
  { std::string dev( std::to_string( id ) );
    return dev;
  }

#ifdef _WIN32
  return std::string( "\\\\.\\com57" );
#elif __APPLE__
  return std::string( "/dev/tty.SLAB_USBtoUART" );
#else // LINUX

  for ( int i = 0; i < maxDevices; ++i )
  {
    std::string dev( "/dev/lidar" );
    if ( id >= 0 )
      dev += std::to_string( id );
    else
      dev += std::to_string( i );

    if ( fileExists( dev ) )
      return dev;

    if ( id >= 0 )
      break;
  }
  
  if ( id >= 0 )
  { std::string devUSB( "/dev/ttyUSB" );
    devUSB += std::to_string( id );
    if ( fileExists( devUSB ) )
      return devUSB;
      
    std::string devACM( "/dev/ttyACM" );
    devACM += std::to_string( id );
    if ( fileExists( devACM ) )
      return devACM;
      
    std::string devS  ( "/dev/ttyS" );
    if ( id == 0 && g_Model == "RockPiS" )
      id = g_RockPiSDefaultSerialId;
    devS   += std::to_string( id );
    return devS;
  }

  for ( int i = 0; i < maxDevices; ++i )
  { std::string devUSB( "/dev/ttyUSB" );
    devUSB += std::to_string( i );
    if ( fileExists( devUSB ) )
      return devUSB;
  }
  
  for ( int i = 0; i < maxDevices; ++i )
  { std::string devACM( "/dev/ttyACM" );
    devACM += std::to_string( i );
    if ( fileExists( devACM ) )
      return devACM;
  }
  
  for ( int i = 0; i < maxDevices; ++i )
  { std::string devS( "/dev/ttyS" );
    devS += std::to_string( i );
    if ( fileExists( devS ) )
    { if ( i == 0 && g_Model == "RockPiS" )
      { devS = "/dev/ttyS";
	i = g_RockPiSDefaultSerialId;
	devS   += std::to_string( i );
      }	
      return devS;
    }
  }
  
  return std::string( "/dev/ttyUSB0" );
#endif
}

std::string
LidarDevice::resolveDeviceName( const char *deviceName )
{
  std::string devName( deviceName );
  
  if ( devName.empty() )
    devName = getDefaultSerialDevice();
  else if ( std::isdigit( devName[0] ) )
  { int id = std::atoi( devName.c_str() );
    devName = getDefaultSerialDevice( id );
  }
#if __LINUX__
  else if ( !fileExists( devName ) )
  {
    std::string deviceName( "/dev/" );
    deviceName += devName;

    if ( fileExists( deviceName ) )
      devName = deviceName;
  }
#endif
  
  return devName;
}

bool
LidarDevice::getInfoLDLidar( LidarDevice::Info &inf, const char *deviceName, bool dumpInfo )
{
  std::string devName( deviceName );
  bool success = true;

  lock();
  if ( !isSimulationMode && ldSerialDrv == NULL && inVirtUrl.empty() && inFileName.empty() )
  { success = false;
    this->deviceName = deviceName;
    if ( openDeviceLDLidar(true) )
    { unlock();
      closeDeviceLDLidar();
      lock();
      info.detectedDriverType = LDLIDAR;
      success = true;

      std::string serialNumber( getUSBSerialNumber( this->deviceName.c_str() ) );
      if ( !serialNumber.empty() )
	strncpy( (char *)info.devinfo.serialnum, serialNumber.c_str(), 16 );

      if ( dumpInfo )
	this->dumpInfo( info );
    }
  }
  
  if ( success )
    inf = info;

  unlock();

  return success;
}

bool
LidarDevice::getInfoMSLidar( LidarDevice::Info &inf, const char *deviceName, bool dumpInfo )
{
  std::string devName( deviceName );
  bool success = true;

  lock();
  if ( !isSimulationMode && msSerialDrv == NULL && inVirtUrl.empty() && inFileName.empty() )
  { success = false;
    this->deviceName = deviceName;
    if ( openDeviceMSLidar(true) )
    { unlock();
      closeDeviceMSLidar();
      lock();
      info.detectedDriverType = MSLIDAR;
      success = true;

      if ( dumpInfo )
	this->dumpInfo( info );
    }
  }
  
  if ( success )
    inf = info;

  unlock();

  return success;
}

bool
LidarDevice::getInfoLSLidar( LidarDevice::Info &inf, const char *deviceName, bool dumpInfo )
{
  std::string devName( deviceName );
  bool success = true;

  lock();
  if ( !isSimulationMode && lsSerialDrv == NULL && inVirtUrl.empty() && inFileName.empty() )
  { success = false;
    this->deviceName = deviceName;
    if ( openDeviceLSLidar(true) )
    { unlock();
      closeDeviceLSLidar();
      lock();
      info.detectedDriverType = LSLIDAR;
      success = true;
 
      std::string serialNumber( getUSBSerialNumber( this->deviceName.c_str() ) );
      if ( !serialNumber.empty() )
	strncpy( (char *)info.devinfo.serialnum, serialNumber.c_str(), 16 );

      if ( dumpInfo )
	this->dumpInfo( info );
    }
  }
  
  if ( success )
    inf = info;

  unlock();

  return success;
}


bool
LidarDevice::getInfoYDLidar( LidarDevice::Info &inf, const char *deviceName, bool dumpInfo )
{
  std::string devName( deviceName );

  lock();
  bool success = true;

  if ( !isSimulationMode && ydSerialDrv == NULL && inVirtUrl.empty() && inFileName.empty() )
  {
    success = false;
    YDLidarDriver *ydSerialDrv = new YDLidarDriver();
	
    if ( g_Verbose > 0 )
      Lidar::info( "LidarDevice(%s): opening %s device %s", YDLidarTypeName, isSimulationMode?"simulation":"serial", devName.c_str() );

    ydSerialDrv->param 	        = ydlidar;
    if ( baudrateOrPort != 0 )
      ydSerialDrv->param.baudrate = baudrateOrPort;
    
    int model, firmware_version, hardware_version;
    if ( ydSerialDrv->pingDeviceInfo( deviceName, model, firmware_version, hardware_version, info.devinfo.serialnum ) )
    { info.devinfo.model = model;
      info.devinfo.firmware_version = firmware_version;
      info.devinfo.hardware_version = hardware_version;
      
      info.detectedDriverType = YDLIDAR;

      YDLidarDeviceSpec *spec = YDLidarDriver::getSpec( model );
      if ( spec != NULL )
      { info.detectedDeviceType = spec->model;
	tolower( inf.detectedDeviceType );
	setDeviceType( info.detectedDeviceType.c_str() );

	delete ydSerialDrv;
	ydSerialDrv = new YDLidarDriver();

	ydSerialDrv->param = ydlidar;
	ydSerialDrv->param.baudrate = baudrateOrPort;
	bool success = ydSerialDrv->connect( deviceName );
	if ( success )
	{ YDLidarGetInfo( ydSerialDrv, info.devinfo );
	  inf = info;
	  if ( dumpInfo )
	    this->dumpInfo( info );
	}
      }
      
      success = true;
    }
    else
      errorMsg = "ping failed";
    
    delete ydSerialDrv;
    ydSerialDrv = NULL;
  }

  if ( success )
    inf = info;

  unlock();

  return success;
}

bool
LidarDevice::getInfoRplidar( LidarDevice::Info &inf, const char *deviceName, bool dumpInfo )
{
  std::string devName( deviceName );

  lock();
  bool connectSuccess = true;

  if ( !isSimulationMode && rpSerialDrv == NULL && inVirtUrl.empty() && inFileName.empty() )
  {
    u_result     op_result;

    connectSuccess = false;

    int size = sizeof(baudrateArray)/sizeof(baudrateArray[0]);

    if ( g_Verbose > 0 )
      Lidar::info( "LidarDevice(%s): opening %s device %s", RPLidarTypeName, isSimulationMode?"simulation":"serial", devName.c_str() );

    for ( int i = size-1 + (baudrateOrPort>0); !connectSuccess && i >= 0; --i )
    {
	// create the driver instance
#if SL_LIDAR_SDK_VERSION_MAJOR >= 2
      rp::standalone::rplidar::RPlidarDriver *rpSerialDrv = RPlidarDriver::CreateDriver(CHANNEL_TYPE_SERIALPORT);
#else
      rp::standalone::rplidar::RPlidarDriver *rpSerialDrv = RPlidarDriver::CreateDriver(DRIVER_TYPE_SERIALPORT);
#endif
      if (!rpSerialDrv)
      { Lidar::error( "LidarDevice(%s)::open(%s) insufficent memory, exit", RPLidarTypeName, devName.c_str() );
	unlock();
	exit(-2);
      }

      _u32 brate = (i >= size ? baudrateOrPort : baudrateArray[i]);

      if ( IS_OK(rpSerialDrv->connect(devName.c_str(), brate)) )
      {
	op_result = rpSerialDrv->getDeviceInfo(info.devinfo,50);
	connectSuccess = IS_OK(op_result);

	if ( connectSuccess )
	{ 
	  guessDeviceTypeRplidar( info );
	  info.detectedDriverType = RPLIDAR;
	  
	  if ( dumpInfo )
	  {
	    this->dumpInfo( info );

	    std::vector<RplidarScanMode> outModes;

	    if ( IS_OK(rpSerialDrv->getAllSupportedScanModes( outModes )) )
            {
	      for ( int i = 0; i < outModes.size(); ++i )
              {
		printf("\n");
		printScanMode( outModes[i] );
	      }
	      printf("\n");
	    }
	  }
	}
      }
	
      if ( rpSerialDrv->isConnected() )
	rpSerialDrv->disconnect();
      delete rpSerialDrv;
    }
  }

  if ( connectSuccess )
    inf = info;

  unlock();

  return connectSuccess;
}

bool
LidarDevice::getInfo( LidarDevice::Info &inf, const char *deviceName, bool dumpInfo )
{
  if ( deviceName == NULL )
    deviceName = this->deviceName.c_str();

  std::string devName( resolveDeviceName( deviceName ) );

  if ( inDrv == NULL && inFile == NULL && !isSimulationMode && !fileExists( devName.c_str() ) )
    return false;

  if ( driverType == RPLIDAR )
    return getInfoRplidar( inf, devName.c_str(), dumpInfo );
  else if ( driverType == YDLIDAR )
    return getInfoYDLidar( inf, devName.c_str(), dumpInfo );
  else if ( driverType == MSLIDAR )
    return getInfoMSLidar( inf, devName.c_str(), dumpInfo );
  else if ( driverType == LDLIDAR )
    return getInfoLDLidar( inf, devName.c_str(), dumpInfo );
  else if ( driverType == LSLIDAR )
    return getInfoLSLidar( inf, devName.c_str(), dumpInfo );
  else 
  {
    if ( getInfoRplidar( inf, devName.c_str(), dumpInfo ) )
      return true;
    if ( getInfoYDLidar( inf, devName.c_str(), dumpInfo ) )
      return true;
    if ( getInfoMSLidar( inf, devName.c_str(), dumpInfo ) )
      return true;
    if ( getInfoLDLidar( inf, devName.c_str(), dumpInfo ) )
      return true;

    return getInfoLSLidar( inf, devName.c_str(), dumpInfo );
  }
  
  return false;
}

std::string
LidarDevice::getSerialNumber( LidarDevice::Info &info )
{
  char serial[33];
  
  if ( info.detectedDriverType == YDLIDAR )
  { for ( int pos = 0; pos < 16 ; ++pos )
      sprintf( &serial[pos], "%c", info.devinfo.serialnum[pos] );
    serial[16] = '\0';
  }
  else
  {
    for ( int pos = 0; pos < 16 ; ++pos )
      sprintf( &serial[2*pos], "%02X", info.devinfo.serialnum[pos] );
    serial[32] = '\0';
  }
  
  return std::string( serial );
}


std::string
LidarDevice::getSerialNumber( const char *deviceName )
{
  LidarDevice::Info info;

  if ( !getInfo( info, deviceName ) )
    return std::string();
  
  return getSerialNumber( info );
}


void
LidarDevice::dumpInfo( const char *deviceName )
{
  LidarDevice::Info info;

  getInfo( info, deviceName, true );
}

  
bool 
LidarDevice::openDevice()
{
  if ( isOpen() || g_Shutdown )
    return true;

  int success = false;

  dataReceived = false;
  receivedTime = getmsec();

      // read serial port from the command line...
  if ( deviceName.empty() && inVirtUrl.empty() && inFileName.empty() )
    deviceName = getDefaultSerialDevice();

  if ( inFileName.empty() && !inVirtUrl.empty() )
  { if ( g_Verbose > 0 )
      Lidar::info( "LidarDevice: opening virtual input device %s", inVirtUrl.c_str() );
    success = openVirtualDevice( inDrv, inVirtUrl.c_str(), true );
    openFailed = !success;
    if ( success )
      errorMsg = "";
    else if ( errorMsg.empty() )
      errorMsg = "failed";

    LidarUrl url( inVirtUrl.c_str() );
    info.detectedDeviceType = std::string("virtual:") + std::to_string(url.port);
  }

  if ( !outVirtUrl.empty() )
  { if ( g_Verbose > 0 )
      Lidar::info( "LidarDevice: opening virtual output device %s", outVirtUrl.c_str() );
    success = (openVirtualDevice( outDrv, outVirtUrl.c_str(), false ) && success);
  }

  if ( inFileName.empty() && !outFileName.empty() )
  { std::string fileName( getFileDriverFileName( outFileName.c_str() ) );
    if ( g_Verbose > 0 )
      Lidar::info( "LidarDevice: opening output file %s", fileName.c_str() );

    std::string path( filePath( fileName ) );
    if ( !path.empty() ) 
    {
      if ( !fileExists( path.c_str() ) )
	std::filesystem::create_directories( path.c_str() );
      
      std::filesystem::path conf( path );
      conf /= std::filesystem::path("conf");

      if ( !fileExists( conf.c_str() ) )
	std::filesystem::create_directories( conf.c_str() );

      writeEnv( conf.c_str() );
      writeMatrix( conf.c_str() );
    }
    
    outFile = new LidarOutFile( fileName.c_str() );
  }

  if ( !inFileName.empty() )
  { std::string fileName( getFileDriverFileName( inFileName.c_str() ) );
    if ( g_Verbose > 0 )
      Lidar::info( "LidarDevice: opening input file %s", fileName.c_str() );
    inFile = new LidarInFile( fileName.c_str(), g_FileDriverSyncTime );
    success = inFile->is_open();
    openFailed = !success;
    if ( success )
      errorMsg = "";
    else if ( errorMsg.empty() )
      errorMsg = "failed";
    
    if ( openFailed )
      Lidar::error( "LidarDevice: opening input file %s", fileName.c_str() );

    std::vector<std::string> path( split(fileName, '/') );
    info.detectedDeviceType = std::string("file:") + path[path.size()-1];
    motorState = true;
  }
  else if ( !deviceName.empty() )
  {
    if ( std::isdigit( deviceName[0] ) )
    { int id = std::atoi( deviceName.c_str() );
      deviceName = getDefaultSerialDevice( id );
    }

    success    = openLocalDevice();
    openFailed = !success;
  }
  
  lock();
  ready = success;
  unlock();

  return success;
}


void 
LidarDevice::closeDevice()
{
  if ( !isOpen() )
    return;

  ready = false;

  if ( dataValid )
    dataValid = false;

  if ( !deviceName.empty() )
    closeLocalDevice();
  
  if ( inDrv != NULL && inDrv->isOpen )
    inDrv->setMotorState( false );
  
  closeVirtualDevice( inDrv,  inVirtUrl  );
  closeVirtualDevice( outDrv, outVirtUrl );

  if ( inFile != NULL )
  { delete inFile;
    inFile = NULL;

    for ( int i = 0; i < g_DeviceList.size(); ++i )
      if ( this == g_DeviceList[i] && g_FileDriverSyncIndex == i )
	g_FileDriverSyncIndex = -1;
  }

  if ( outFile != NULL )
  { delete outFile;
    outFile = NULL;
  }
}

bool 
LidarDevice::open()
{
  if ( thread == NULL )
    thread = new std::thread( runScanThread, this );  

  lock();
  if ( !shouldOpen )
  { shouldOpen = true;
    openTime   = getmsec();
  }
  
  unlock();
  
  return true;
}

void 
LidarDevice::close()
{
  lock();
  shouldOpen  = false;
  errorMsg    = "";
  unlock();
}

bool 
LidarDevice::checkHealth()
{
  if ( isSimulationMode )
    return true;
  
  if ( driverType == RPLIDAR )
  {
    u_result     op_result;
    rplidar_response_device_health_t healthinfo;

    op_result = rpSerialDrv->getHealth(healthinfo);
    if (IS_OK(op_result))
    {
      if ( healthinfo.status != RPLIDAR_STATUS_OK )
	Lidar::error("RPLidar health status : %s (errorcode: %d)", healthinfo.status == RPLIDAR_STATUS_WARNING ? "Warning." : "Error.", healthinfo.error_code);
      else if ( g_Verbose > 0 )
	Lidar::info("RPLidar health status : Ok." );

      if (healthinfo.status == RPLIDAR_STATUS_ERROR) {
	rpSerialDrv->reset();
	return false;
      } 

      return true;
    } 
    else
    {
      Lidar::error( "can not retrieve the lidar health code: %x", op_result);
      return false;
    }
  }

  return false;
}

LidarDevice &
LidarDevice::operator *=( const Matrix3H &m )
{
  if ( m.isIdentity() )
    return *this;
  
  for ( int i = 0; i < numSampleBuffers; ++i )
    sampleBuffer(i) *= m;
 
  if ( envValid )
  { envSamples    *= m;
    envRawSamples *= m;
  }
  
  if ( isAccumulating )
    accumSamples *= m;

  objects *= m;
  matrix        = m * matrix;
  matrixInverse = matrixInverse * m.inverse();

  return *this;
}


void
LidarDevice::setMatrix( const Matrix3H &m )
{
  if ( m == matrix )
    return;
  
  (*this) *= matrix.inverse();
  (*this) *= m;

  matrix        = m;
  matrixInverse = m.inverse();
}


void
LidarDevice::setDeviceMatrix( const Matrix3H &devMatrix )
{
  if ( deviceMatrix == devMatrix )
    return;
  
  deviceMatrix = devMatrix;
  setMatrix( viewMatrix * deviceMatrix );
}


void
LidarDevice::setViewMatrix( const Matrix3H &vMatrix )
{
  if ( viewMatrix == vMatrix )
    return;
  
  viewMatrix = vMatrix;
  setMatrix( viewMatrix * deviceMatrix );
}


void
LidarDevice::setCharacteristic( double char1, double char2, const char *devType )
{
  if ( devType != NULL && !deviceType.empty() && deviceType != devType )
    return;

  this->char1 = char1;
  this->char2 = char2;
}

bool
LidarDevice::isEnvSample( LidarSample &sample ) const
{
  int angIndex = angIndexByAngle( sample.angle );

  if ( envValid && useEnv && envSamples[angIndex].quality > info.spec.minQuality )
  { if ( sample.distance > envSamples[angIndex].distance - envThreshold )
      return true;

    if ( g_UseSimulationRange && sample.distance > info.spec.maxRange )
      return true;
  }

  return false;
}

bool 
LidarDevice::isTempNoiseSample( int angIndex ) const
{
#if DEBUG
  return false;
#endif

  for ( int i = 1; i < numSampleBuffers; ++i )
  { 
    LidarSampleBuffer &sampleBuf( sampleBuffer(i) );
    
    LidarSample &sample( sampleBuf[sampleBuf[angIndex].sourceIndex] );
    if ( sample.sourceQuality <= info.spec.minQuality )
      return true;
  }

  return false;
}


bool
LidarDevice::scanValid( int i ) const
{
  LidarSample &sample( sampleBuffer(0)[i] );
  if ( !sample.isValid() )
    return false;
    
  if ( useTemporalDenoise && isTempNoiseSample(i) )
    return false;
  
  if ( isEnvSample( sample ) )
    return false;

  return true;
}

bool
LidarDevice::isValid( int i ) const
{
  LidarSample &sample( sampleBuffer()[i] );
  if ( !sample.isValid() )
    return false;
  
  if ( isEnvSample( sample ) )
    return false;

  if ( !isAccumulating && useTemporalDenoise && isTempNoiseSample(i) )
    return false;
  
  return true;
}

bool
LidarDevice::coordVisible( const Vector3D &coor ) const
{
  Vector3D coord( matrixInverse * coor );
 
  float distance = coord.length();
  
//  if ( distance > 18 ) // try it first with fixed width
//    return false;

  if ( !isLocalDevice() )
  {
    if ( distance > 18 ) 
      return false;
  }
  else if ( distance > rplidar.outUsedScanMode.max_distance )
    return false;

  float angle  = ((Vector2D&)coord).angle();
  int angIndex = angIndexByAngle( angle );

  const float fuzzyDist = 3.0; // allow 3m displacement in registration
  
  if ( envValid && useEnv && envSamples[angIndex].quality > info.spec.minQuality )
  { if ( distance > envSamples[angIndex].distance + fuzzyDist )
      return false;
  }

  LidarSampleBuffer &samples( sampleBuffer() );
  LidarSample &sample( samples[angIndex] );

  if ( sample.isValid() && distance > sample.distance + fuzzyDist )
    return false;
    
  return true;
}


bool
LidarDevice::coordVisible( float x, float y ) const
{
  Vector3D coord( x, y, 0.0 );

  return coordVisible( coord );
}


bool
LidarDevice::getCoord( int i, float &x, float &y ) const
{
  if ( !isValid( i ) )
    return false;

  LidarSample &sample( sampleBuffer()[i] );

  x = sample.coord.x;
  y = sample.coord.y;
  
  return true;
}

int
LidarDevice::getObjectId( int i ) const
{
  LidarSample &sample( sampleBuffer()[i] );

  return sample.oid;
}

void
LidarDevice::cleanupAccum( int registerSec )
{
  int threshold = registerSec * 3;

  if ( info.average_fps.fps > 0 )
  {
//    printf( "device(%d) fps: %d samples: %d\n", deviceId, info.average_fps.fps, info.average_samples.average() );

    float thres = (registerSec * info.average_fps.fps * info.average_samples.average() / 1150 ) / 5.7;

    const float maxThres = 9;

    if ( thres < maxThres )
      thres = sqrt(thres/maxThres) * maxThres;

    threshold = round( thres );
  }

  if ( maxAccumCount > 0 )
  {
//    printf( "device(%d) fps: %d samples: %d maxAccumCount: %d\n", deviceId, info.average_fps.fps, info.average_samples.average(), maxAccumCount );

    float thres = (maxAccumCount - 3) * 0.3;

//    printf( "thres = %g\n", thres );

    const float minThres = 3;

    if ( thres < minThres )
      thres = minThres;

    threshold = round( thres );
  }
  
  for ( int i = (int)numSamples-1; i >= 0; --i )
  {
//    if ( accumSamples[i].accumCount > 1 )
//      printf( "device(%d)[%d] count: %d thres: %d  %s\n", deviceId, i, accumSamples[i].accumCount, threshold, accumSamples[i].accumCount >= threshold ? " *" : "" );
    
    if ( accumSamples[i].accumCount < threshold )
      accumSamples[i].quality = 0;
  }
}

void
LidarDevice::setAccum( bool set )
{
  if ( set == isAccumulating )
    return;

  isAccumulating = set;

  if ( set )
  {
    maxAccumCount = 0;
    
    for ( int i = (int)numSamples-1; i >= 0; --i )
    { accumSamples[i].accumCount = 1;
      accumSamples[i].quality    = 0;
    }
  }
}

void
LidarDevice::setUseOutEnv( bool outEnv )
{
  useOutEnv   = outEnv;
  envChanged();
}

void
LidarDevice::sendOutEnv()
{
  if ( inDrv == NULL || !inDrv->isOpen )
    return;

  LidarRawSampleBuffer  nodes;
  LidarSampleBuffer    &samples( envSamples );

  if ( !doEnvAdaption )
  {
    nodes.resize( samples.size() );

    for ( int i = ((int) samples.size())-1; i >= 0; --i )
    {
      LidarRawSample &node  ( nodes[i] );
      LidarSample    &sample( samples[i] );
   
      node.quality     = sample.quality;
      node.dist_mm_q2  = sample.distance * 1000 * 4;
      node.angle_z_q14 = (sample.angle / M_PI) * 180.0 * (1 << 14) / 90.0;
    }
  }
  
  envOutDirty = false;

  inDrv->sendUseOutEnv( useOutEnv && useEnv );
  inDrv->sendEnvData( nodes );
}


void
LidarDevice::envChanged()
{
  envOutDirty = true;
}


void
LidarDevice::erodeEnv( LidarSampleBuffer &srcSamples, LidarSampleBuffer &dstSamples, int steps )
{
  for ( int angIndex = numSamples-1; angIndex >= 0; --angIndex )
  {
    LidarSample &dstSample ( dstSamples[angIndex] );
    LidarSample &srcSample ( srcSamples[angIndex] );
    
    dstSample = srcSample;
    /**/
    for ( int i = 1; i < steps; ++i )
    {
      LidarSample &prevSample( srcSamples[addAngIndex(angIndex,-i)] );
      LidarSample &nextSample( srcSamples[addAngIndex(angIndex,+i)] );

      if ( prevSample.quality > info.spec.minQuality )
      { if ( dstSample.quality <= info.spec.minQuality || (fabs(prevSample.distance-dstSample.distance) < envFilterMinDistance && prevSample.distance < dstSample.distance) )
	{ if ( dstSample.quality <= info.spec.minQuality )
	    dstSample.angle = angleByAngIndex( angIndex );
	  dstSample.quality  = prevSample.quality;
	  dstSample.distance = prevSample.distance;
	}
      }

      if ( nextSample.quality > info.spec.minQuality )
      { if ( dstSample.quality <= info.spec.minQuality || (fabs(nextSample.distance-dstSample.distance) < envFilterMinDistance && nextSample.distance < dstSample.distance) )
        { if ( dstSample.quality <= info.spec.minQuality )
	    dstSample.angle = angleByAngIndex( angIndex );
	  dstSample.quality  = nextSample.quality;
	  dstSample.distance = nextSample.distance;
	}
      }
    }
    /**/
  }
}

void
LidarDevice::smoothEnv( LidarSampleBuffer &samples, int steps )
{
  float distances[numSamples];

  double stepsm1 = (steps <= 1 ? 1 : steps-1);

  const float minDistance = envFilterMinDistance;

  for ( int angIndex = numSamples-1; angIndex >= 0; --angIndex )
  {
    LidarSample &sample( samples[angIndex] );

    distances[angIndex] = sample.distance;

    if ( sample.quality > info.spec.minQuality )
    {
      const float sampleDistance = sample.distance;
      float distance       	 = sampleDistance;
      float distanceSum    	 = sampleDistance;

      int count = 1;
      for ( int i = steps-1; i > 0; --i )
      { 
	const LidarSample &prevSample( samples[addAngIndex(angIndex,-i)] );
	const LidarSample &nextSample( samples[addAngIndex(angIndex,+i)] );
	
	double alpha = 1.0 - (0.3*i) / stepsm1;

	if ( prevSample.quality > info.spec.minQuality && (sampleDistance-prevSample.distance < minDistance && prevSample.distance < distance) )
	{ distance     = mix( alpha, sampleDistance, prevSample.distance );
	  distanceSum += distance;
	  count += 1;
	}
	
	if ( nextSample.quality > info.spec.minQuality && (sampleDistance-nextSample.distance < minDistance && nextSample.distance < distance) )
	{ distance     = mix( alpha, sampleDistance, nextSample.distance );
	  distanceSum += distance;
	  count += 1;
	}
      }

      if ( distance < 0.01 )
	distanceSum = 100 * count;
      

      distances[angIndex] = distanceSum / count;
    }
    else
      distances[angIndex] = 1024;
  }
  
  for ( int angIndex = numSamples-1; angIndex >= 0; --angIndex )
  {
    LidarSample &sample( samples[angIndex] );

    sample.distance = distances[angIndex];

    Vector3D coord( sample.distance * sin( sample.angle ), sample.distance * cos( sample.angle ), 0.0 );
    sample.coord = matrix * coord;
  }
}

void
LidarDevice::processEnv()
{
  int steps = round( envFilterSize/360.0 * numSamples);

//	  printf( "LidarDevice:filterSteps = %d\n", steps );

  if ( info.detectedDeviceType == "ms200" || info.detectedDeviceType == "st27" ) // hack eroding does not work for this for a magic reason
  {
    lock();
  
    for ( int angIndex = numSamples-1; angIndex >= 0; --angIndex )
    { LidarSample &sample( envSamples[angIndex] );
      sample = envRawSamples[angIndex];
      Vector3D coord( sample.distance * sin( sample.angle ), sample.distance * cos( sample.angle ), 0.0 );
      sample.coord = matrix * coord;
    }
    unlock();
  }
  else
  {
    erodeEnv ( envRawSamples, envErodedSamples, steps );
    smoothEnv( envErodedSamples, steps );

    LidarSampleBuffer &erodedSamples( envErodedSamples );
    LidarSampleBuffer &eSamples( envSamples );

    lock();
  
    for ( int angIndex = numSamples-1; angIndex >= 0; --angIndex )
      eSamples[angIndex] = erodedSamples[angIndex];

  /*
  for ( int angIndex = numSamples-1; angIndex >= 0; --angIndex )
    if ( envSamples[angIndex].quality > info.spec.minQuality )
      printf( "env q: %d (%s) d=%g(%g)\n", envSamples[angIndex].quality, info.detectedDeviceType.c_str(), envSamples[angIndex].distance, info.spec.maxRange );
  */
    unlock();
  }
  
  envValid = true;  
}

void
LidarDevice::adaptEnv()
{
  const float thres  = envThreshold;
  const uint64_t environmentDepthTime = envAdaptSec * 1000;

  LidarSampleBuffer &samples( sampleBuffer() );
  LidarSampleBuffer &eRawSamples ( envRawSamples );
  LidarSampleBuffer &eDSamples ( envDSamples );
  
  for ( int i = 0; i < (int)numSamples ; ++i )
  {
    LidarSample &sample( samples[i] );

    if ( sample.touched )
    { 
      int angIndex = angIndexByAngle( sample.angle );
    
      LidarSample &eSample( eRawSamples[angIndex] );
      LidarSample &dSample( eDSamples[angIndex] );

      bool  valid = scanValid(i);
      float z     = sample.distance;
      float dz    = dSample.distance;

      if ( valid && dSample.quality > info.spec.minQuality )
      {
	dSample 	  = sample;
	dSample.quality   = valid;
	dSample.distance -= thres;
//	eSample 	  = sample;
	envTimeStamps[i] = timeStamp;
      }
      else if ( valid )
      {
	if ( z < dz )
        { dSample.distance = z;
	  envTimeStamps[i] = timeStamp;
	}
        else if ( z > dz+thres )
        {
	  dSample 	    = sample;
	  dSample.distance -= thres;

	  float ez = eSample.distance;
	  if ( ez+thres < z )
	    eSample.distance = z-thres;
	  
	  envTimeStamps[i] = timeStamp;
	}
	else if ( timeStamp - envTimeStamps[i] > environmentDepthTime )
	  eSample = sample;
      }
    }
  }
}

float
LidarDevice::calcEnvConfidence( const LidarSample &sample ) const
{
  float confidence = 1.0;
  
  if ( info.spec.envMinQuality > 0 )
  {
    float qualityConfidence  = (sample.quality-info.spec.envMinQuality) / (float)(127-info.spec.envMinQuality);
    float distanceConfidence = sample.distance / (info.spec.maxRange * 1.1);
    qualityConfidence  = pow( qualityConfidence,  1.8 );
    distanceConfidence = pow( distanceConfidence, 0.25 );

    confidence = qualityConfidence + distanceConfidence;
    //    if ( sample.quality > info.spec.envMinQuality )
    //      printf( "q: %d (%s) d=%g(%g) qc=%g dc=%g c=%g\n", sample.quality, info.detectedDeviceType.c_str(), sample.distance, info.spec.maxRange, qualityConfidence, distanceConfidence, confidence );
  }
  
  return confidence;
}

void
LidarDevice::updateEnv()
{
  const LidarSampleBuffer &samples( sampleBuffer() );

  for ( int i = 0; i < (int)numSamples ; ++i )
  {
    const LidarSample    &sample( samples[i] );
    int angIndex = angIndexByAngle( sample.angle );

    LidarSample &envSample( envSamples[angIndex] );
    LidarSample &rawSample( envRawSamples[angIndex] );

    if ( sample.quality > info.spec.envMinQuality ) 
    { 
      float confidence = calcEnvConfidence( sample );

      if ( confidence >= 1.0 && (envSample.quality <= 0 || sample.distance < envSample.distance) )
      { envSample.quality  = rawSample.quality  = sample.quality;
	envSample.distance = rawSample.distance = sample.distance;
	envSample.coord    = rawSample.coord    = sample.coord;
	envTimeStamps[i]   = timeStamp;
      }
    }
  }

  envValid = true;
}

void
LidarDevice::scanEnv()
{
  lock();
  
  LidarSampleBuffer &samples( sampleBuffer() );

  for ( int i = 0; i < (int)numSamples ; ++i )
  { LidarSample &envSample( envSamples[i] );
    LidarSample &rawSample( envRawSamples[i] );
    envSample.quality  = rawSample.quality  = -1;
    envSample.angle    = rawSample.angle    = angleByAngIndex( i );
    envSample.distance = rawSample.distance = info.spec.maxRange*10;
    envTimeStamps[i]   = timeStamp;
  }


  useOutEnvBak = useOutEnv;
  setUseOutEnv( false );
  //  updateEnv();

  /*
  for ( int i = 0; i < (int)numSamples; ++i )
  {
    LidarSample &sample   ( samples[i] );

    int angIndex = angIndexByAngle(sample.angle);
    LidarSample &envSample( envSamples[angIndex] );
    LidarSample &rawSample( envRawSamples[angIndex] );

    if ( envSample.quality <= 0 || sample.distance < envSample.distance )
    {
      envSample.distance = rawSample.distance = sample.distance;
      envSample.quality  = rawSample.quality  = sample.quality;
      envSample.coord    = rawSample.coord    = sample.coord;
    }
  }
  */
  envValid = true;

  isEnvScanning = true;
  processStartTime = getmsec();

  unlock();
}

bool
LidarDevice::readEnv( const char *path )
{
  CheckPointMode mode = (g_ReadCheckPoint.empty()? NoCheckPoint : ReadCheckPoint);

  bool reportError     = !envFileName.empty();
  std::string fileName = getConfigFileName( getEnvFileName().c_str(), NULL, path, mode );
  
  lock();
  
  bool result = envRawSamples.read( fileName.c_str() );

  for ( int i = 0; i < (int)numSamples ; ++i )
    envTimeStamps[i] = timeStamp;

  envSamples *= matrix;

  unlock();

  processEnv();
  envChanged();

  if ( !result )
  { if ( reportError )
      Lidar::error( "failed to read Environment file '%s'", fileName.c_str() );
  }
  else if ( g_Verbose > 0 )
    Lidar::info( "reading Environment file '%s'", fileName.c_str() );

  return result;
}


bool
LidarDevice::writeEnv( const char *path, uint64_t timestamp )
{
  CheckPointMode mode = (timestamp != 0? WriteCreateCheckPoint : NoCheckPoint);
  std::string fileName = getConfigFileName( getEnvFileName().c_str(), NULL, path, mode, timestamp );
  
  lock();
  
  envRawSamples *= matrixInverse;
  bool result = envRawSamples.write( fileName.c_str() );
  envRawSamples *= matrix;
  
  unlock();

  return result;
}

void
LidarDevice::resetEnv()
{
  lock();
  
  for ( int i = 0; i < (int)numSamples ; ++i )
  { envSamples[i].quality  = -1;
    envSamples[i].angle    = angleByAngIndex( i );
    envSamples[i].distance = info.spec.maxRange * 10;
  }

  unlock();

  envChanged();
}

bool
LidarDevice::readMatrix( const char *path )
{
  CheckPointMode mode = (g_ReadCheckPoint.empty()? NoCheckPoint : ReadCheckPoint);

  bool reportError     = !matrixFileName.empty();
  std::string fileName = getConfigFileName( getMatrixFileName().c_str(), NULL, path, mode );

  std::ifstream stream( fileName );
  if ( !stream.is_open() )
  { if ( reportError )
      Lidar::error( "failed to read Transformation file '%s'", fileName.c_str() );
    return false;
  }
  
  if ( g_Verbose > 0 )
    Lidar::info( "reading Transformation file '%s'", fileName.c_str() );

  lock();
  
  Matrix3H m;
  stream >> m.x.x >> m.x.y >> m.y.x >> m.y.y >> m.w.x >> m.w.y;
  setDeviceMatrix( m );
    
  m.id();
  stream >> m.x.x >> m.x.y >> m.y.x >> m.y.y >> m.w.x >> m.w.y;
  setViewMatrix( m );

  unlock();

  return true;
}


bool
LidarDevice::writeMatrix( const char *path, uint64_t timestamp )
{
  CheckPointMode mode = (timestamp != 0? WriteCreateCheckPoint : NoCheckPoint);
  std::string fileName = getConfigFileName( getMatrixFileName().c_str(), NULL, path, mode, timestamp );
  
  std::ofstream stream( fileName );
  if ( !stream.is_open() )
    return false;
  
  lock();
  
  stream << deviceMatrix.x.x << " " << deviceMatrix.x.y << " " << deviceMatrix.y.x << " " << deviceMatrix.y.y << " " << deviceMatrix.w.x << " " << deviceMatrix.w.y << "\n";
  stream << viewMatrix.x.x << " " << viewMatrix.x.y << " " << viewMatrix.y.x << " " << viewMatrix.y.y << " " << viewMatrix.w.x << " " << viewMatrix.w.y << "\n";

  unlock();

  return true;
}

bool
LidarDevice::addDetectedObject( LidarObjects &objects, int lowerIndex, int higherIndex, bool isSplit )
{
  LidarSampleBuffer &samples( sampleBuffer() );

  float closest = 1000.0;
  
  int lIndex = -1;
  int hIndex = -1;

  for ( int count = higherIndex; count >= lowerIndex; --count )
  {
    int angIndex = LidarDevice::angIndex( count );
    LidarSample &sample( samples[angIndex] );

    if ( sample.isValid() && sample.oid != 0 )
    {
      if ( hIndex == -1 )
	hIndex = angIndex;
      else
	lIndex = angIndex;

      float c = sample.distance;
      if ( c < closest )
	closest = c;
    }
  }

  if ( lIndex == -1 || hIndex == -1 )
    return false;
  
  float extent = samples[lIndex].coord.distance( samples[hIndex].coord );
	
  addDetectedObject( objects, lIndex, hIndex, extent, closest, isSplit );

  return true;
}

void
LidarDevice::addDetectedObject( LidarObjects &objects, int lowerIndex, int higherIndex, float extent, float closest, bool isSplit )
{
  LidarSampleBuffer &samples( sampleBuffer() );

  int hIndex = (higherIndex < lowerIndex ? higherIndex+numSamples : higherIndex);

  int indexRange = hIndex - lowerIndex;

  if ( objectMaxExtent > 0 && extent > objectMaxExtent )
  {
    int num = ceil( extent / objectMaxExtent );
    if ( num == 1 )
      num = 2;

    LidarObjects obj;
    bool success = true;

    if ( num == 2 )
    {
      int firstIndex = lowerIndex;
      int lastIndex  = higherIndex;
    
      const float lower  = 0.25;
      const float higher = 0.75;
    
      int lIndex 	   = round( lowerIndex + lower  * (higherIndex-lowerIndex) );
      int hIndex 	   = round( lowerIndex + higher * (higherIndex-lowerIndex) );
   
      float maxCurvature = 0;
      int maxIndex = -1;

      for ( int index = lIndex; index <= hIndex; ++index )
      {
	float c1, c2;
    
	if ( calcCurvature( c1, samples, lowerIndex, index, 0.0 ) && calcCurvature( c2, samples, index, higherIndex, 0.0 ) )
        {
	  float curvature = fabsf(c1) + fabsf(c2);
	
	  if ( curvature > maxCurvature )
          { maxCurvature = curvature;
	    maxIndex     = index;
	  }
	}
      }

      if ( maxIndex >= 0 )
      {
	if ( !addDetectedObject( obj, lowerIndex, maxIndex, true ) )
	  success = false;
	if ( !addDetectedObject( obj, maxIndex, higherIndex, true ) )
	  success = false;
      }
    }
    else
    {
      int lastIndex = lowerIndex;
      for ( int i = 0; i < num; ++i )
      { int nextIndex = lowerIndex + (i+1) * indexRange/(float)num;
	if ( !addDetectedObject( obj, lastIndex, nextIndex, true ) )
	  success = false;
	lastIndex = nextIndex + 1;
      }
    }

    if ( success && obj.size() > 0 )
    { for ( int i = 0; i < obj.size(); ++i )
	objects.push_back( obj[i] );
      return;
    }
  }

  Vector3D &lowerCoord( samples[lowerIndex].coord );
  Vector3D &higherCoord( samples[higherIndex].coord );

  if ( higherIndex < lowerIndex )
    higherIndex += numSamples;

  objects.push_back( LidarObject( lowerIndex, higherIndex, extent ) );

  LidarObject &object( objects.back() );
  object.isSplit  = isSplit;

  object.lowerCoord  = lowerCoord;
  object.higherCoord = higherCoord;
  object.update();
  
  object.normal = matrixInverse * object.center;

  if ( closest < 1000 )
  { closest = object.normal.length() - closest;
    if ( closest > 0 && closest < 1.0 )
      object.closest = closest;
  }

//  printf( "c: %g %g %g\n", closest, object.normal.length(), object.closest );

  object.normal.normalize();
}

void
LidarDevice::detectObjects()
{
  LidarObjects detectedObjects;
  LidarSampleBuffer &samples( sampleBuffer() );

  int oidcount = 1;
  int lastoid  = oidcount;

  LidarSample *lastSample = NULL;

  for ( int angIndex = numSamples-1; angIndex >= 0; --angIndex )
  {
    LidarSample &sample( samples[angIndex] );

    if ( !isValid( angIndex ) )
      sample.oid = 0;
    else
    {
      if ( lastSample != NULL )
      { float distance = sample.coord.distance( lastSample->coord );
	if ( distance > objectMaxDistance )
	  lastoid = (oidcount+=1);
      }
      sample.oid = lastoid;

      lastSample = &sample;
    }
  }

      // make object ids continuous at 0 degree
  for ( int angIndex = numSamples-1; angIndex >= 0; --angIndex )
  {
    LidarSample &sample( samples[angIndex] );

    if ( sample.oid == 0 )
      break;
    
    LidarSample &prevSample( samples[addAngIndex(angIndex,1)] );
	
    if ( prevSample.oid == 0 )
      break;
    
    sample.oid = prevSample.oid;
  }

      // remove too small objects
  
  int indexOffset = -1;
  
      // search first sample
  for ( int angIndex = 0; angIndex < numSamples/2; ++angIndex )
  {
    LidarSample &sample( samples[angIndex] );
    if ( sample.oid != 0 )
    { if ( indexOffset == -1 || samples[indexOffset].oid == sample.oid ) 
	indexOffset = angIndex;
      else
	break;
    }
  }

//  printf( "-----\n" );
  

  int lowerAngIndex  = -1;
  int higherAngIndex = -1;

  float closest = 1000;

  for ( int count = numSamples-1; count > 0; --count )
  {
    int angIndex = LidarDevice::addAngIndex( count, indexOffset );
      
    LidarSample &sample( samples[angIndex] );

    if ( sample.oid != 0 )
    {
      if ( higherAngIndex == -1 )
      {	higherAngIndex = angIndex;
	lowerAngIndex  = angIndex;
	closest = sample.distance;
      }
      else if ( samples[lowerAngIndex].oid == sample.oid )
      {
	lowerAngIndex = angIndex;
	float c = sample.distance;
	if ( c < closest )
	  closest = c;
      }
      else
      {
	float extent = samples[lowerAngIndex].coord.distance( samples[higherAngIndex].coord );
	  
//	printf( "extent1: %d - %d  %g\n", lowerAngIndex, higherAngIndex, extent );

	if ( extent >= objectMinExtent )
	  addDetectedObject( detectedObjects, lowerAngIndex, higherAngIndex, extent, closest );
	
	higherAngIndex = angIndex;
	lowerAngIndex  = angIndex;
	closest = 1000;
      }
    }
  }

  if ( higherAngIndex != -1 && lowerAngIndex != -1 && lowerAngIndex != higherAngIndex )
  {
    float extent = samples[lowerAngIndex].coord.distance( samples[higherAngIndex].coord );
//    printf( "extent2: %d - %d  %g\n", lowerAngIndex, higherAngIndex, extent );
    if ( extent >= objectMinExtent )
      addDetectedObject( detectedObjects, lowerAngIndex, higherAngIndex, extent, closest );
  }

  for ( int angIndex = numSamples-1; angIndex > 0; --angIndex )
    samples[angIndex].oid = 0;

  detectedObjects.calcCurvature( samples );

  if ( !doObjectTracking || detectedObjects.size() == 0 || objects.size() == 0 )
  {
    objects = detectedObjects;

    for ( int oi = 0; oi < objects.size(); ++oi )
      objects[oi].oid = (doObjectTracking ? (oidCount=(oidCount%(oidMax))+1) : oi+1);
  }
  else
  {
    std::vector<TrackInfo> trackInfo;
    
	/* calculate distances from substage to merged stage */

    for ( int di = 0; di < detectedObjects.size(); ++di )
    { for ( int oi = 0; oi < objects.size(); ++oi )
      { double distance =  detectedObjects[di].center.distance( objects[oi].center );
	if ( distance <= objectTrackDistance )
	  trackInfo.push_back( TrackInfo({distance, di, oi}) ); /* { distance, detectedIndex, objectIndex } */
      }
    }

    std::sort( trackInfo.begin(), trackInfo.end(), compareTrackInfo );

    bool detectedUsed[detectedObjects.size()] = { false };
    bool objectsUsed[objects.size()] 	      = { false };
    std::fill_n( detectedUsed, detectedObjects.size(), 	false );
    std::fill_n( objectsUsed,  objects.size(), 	 	false );

    for ( int i = 0; i < trackInfo.size(); ++i )
    {
      TrackInfo &ti( trackInfo[i] );

	  /* only if they are not already assigned */
      if ( !detectedUsed[ti.detectedIndex] && !objectsUsed[ti.objectIndex] )
      {
	detectedObjects[ti.detectedIndex].oid = objects[ti.objectIndex].oid;

	detectedUsed[ti.detectedIndex] = true;
	objectsUsed[ti.objectIndex]    = true;
      }
    }
	/* add iods for objects with no correspondence */
    for ( int di = ((int)detectedObjects.size())-1; di >= 0; --di )
      if ( !detectedUsed[di] )
	detectedObjects[di].oid = (oidCount=(oidCount%1024)+1);

    objects = detectedObjects;
  }

  for ( int oi = 0; oi < objects.size(); ++oi )
  {
    int lowerAngIndex  = addAngIndex( objects[oi].lowerIndex, 0 );
    int higherAngIndex = addAngIndex( objects[oi].higherIndex, 0 );
    
    int oid = objects[oi].oid;

    for ( int angIndex = lowerAngIndex; angIndex <= higherAngIndex; ++angIndex )
      samples[angIndex].oid = oid;

//    printf( "%d: %g\n", objects[oi].oid, objects[oi].lineScatter(samples) );
  }

  objects.sortByAngle();
}

inline double rnd()
{ return ((double) rand() / (RAND_MAX));
}

bool (*LidarDevice::obstacleSimulationRay)( LidarDevice &device, LidarRawSample &sample, float &angle, float &distance );
bool (*LidarDevice::obstacleSimulationCheckOverlap)( LidarDevice &device );

bool
LidarDevice::scanSimulation( LidarRawSampleBuffer &sampleBuffer, float distance, int numSamples, float scanFreq, bool coverage )
{
  int delayMsec = 1000000 / scanFreq;
  
  usleep( delayMsec );

  if ( isEnvScanning )
    numSamples = envSamples.size();
  
  sampleBuffer.resize( numSamples );
  
  //  const float angleVariance = (isEnvScanning ? 0 : 0.2);
  const float angleVariance = 0;
  const float distVariance  = 0.0175;

  for ( int i = numSamples-1; i >= 0; --i )
  {
    LidarRawSample &sample( sampleBuffer[i] );

    if ( coverage )
    {
      float random = rnd();

      float angle = (2*M_PI * (i + angleVariance * random)) / numSamples;;
      float sampleDistance = distance * (1-0.5*distVariance + distVariance * rnd());
      
      int angIndex = angIndexByAngle( angle );

      sample.quality = 0;
      
      if ( envValid && useEnv )
      { if ( envSamples[angIndex].quality > 0 )
        { 
       	  sampleDistance = envSamples[angIndex].distance - envThreshold;
	  if ( sampleDistance < 0 )
          { sample.quality = 0;
       	    sampleDistance = 0;
	  }
        }
      }
      
      if ( obstacleSimulationRay != NULL )
	if ( obstacleSimulationRay( *this, sample, angle, sampleDistance ) )
	  sample.quality = 100;
            
     //      printf( "ang: %d %d\n", i, angIndex );

      sample.angle_z_q14 = angle / M_PI * 180.0 / 90.0 * (1 << 14);
      sample.dist_mm_q2  = sampleDistance * 1000 * 4;
    }
  }
  
  return true;
}

bool
LidarDevice::scanSimulation( LidarRawSampleBuffer &sampleBuffer, bool coverage )
{
  return scanSimulation( sampleBuffer, info.spec.maxRange, info.spec.numSamples, info.spec.scanFreq, coverage );
}

bool
LidarDevice::scanLDLidar( LidarRawSampleBuffer &sampleBuffer )
{
  ScanData laserScan;
  bool result = false;

  if ( ldSerialDrv == NULL )
    return false;

  if ( !ldSerialDrv->grabScanData( laserScan ) )
    return false;
 
  if ( g_Verbose >= 2 )
  {
    std::cout << "[ldlidar] speed(Hz)         " << ldSerialDrv->getSpeed() << std::endl;
    std::cout << "[ldlidar] laser_scan.size() " << laserScan.size() << std::endl;
  }

  sampleBuffer.resize( laserScan.size() );
  
  for ( int i = ((int)laserScan.size())-1; i >= 0; --i )
  {
    LidarRawSample &sample( sampleBuffer[i] );
    ScanPoint      &scanPoint( laserScan[i] );
   
    if ( g_Verbose >= 3 )
      Lidar::info("sample(%d): theta: %03.2f Dist: %08.2f Q: %d ", i, scanPoint.angle, scanPoint.distance, (int)scanPoint.quality );

    sample.dist_mm_q2  = scanPoint.distance * 1000 * 4;
    sample.angle_z_q14 = scanPoint.angle / 90.0 * (1 << 14);
    sample.quality     = (scanPoint.quality < 127 ? scanPoint.quality : 127);
  }
  
  return true;
}

bool
LidarDevice::scanMSLidar( LidarRawSampleBuffer &sampleBuffer )
{
  ScanData laserScan;
  bool result = false;

  if ( msSerialDrv == NULL )
    return false;

  if ( !msSerialDrv->grabScanData( laserScan ) )
    return false;
 
  if ( g_Verbose >= 2 )
  {
    std::cout << "[mslidar] speed(Hz)： " << msSerialDrv->getRotationSpeed() << std::endl;
    std::cout << "[mslidar] laser_scan.size() " << laserScan.size() << std::endl;
  }

  sampleBuffer.resize( laserScan.size() );
  
  for ( int i = ((int)laserScan.size())-1; i >= 0; --i )
  {
    LidarRawSample &sample( sampleBuffer[i] );
    ScanPoint      &scanPoint( laserScan[i] );
   
    if ( g_Verbose >= 3 )
      Lidar::info("sample(%d): theta: %03.2f Dist: %08.2f Q: %d ", i, scanPoint.angle, scanPoint.distance, (int)scanPoint.quality );

    sample.dist_mm_q2  = scanPoint.distance * 1000 * 4;
    sample.angle_z_q14 = scanPoint.angle / 90.0 * (1 << 14);
    sample.quality     = (scanPoint.quality < 127 ? scanPoint.quality : 127);
  }
  
  return true;
}

bool
LidarDevice::scanLSLidar( LidarRawSampleBuffer &sampleBuffer )
{
  ScanData laserScan;
  bool result = false;

  if ( lsSerialDrv == NULL || !lsSerialDrv->grabScanData( laserScan ) )
    return false;
  
  if ( g_Verbose >= 2 )
  {
//    std::cout << "[lslidar] speed(Hz)： " << lsSerialDrv->lidar->GetSpeed() << std::endl;
//    std::cout << "[lslidar] pack errcount: " << lsSerialDrv->lidar->GetErrorTimes() << std::endl;
    std::cout << "[lslidar] laser_scan.size() " << laserScan.size() << std::endl;
  }

  sampleBuffer.resize( laserScan.size() );
  
  for ( int i = ((int)laserScan.size())-1; i >= 0; --i )
  {
    LidarRawSample &sample( sampleBuffer[i] );
    ScanPoint      &scanPoint( laserScan[i] );
   
    if ( g_Verbose >= 3 )
      Lidar::info("sample(%d): theta: %03.2f Dist: %08.2f Q: %d ", i, scanPoint.angle, scanPoint.distance, (int)scanPoint.quality );

    sample.dist_mm_q2  = scanPoint.distance * 1000 * 4;
    sample.angle_z_q14 = scanPoint.angle / 90.0 * (1 << 14);
    sample.quality     = (scanPoint.quality < 127 ? scanPoint.quality : 127);
  }
  
  return true;
}

bool
LidarDevice::scanYDLidar( LidarRawSampleBuffer &sampleBuffer )
{
  ScanData laserScan;
  bool result = false;

  if ( ydSerialDrv == NULL || !ydSerialDrv->grabScanData( laserScan ) )
    return false;

/*  
  if ( g_Verbose >= 2 )
  {
    std::cout << "[ydlidar] speed(Hz)： " << ydSerialDrv->lidar->GetSpeed() << std::endl;
    std::cout << "[ydlidar] pack errcount: " << ydSerialDrv->lidar->GetErrorTimes() << std::endl;
    std::cout << "[ydlidar] laser_scan.size() " << laserScan.size() << std::endl;
  }
*/

  sampleBuffer.resize( laserScan.size() );
  
  for ( int i = ((int)laserScan.size())-1; i >= 0; --i )
  {
    LidarRawSample &sample( sampleBuffer[i] );
    ScanPoint      &scanPoint( laserScan[i] );
   
    if ( g_Verbose >= 3 )
      Lidar::info("sample(%d): theta: %03.2f Dist: %08.2f Q: %d ", i, scanPoint.angle, scanPoint.distance, (int)scanPoint.quality );

    sample.dist_mm_q2  = scanPoint.distance * 1000 * 4;
    sample.angle_z_q14 = scanPoint.angle / 90.0 * (1 << 14);
    sample.quality     = (scanPoint.quality < 127 ? scanPoint.quality : 127);
  }

  return true;
}

bool
LidarDevice::scanRplidar( LidarRawSampleBuffer &sampleBuffer )
{
  rplidar_response_measurement_node_hq_t nodes[numScanSamples];
  size_t   count = numScanSamples;

  bool result = false;

  if ( rpSerialDrv != NULL )
  {
    u_result op_result = rpSerialDrv->grabScanDataHq( nodes, count );

    if (IS_OK(op_result))
    { rpSerialDrv->ascendScanData( nodes, count );
      result = true;
    }
  }

  if ( result )
  {
    sampleBuffer.resize( count );

    for (int i = (int)count-1; i >= 0; --i)
    {
      if ( g_Verbose >= 3 )
	Lidar::info("sample(%d) %s theta: %03.2f Dist: %08.2f Q: %d ", i,
		   (nodes[i].flag & RPLIDAR_RESP_MEASUREMENT_SYNCBIT) ?"S ":"  ", 
		   (nodes[i].angle_z_q14 * 90.f / (1 << 14)), 
		   nodes[i].dist_mm_q2/1000.0f/4.0f,
		   nodes[i].quality);

      LidarRawSample &sample( sampleBuffer[i] );
      rplidar_response_measurement_node_hq_t &node( nodes[i] );
      
      sample.angle_z_q14 = node.angle_z_q14;
      sample.dist_mm_q2  = node.dist_mm_q2;
      sample.quality     = (node.quality < 127 ? node.quality : 127);
    }
  }

  return result;
}


bool
LidarDevice::checkInVirtHostName()
{
  if ( inDrv == NULL )
    return false;
  
  if ( inVirtHostName == inDrv->getRemoteHostname() && inVirtPort == inDrv->getRemotePort() )
    return false;
    
  inVirtHostName        = inDrv->getRemoteHostname();
  inVirtPort            = inDrv->getRemotePort();
  inDrv->remoteHostname = inVirtHostName;
  inDrv->remotePort     = inVirtPort;

  if ( g_Verbose > 0 )
    Lidar::info( "Got new Host for Port %s -> %s:%d", info.detectedDeviceType.c_str(), inVirtHostName.c_str(), inVirtPort );

  envChanged();

  return true;
}

bool
LidarDevice::scan()
{
  if ( !isReady() )
    return false;
  LidarRawSampleBuffer nodes;
  bool result    = false;
  bool clearData = false;
  bool isEnvData = false;

  uint64_t samplesTimeStamp = 0;

  if ( inFile != NULL )
  { 
    usleep( 2000 );
    
    if ( !g_FileDriverPaused || scanOnce )
    { result = inFile->grabScanData( nodes );
      if ( result )
      { if ( g_FileDriverSyncIndex == -1 )
	  for ( int i = 0; i < g_DeviceList.size(); ++i )
	    if ( this == g_DeviceList[i] )
	      g_FileDriverSyncIndex = i;
	
	samplesTimeStamp = inFile->timeStamp();

	if ( g_FileDriverSyncIndex >= 0 && this == g_DeviceList[g_FileDriverSyncIndex] )
        { g_FileDriverCurrentTime  = inFile->currentTime();
	  g_FileDriverTimeStamp    = inFile->timeStamp();
	  g_FileDriverTimeStampRef = getmsec();
	  g_FileDriverPlayPos      = inFile->playPos();
	}
	scanOnce = false;
      }
      else
      { usleep( 100*1000 );
	if ( inFile->is_eof() )
	  errorMsg = "end of file";
      }
    }
  }
  else if ( inDrv != NULL )
  { result = inDrv->grabScanData( nodes, 1, true );
    
//    printf( "grabScanData: %d\n", result );
    
    if ( result )
      checkInVirtHostName();
  }
  else if ( isSimulationMode )
    result = scanSimulation( nodes );
  else if ( driverType == RPLIDAR )
    result = scanRplidar( nodes );
  else if ( driverType == YDLIDAR )
    result = scanYDLidar( nodes );
  else if ( driverType == LDLIDAR )
    result = scanLDLidar( nodes );
  else if ( driverType == MSLIDAR )
    result = scanMSLidar( nodes );
  else if ( driverType == LSLIDAR )
    result = scanLSLidar( nodes );

//  printf( "result: %d %ld\n", result, nodes.size() );
  uint64_t now = getmsec();

  if ( samplesTimeStamp == 0 )
    samplesTimeStamp = now;

  if ( !result )
  {
    if ( !g_FileDriverPaused )
    {
      uint64_t no_data_msec = now - receivedTime;

#define DEBUG_FRAMERATE 0

#if DEBUG_FRAMERATE
      if ( no_data_msec > 300 )
      {
	Lidar::error( "do data since %ld msec", no_data_msec );
      }
#endif

      if ( no_data_msec > 1000 )
      { clearData = true;
	if ( no_data_msec > 30000 && dataReceived ) // after 30sec no receiving dada mark as no data
	{
	  dataReceived = false;

	  if ( g_StatusIndicatorSupported && !deviceName.empty() && inDrv == NULL && inFile == NULL )
      { std::string cmd( hardwareDir );
	    cmd += "setStatusIndicator.sh failure";
	    system( cmd.c_str() );
	  }
	}
      }
    }
  }
  else
  {
    if ( outFile != NULL )
      outFile->put( nodes );
    
    if ( outDrv != NULL )
      isEnvData = outDrv->grabEnvData( nodes );
    
    if ( !dataReceived )
    {
      dataReceived = true;

      if ( g_StatusIndicatorSupported && !deviceName.empty() && inDrv == NULL && inFile == NULL )
      { std::string cmd( hardwareDir );
	cmd += "setStatusIndicator.sh lidarOn";
	system( cmd.c_str() );
      }
    }

    errorMsg = "";
  }
  
  if ( result || clearData )
  {
    lock();
    if ( result )
    { timeStamp    = now - startTime;
      receivedTime = now;
    }
    
    info.samplesPerScan = nodes.size();
    info.tick();
    
    if ( g_Verbose >= 2 )
      Lidar::info( "samples: %d \tfps: %d\t average fps: %d\t average samples: %d", info.samplesPerScan, info.fps.fps, info.average_fps.fps, info.average_samples.average() );
    
    sampleBufferIndex = (sampleBufferIndex+1) % (32768*numSampleBuffers);

    LidarSampleBuffer &samples( sampleBuffer(0) );
/*
    printf( "++++++++++\n" );
*/  
  
    for (int i = (int)numSamples-1; i >= 0; --i)
    { LidarSample &sample( samples[i] );
      sample.quality       = -1;
      sample.oid           = 0;
      sample.touched       = false;
    }

//    printf( "1 char; %g %g\n", char1, char2 );

    for (int i = (int)(nodes.size())-1; i >= 0; --i)
    {
      int quality     = nodes[i].quality;
      samples[i].sourceQuality = quality;

      float angle     = M_PI * (nodes[i].angle_z_q14 * 90.f / (1 << 14)) / 180.0;
      float distance  = nodes[i].dist_mm_q2/1000.0f/4.0f;
      int angIndex    = angIndexByAngle( angle );

      LidarSample &sample( samples[angIndex] );
      sample.sourceIndex  = i;
      sample.touched      = true;
  
      //      if ( quality > info.spec.minQuality )
      {
//	if ( sample.quality != 0 || distance < sample.distance )
	{
	  sample.quality    = quality;
	  sample.angle      = angle;
	  sample.distance   = nodes[i].dist_mm_q2/1000.0f/4.0f;
//        visited[angIndex] = true;
      
//        printf( "%d %d\n", i, angIndexByAngle(sample.angle) );

//    printf( "2 char; %g %g\n", char1, char2 );

	  sample.distance = sample.distance * (char1 + char2 * sample.distance);
	
	  Vector3D coord( sample.distance * sin( sample.angle ), sample.distance * cos( sample.angle ), 0.0 );
	  sample.coord = matrix * coord;
	}
      }
    }

    if ( outDrv != NULL && !isEnvData )
    {
      int ni = 0;
      nodes.resize( 0 );

      for ( int i = (int)numSamples-1; i >= 0; --i )
      { 
	if ( (!(envValid && useOutEnv) && samples[i].quality > info.spec.minQuality) || isValid( i ) )
	{ LidarSample &sample( samples[i] );
	  nodes.resize( ni+1 );
	  nodes[ni].quality     = sample.quality;
	  nodes[ni].dist_mm_q2  = sample.distance * 1000 * 4;
	  nodes[ni].angle_z_q14 = sample.angle / M_PI * 180.0 / 90.0 * (1 << 14);
	  ni += 1;
	}
      }
      outDrv->sendScanData( nodes );
    }
    
    if ( isAccumulating )
    {
      for (int i = numSamples-1; i >= 0; --i)
      {
	if ( scanValid(i) )
	{ 
	  LidarSample &sample     ( samples[i]      );
	  LidarSample &accumSample( accumSamples[i] );

//	  printf( "acc: %d %d\n", i, sample.quality );

	  double alpha = 1.0 / accumSample.accumCount;

	  accumSample.angle       = sample.angle;
	  accumSample.distance    = sample.distance;
	  accumSample.quality     = sample.quality;
	  accumSample.oid         = 0;
	  accumSample.accumCount += 1;

	  if ( accumSample.accumCount > maxAccumCount )
	    maxAccumCount = accumSample.accumCount;

	  Vector3D coord( accumSample.distance * sin( accumSample.angle ), accumSample.distance * cos( accumSample.angle ), 0.0 );
	  coord = matrix * coord;

//	  printf( "alpha: %g\n", alpha );
	  
	  accumSample.coord = coord * alpha + accumSample.coord * (1-alpha);
	}
      }
    }

    if ( isEnvData )
    {
      unlock();

      scanEnv();
      updateEnv();
      processEnv();
      
      setUseOutEnv( useOutEnvBak );

      isEnvScanning = false;

      envValid = (nodes.size() > 0);  
    }
    else
    {
      if ( !dataValid )
	dataValid = true;

      if ( doObjectDetection )
      {
	if ( isEnvScanning )
	  objects = LidarObjects();
	else
        {
	  detectObjects();
	  objects.setTimeStamp( samplesTimeStamp );
	}
      }

      unlock();
    }

    if ( dataValid && !isEnvScanning && doEnvAdaption && envAdaptSec > 0.0 )
    {
      adaptEnv();
  
      processEnv();
      envChanged();

      envValid = true;
    }

//    uint64_t end = getmsec();
//    printf( "scan: %ld\n", end - now );
  }

  return result;
}

LidarObjects
LidarDevice::visibleObjects( const LidarObjects &other ) const
{
  LidarObjects objects;

  for ( int i = ((int)other.size())-1; i >= 0; --i )
  {
    const LidarObject &object( other[i] );
    if ( coordVisible( object.center      ) && 
	 coordVisible( object.lowerCoord  ) && 
	 coordVisible( object.higherCoord ) )
      objects.push_back( object );
  }
  
  return objects;
}


float
LidarDevice::calcTransformTo( const LidarDevice &other, Matrix3H &meMatrix, Matrix3H &otMatrix, bool refine ) const
{
  Matrix3H meMat( matrix );
  Matrix3H otMat( other.matrix );

  std::string lastMessage = LidarDevices::message;

  LidarObjects meObjects( refine ? other.visibleObjects( objects ) : objects );
  LidarObjects otObjects( refine ? visibleObjects( other.objects ) : other.objects );

//  printf( "size: %d %d\n", meObjects.size(), otObjects.size() );

  meObjects *= meMat.inverse();
  otObjects *= otMat.inverse();

  LidarObjects::Marker meMarker( meObjects.getMarker( sampleBuffer() ) );
  LidarObjects::Marker otMarker( otObjects.getMarker( sampleBuffer() ) );

  char msg[1000];
  sprintf( msg, "id(%d) m=%d -> id(%d) m=%d\n", deviceId, (int)meMarker.size(), other.deviceId, (int)otMarker.size() );

  if ( g_Verbose > 0 )
    Lidar::info( "%s", msg );

  LidarDevices::message = lastMessage;
  LidarDevices::message += msg;

  float distance = meMarker.calcTransformTo( otMarker, meMatrix, otMatrix );

  sprintf( msg, "id(%d) m=%d -> id(%d) m=%d distance=%g\n", deviceId, (int)meMarker.size(), other.deviceId, (int)otMarker.size(), distance );

  if ( g_Verbose > 0 )
    Lidar::info( "%s", msg );

  LidarDevices::message = lastMessage;
  LidarDevices::message += msg;

  return distance;
}

bool
LidarDevice::parseArg( int &i, const char *argv[], int &argc ) 
{
  bool success = true;

  if ( strcmp(argv[i],"lidar.env.scanSec") == 0 )
  {
    envScanSec = atof( argv[++i] );
  }
  else if ( strcmp(argv[i],"lidar.env.adaptSec") == 0 )
  {
    envAdaptSec = atof( argv[++i] );
  }
  else if ( strcmp(argv[i],"lidar.env.threshold") == 0 )
  {
    envThreshold = atof( argv[++i] );
  }
  else if ( strcmp(argv[i],"lidar.env.adapt") == 0 )
  {
    doEnvAdaption = atoi( argv[++i] );
  }
  else if ( strcmp(argv[i],"lidar.env.filterMinDistance") == 0 )
  {
    envFilterMinDistance = atof( argv[++i] );
  }
  else if ( strcmp(argv[i],"lidar.env.filterSize") == 0 )
  {
    envFilterSize = atof( argv[++i] );
  }
  else if ( strcmp(argv[i],"lidar.object.maxDistance") == 0 )
  {
    objectMaxDistance = atof( argv[++i] );
  }
  else if ( strcmp(argv[i],"lidar.object.minExtent") == 0 )
  {
    objectMinExtent = atof( argv[++i] );
  }
  else if ( strcmp(argv[i],"lidar.object.maxExtent") == 0 )
  {
    objectMaxExtent = atof( argv[++i] );
  }
  else if ( strcmp(argv[i],"lidar.object.trackDistance") == 0 )
  {
    objectTrackDistance = atof( argv[++i] );
  }
  else
    success = false;
    
  return success;
}
  
void
LidarDevice::copyArgs( LidarDevice *argDevice )
{
  objectMaxDistance 	= argDevice->objectMaxDistance;
  objectMinExtent 	= argDevice->objectMinExtent;
  objectMaxExtent 	= argDevice->objectMaxExtent;
  objectTrackDistance 	= argDevice->objectTrackDistance;
  envThreshold 		= argDevice->envThreshold;
  envFilterMinDistance 	= argDevice->envFilterMinDistance;
  envScanSec 		= argDevice->envScanSec;
  envAdaptSec 		= argDevice->envAdaptSec;
  envFilterSize 	= argDevice->envFilterSize;
  doEnvAdaption         = argDevice->doEnvAdaption;
}


static void
printArgHelpImpl( const char *name, float value, const char *descr )
{
  printf( "  %s (default: %g)  \t%s\n", name, value, descr );
}

static void
printArgHelpImpl( const char *name, bool value, const char *descr )
{
  printf( "  %s (default: %d)  \t%s\n", name, value, descr );
}

static void
printArgHelpImpl( const char *name, int value, const char *descr )
{
  printf( "  %s (default: %d)  \t%s\n", name, value, descr );
}

void
LidarDevice::printArgHelp() const
{
  printArgHelpImpl( "lidar.object.maxDistance",		objectMaxDistance,	"max distance between samples to be united to a single object" );
  printArgHelpImpl( "lidar.object.minExtent",		objectMinExtent,	"min extent of a group of samples to be reported as a object" );
  printArgHelpImpl( "lidar.object.maxExtent",		objectMaxExtent,	"\textent of a group of samples to be split into several objects" );
//  printArgHelpImpl( "lidar.object.trackDistance", 	objectTrackDistance,	"\tmax distance between samples to be united to a singe object" );
  printArgHelpImpl( "lidar.env.threshold",		envThreshold,		"\tdistance from measured value in which a sample is still reported as environmental" );
  printArgHelpImpl( "lidar.env.filterMinDistance",	envFilterMinDistance,	"distance between samples used for eroding and smoothing the environment" );
  printArgHelpImpl( "lidar.env.scanSec",		envScanSec,		"\ttime in sec used to scan the environment" );
  printArgHelpImpl( "lidar.env.adapt",		doEnvAdaption,		"\tswitches Environment adaption on=1 or off=0" );
  printArgHelpImpl( "lidar.env.adaptSec",		envAdaptSec,		"\ttime in sec used to adapt the environment." );
  printArgHelpImpl( "lidar.env.filterSize",		envFilterSize,		"size of angular filter used for eroding and smoothing the environment" );
}
  

  
void
LidarDevice::ThreadFunction()
{
  while( !exitThread )
  { 
    bool open = isOpen();
    
    if ( open != shouldOpen && !openFailed )
    {
      if ( shouldOpen )
	openDevice();
      else
	closeDevice();

      open = isOpen();
    }

    if ( !open )
    {
      usleep( 100 * 1000 );
      
      if ( outDrv != NULL )
	outDrv->update( 100 );
      
      if ( inDrv != NULL )
	inDrv->update( 100 );
    }
    else if ( !powerOff )
    {
      const int waitTimeout = 10;
      
      if ( inDrv != NULL )
      {
	if ( envOutDirty )
	  sendOutEnv();

	inDrv->update( waitTimeout );

	std::string cmd;
	while ( !(cmd=inDrv->getNextCmd()).empty() )
        {
	  if ( cmd == "connect" )
	    envChanged();
	  else if ( cmd == "startPowerUp" )
	    isPoweringUp = true;
	  else if ( cmd == "finishPowerUp" )
	    isPoweringUp = false;
	  else if ( startsWith( cmd, "deviceType=" ) )
	  { 
	    LidarUrl url( inVirtUrl.c_str() );

	    std::vector<std::string> pairs( split(cmd,' ') );
	    for ( int i = 0; i < pairs.size(); ++i )
            {
	      std::vector<std::string> pair( split(pairs[i],'=') );

	      if ( pair.size() == 2 )
              { 
		if ( pair[0] == "deviceType" )
                { setSpec( pair[1].c_str() );
		  info.detectedDeviceType = "virtual:"+ std::to_string(url.port) + ":" + pair[1];
		}
		else if ( pair[0] == "sensorIN" )
		  sensorIN = pair[1];
		else if ( pair[0] == "sensorPowerSupported" )
		  inVirtSensorPower = (pair[1]=="true");
	      }
	    }
	  }
	  
	  checkInVirtHostName();
	}
      }

      if (outDrv != NULL && outDrv->isOpen )
      {
	outDrv->update( 0 );	

	std::string cmd;
	while ( !(cmd=outDrv->getNextCmd()).empty() )
        {
	  if ( cmd == "motorOn" )
	    setMotorState( true );
	  else if ( cmd == "motorOff" )
	    setMotorState( false );
	  else if ( cmd == "outEnvOn" )
	    useOutEnv = true;
	  else if ( cmd == "outEnvOff" )
	    useOutEnv = false;
	  else if ( !outDrv->deviceStatusSent && !info.detectedDeviceType.empty() )
	    outDrv->sendDeviceType( info.detectedDeviceType.c_str(), sensorIN.c_str(), devicePoweringSupported() );  
//	  printf( "got cmd: '%s'\n", cmd.c_str() );
	}
      }
   
      if ( motorState )
      {
	if ( (motorPWM > 0 && driverType == RPLIDAR) || (motorSpeed > 0 && driverType == MSLIDAR)  )
        {
	  uint64_t currentTime = getmsec();
	  uint64_t milliSec    = currentTime - motorStartTime;

	  if ( milliSec >= 3000 )
          {
	    if ( !isSimulationMode )
            {
	      if ( driverType == RPLIDAR )
	      {
		if ( motorCtrlSupport && !IS_OK(rpSerialDrv->setMotorPWM( motorPWM )) )
		  Lidar::error( "failed to set Motor PWM to %d", motorPWM );
		else if ( g_Verbose > 0 )
		  Lidar::info( "set Motor PWM to %d", motorPWM );
	      }
	      else if ( driverType == MSLIDAR )
	      {
		msSerialDrv->setRotationSpeed( motorSpeed );
		if ( g_Verbose > 0 )
		  Lidar::info( "set Motor Speed to %g", motorSpeed );
	      }
	    }
	    
	    motorPWM   = 0;
	    motorSpeed = 0;
	  }
	}

	bool result = scan();

        if ( isEnvScanning )
        { 
	  uint64_t currentTime = getmsec();
	  uint64_t milliSec    = currentTime - processStartTime;

	  if ( milliSec < envScanSec * 1000 )
	  { if ( result )
            { lock();
	      updateEnv();
	      unlock();
	    }
	  }
	  else
	  { 
	    processEnv();
	    envChanged();

	    setUseOutEnv( useOutEnvBak );

	    isEnvScanning = false;
	  }
	}
	else if ( !result )
	  usleep( 500 );
      }
    }
    else
    { 
      usleep( 10*1000 );
    }
  }
}


void
LidarDevice::setUseStatusIndicator( bool set )
{
  g_UseStatusIndicator = set;
}

/***************************************************************************
*** 
*** LidarDevices
***
****************************************************************************/

void
LidarDevices::setViewMatrix( Matrix3H &matrix, bool all )
{
  LidarDeviceList devices( runningOrAllDevices(all) );

  viewMatrix = matrix;
  
  for ( int d = 0; d < devices.size(); ++d )
    devices[d]->setViewMatrix( matrix );
}

void
LidarDevices::setAccum( bool set, bool all )
{
  LidarDeviceList devices( runningOrAllDevices(all) );

  for ( int i = 0; i < devices.size(); ++i )
  {
    LidarDevice  *device = devices[i];

    if ( device->isReady() )
    { device->lock();
      device->setAccum( set );
      device->unlock();
    }
  }
}

bool
LidarDevices::isSimulationMode()
{ return g_IsSimulationMode;
}

void
LidarDevices::setSimulationMode( bool set )
{ 
  LidarDeviceList devices( runningOrAllDevices(true) );

  for ( int i = 0; i < devices.size(); ++i )
  { LidarDevice  *device = devices[i];
    device->isSimulationMode = set;
  }
  
  g_IsSimulationMode = set;
}


void
LidarDevices::setUseSimulationRange( bool set )
{ g_UseSimulationRange = set;
}

void
LidarDevices::setReadCheckPoint( const char *checkPoint )
{ g_ReadCheckPoint = std::filesystem::path( checkPoint );
}

void
LidarDevices::setObjectTracking( bool set )
{ 
  LidarDeviceList devices( runningOrAllDevices(true) );

  for ( int i = 0; i < devices.size(); ++i )
  { LidarDevice  *device = devices[i];
    device->doObjectDetection = set;
//    device->doObjectTracking  = set;
  }
}


void
LidarDevices::scanEnv()
{
  LidarDeviceList devices( runningDevices() );

  for ( int d = 0; d < devices.size(); ++d )
  {
    LidarDevice *device = devices[d];
    
    if ( device->isReady() )
    { if ( envScanSec > 0 )
	device->envScanSec = envScanSec;
      device->doEnvAdaption = false;
      device->scanEnv();
    }
  }
}

void
LidarDevices::resetEnv( bool all )
{
  LidarDeviceList devices( runningOrAllDevices(all) );

  for ( int d = 0; d < devices.size(); ++d )
    devices[d]->resetEnv();
}

void
LidarDevices::loadEnv( bool all )
{
  LidarDeviceList devices( runningOrAllDevices(all) );

  for ( int d = 0; d < devices.size(); ++d )
    devices[d]->readEnv();
}

void
LidarDevices::saveEnv( bool all, uint64_t timestamp )
{
  LidarDeviceList devices( runningOrAllDevices(all) );

  for ( int d = 0; d < devices.size(); ++d )
    if ( devices[d]->isReady() )
      devices[d]->writeEnv( NULL, timestamp );
}

void
LidarDevices::useEnv( bool use )
{
  if ( _useEnv == use )
    return;
  
  _useEnv = use;
  for ( int d = 0; d < size(); ++d )
  {
    LidarDevice *device( (*this)[d] );
    
    device->lock();
    device->useEnv = use;
    device->envChanged();
    device->unlock();
  }
}

void
LidarDevices::initBasisChanges() 
{
  LidarDeviceList devices( runningDevices() );

  for ( int d = 0; d < devices.size(); ++d )
  {
    LidarBasisChanges &basisChanges( devices[d]->basisChanges );
      
    basisChanges.clear();
    
    for ( int i = ((int)devices.size())-1; i >= 0; --i )
      basisChanges.push_back(LidarBasisChange());

    basisChanges[d].valid = true;

    basisChanges.shrink_to_fit();
  }
}

bool
LidarDevices::basisChangesComplete() const
{
  LidarDeviceList devices( runningDevices() );

  for ( int i = 0; i < ((int)devices.size())-1; ++i )
  {
    LidarBasisChanges &bcdi( devices[i]->basisChanges );

    for ( int j = i+1; j < devices.size(); ++j )
    {
      if ( g_Verbose > 0 )
	printf( "basischange %d -> %d: %s\n", i, j, bcdi[j].valid ? "valid" : "invalid" );

      if ( !bcdi[j].valid )
	return false;
    }
  }
    
  return true;
}
  
bool
LidarDevices::calculateBasisChanges()
{
  LidarDeviceList devices( runningDevices() );

  for ( int step = 10; step > 0; --step )
  {
//    printf( "step %d\n", step );

    for ( int i = 0; i < ((int)devices.size())-1; ++i )
    {
      LidarBasisChanges &bcdi( devices[i]->basisChanges );

      for ( int j = i+1; j < devices.size(); ++j )
      {
	for ( int k = 0; k < devices.size(); ++k )
        {
	  LidarBasisChanges &bcdk( devices[k]->basisChanges );
	    
	  if ( bcdi[k].valid && bcdk[j].valid )
	  {
//	    printf( "computing %d -> %d via %d\n", i, j, k );

	    LidarBasisChanges &bcdj( devices[j]->basisChanges );

	    float error = bcdi[k].error + bcdk[j].error;
	    
	    if ( !bcdi[j].valid ||  error< bcdi[j].error )
	    {
	      char msg[1000];
	      if ( !bcdi[j].valid )
		sprintf( msg, "choosing    id(%d) -> id(%d) via id(%d) (error=%g)\n", i, j, k, error );
	      else
		sprintf( msg, "overwriting id(%d) -> id(%d) via id(%d) (error=%g)\n", i, j, k, error );

	      LidarDevices::message += msg;

	      if ( g_Verbose > 0 )
		printf( "%s", msg );

	      bcdi[j].matrix   = bcdk[j].matrix * bcdi[k].matrix;
	      bcdi[j].error    = error;
	      bcdi[j].valid    = true;
	      bcdj[i].matrix   = bcdi[j].matrix.inverse();
	      bcdj[i].error    = error;
	      bcdj[i].valid    = true;
	    }
	  }
	}
      }
    }
  }

  bool complete = basisChangesComplete();

  if ( g_Verbose > 0 )
  {
    printf( "calculateBasisChanges() %s\n", complete ? "complete" : "incomplete" );

    for ( int i = 0; i < devices.size(); ++i )
    { LidarBasisChanges &bcdi( devices[i]->basisChanges );
      for ( int j = 0; j < devices.size(); ++j )
      { LidarBasisChanges &bcdj( devices[j]->basisChanges );
        printf( "valid( id(%d) -> id(%d) ): %d %d\n", i, j, bcdi[j].valid, bcdj[i].valid );
      }
    }
  }
  
  if ( complete )
    LidarDevices::message += "complete\n";
  else
  {
    for ( int i = 0; i < ((int)devices.size())-1; ++i )
    { LidarBasisChanges &bcdi( devices[i]->basisChanges );
      for ( int j = i+1; j < devices.size(); ++j )
      { if ( !bcdi[j].valid )
	{ char msg[100];
	  sprintf( msg, "missing transformation: id(%d) -> id(%d)\n", i, j );
	  LidarDevices::message += msg;
	  if ( g_Verbose > 0 )
	    printf( "%s", msg );
	}
      }
    }
    LidarDevices::message += "incomplete\n";
  }
  
  return complete;
}

void
LidarDevices::startRegistration( bool refine ) 
{
  message = "";

  isRegistering      = true;
  refineRegistration = refine;
  
  LidarDeviceList devices( runningDevices() );

  for ( int d = ((int)devices.size())-1; d >= 0; --d )
  { LidarDevice &device = *devices[d];
    device.objectMaxDistanceBak = device.objectMaxDistance;
    device.objectMaxExtentBak   = device.objectMaxExtent;
    device.objectMaxDistance    = 0.07;
    device.objectMaxExtent      = 0.0;
  }
    
  startTime = getmsec();

  setAccum( true );
}

void
LidarDevices::calculateRegistion() 
{
  if ( size() == 0 )
    return;
  
  initBasisChanges();
  
  LidarDeviceList devices( runningDevices() );
  Matrix3H matrix;

  for ( int d = ((int)devices.size())-1; d >= 0; --d )
  {
    LidarDevice &device = *devices[d];
      
    device.lock();
    device.cleanupAccum( registerSec );
    device.detectObjects();
  }
  
  for ( int d0 = ((int)devices.size())-1; d0 > 0; --d0 )
  {
    LidarDevice &device0 = *devices[d0];
    if ( device0.isReady(false) )
    {
      for ( int d1 = d0-1; d1 >= 0; --d1 )
      {
	LidarDevice &device1 = *devices[d1];
        if ( device1.isReady(false) )
        {
//	  device0.lock();
//	  device1.lock();
      
	  Matrix3H meMatrix, otMatrix;
	  float distance = device1.calcTransformTo( device0, meMatrix, otMatrix, refineRegistration );
	  
//	  float matrixDist = device1.matrix.w.distance( otMatrix.w );

	  if ( distance < markerMatchDifference )
          {
	    float error                       = distance * distance;
	    
	    Matrix3H matrixDev1ToDev0       = otMatrix.inverse() * meMatrix;

	    device1.basisChanges[d0].matrix   = matrixDev1ToDev0;
	    device1.basisChanges[d0].valid    = true;
	    device1.basisChanges[d0].error    = error;
	    
	    device0.basisChanges[d1].matrix   = matrixDev1ToDev0.inverse();
	    device0.basisChanges[d1].valid    = true;
	    device0.basisChanges[d1].error    = error;
	  }

//	  device0.unlock();
//	  device1.unlock();
	}
      }
    }
  }
  
  
  for ( int d = ((int)devices.size())-1; d >= 0; --d )
  { LidarDevice &device = *devices[d];
    device.objectMaxDistance  = device.objectMaxDistanceBak;
    device.objectMaxExtent    = device.objectMaxExtentBak;
    device.unlock();
  }

  calculateBasisChanges();
  
  Matrix3H id;
  for ( int d1 = ((int)devices.size())-1; d1 >= 0; --d1 )
  {
    LidarDevice &device1 = *devices[d1];

    for ( int d0 = 0; d0 < size(); ++d0 )
    { if ( device1.basisChanges[d0].valid )
      { device1.setDeviceMatrix( device1.basisChanges[d0].matrix );
	device1.setViewMatrix  ( id );
	break;
      }
    }
  }
}

void
LidarDevices::finishRegistration()
{
  isRegistering = false;

  isCalculating = true;
  calculateRegistion();
  isCalculating = false;
  
  setAccum( false );
}

  
void
LidarDevices::loadRegistration( bool all )
{
  if ( all )
  {
    if ( size() == 0 )
      return;

    for ( int d = 0; d < size(); ++d )
      (*this)[d]->readMatrix();
   
    viewMatrix = (*this)[0]->viewMatrix;
    
    return;
  }

  LidarDeviceList devices( runningOrAllDevices(all) );
  if ( devices.size() == 0 )
    return;

  for ( int d = 0; d < devices.size(); ++d )
    devices[d]->readMatrix();

  viewMatrix = devices[0]->viewMatrix;
}


void
LidarDevices::saveRegistration( bool all, uint64_t timestamp )
{
  LidarDeviceList devices( runningOrAllDevices(all) );

  for ( int d = 0; d < devices.size(); ++d )
    if ( devices[d]->isReady() )
      devices[d]->writeMatrix( NULL, timestamp );
}


void
LidarDevices::resetRegistration( bool all )
{
  Matrix3H id;
 
  LidarDeviceList devices( runningOrAllDevices(all) );
  for ( int d = 0; d < devices.size(); ++d )
  { devices[d]->setDeviceMatrix( id );
    devices[d]->setViewMatrix  ( id );
  }
}


void
LidarDevices::setUseOutEnv( bool useOutEnv )
{
  LidarDeviceList devices( runningOrAllDevices(true) );

  for ( int d = 0; d < devices.size(); ++d )
    devices[d]->setUseOutEnv( useOutEnv );
}


void
LidarDevices::setCharacteristic( double char1, double char2, const char *devType )
{
  LidarDeviceList devices( runningOrAllDevices(true) );

  for ( int d = 0; d < devices.size(); ++d )
    devices[d]->setCharacteristic( char1, char2, devType );
}


void
LidarDevices::update()
{
  if ( isRegistering )
  {
    uint64_t currentTime = getmsec();
    uint64_t milliSec    = currentTime - startTime;

    if ( milliSec >= registerSec * 1000 )
      finishRegistration();
  }
  else
  {
    for ( int d = 0; d < size(); ++d )
    {
      if ( (*this)[d]->reopenTime > 0 && (*this)[d]->reopenTime <= getmsec() )
      { (*this)[d]->open();
	(*this)[d]->reopenTime = 0;
      }
    }
  }
}

bool
LidarDevices::parseArg( int &i, const char *argv[], int &argc ) 
{
  bool success = true;

  if ( strcmp(argv[i],"lidar.register.sec") == 0 )
  {
    registerSec = atof( argv[++i] );
  }
  else if ( strcmp(argv[i],"lidar.register.maxObjectDistanceOfMarkers") == 0 )
  {
    LidarObject::maxMarkerDistance = atof( argv[++i] );
  }
  else if ( strcmp(argv[i],"lidar.register.markerMatchDifference") == 0 )
  {
    LidarObject::maxMarkerDistance = atof( argv[++i] );
  }
  else if ( strcmp(argv[i],"lidar.env.scanSec") == 0 )
  {
    envScanSec = atof( argv[++i] );
  }
  else
    success = false;
    
  return success;
}

void
LidarDevices::printArgHelp() const
{
//  printArgHelpImpl( "lidar.env.scanSec",     envScanSec,	"\ttime in sec used to scan the environment" );
  printArgHelpImpl( "lidar.register.sec",    registerSec,     "\ttime in sec used to register markers" );
  printArgHelpImpl( "lidar.register.maxObjectDistanceOfMarkers",    LidarObject::maxMarkerDistance,     "\tmaximum distance between two flat objects to be treated as marker" );
  printArgHelpImpl( "lidar.register.markerMatchDifference", markerMatchDifference,     "\tmaximum difference between markers to treat them as the same marker" );
}

void
LidarDevices::copyArgs( LidarDevice *argDevice )
{
  for ( int d = 0; d < size(); ++d )
  {
    LidarDevice *device( (*this)[d] );
    if ( device != argDevice )
      device->copyArgs( argDevice );
  }
}

bool
LidarDevices::deviceInGroup( LidarDevice &device, const char *groupName ) const
{
  std::string gn( groupName );
  
  if ( gn.empty() )
    return false;

  if ( gn == "all" )
    return true;

  KeyValueMap *map;
  if ( !LidarDeviceGroup::groups.get( groupName, map ) )
    return false;

  KeyValueMap resolvedMap;
  resolvedGroups.get( groupName, resolvedMap );

  std::string value;

  std::string nikName( device.getNikName() );
  if ( map->get( nikName.c_str(), value ) && value == "device" )
    return true;
  
  std::string baseName( device.getBaseName() );
  if ( map->get( baseName.c_str(), value ) && value == "device" )
    return true;
  
///  printf( "3, %s\n", baseName.c_str() );
  
  if ( resolvedMap.get( baseName.c_str(), value ) && value == "device" )
    return true;
  
  return false;
}

bool
LidarDevices::isActive( const char *groupName ) const
{
  if ( _activeDevices.groupName == "all" )
    return true;
  
  std::vector<std::string> list( split( _activeDevices.groupName, ',' ) );

  for ( int i = 0; i < list.size(); ++i )
    if ( list[i] == groupName )
      return true;
  
  return false;
}


void
LidarDevices::activateGroup( const char *groupName )
{
  if ( _activeDevices.groupName == groupName )
    return;

  _activeDevices.groupName = groupName;

  _activeDevices.resize( 0 );
  _inactiveDevices.resize( 0 );
    
  std::vector<std::string> list( split( _activeDevices.groupName, ',' ) );

  for ( int i = 0; i < list.size(); ++i )
  {
    groupName = list[i].c_str();
    
    for ( int i = 0; i < size(); ++i )
    { LidarDevice *device = (*this)[i];
      if ( deviceInGroup( *device, groupName )  )
	_activeDevices.addMember( device );
    }
  }

  for ( int i = 0; i < size(); ++i )
  { LidarDevice *device = (*this)[i];
    if ( !_activeDevices.isMember( device )  )
      _inactiveDevices.addMember( device );
  }
}

LidarDeviceList &
LidarDevices::activeDevices()
{
  return _activeDevices;
}

LidarDeviceList 
LidarDevices::runningDevices( bool onlyValidDevices ) const
{
  LidarDeviceList devices;
  
  for ( int i = 0; i < size(); ++i )
  {
    LidarDevice *device = (*this)[i];

    if ( device->isOpen() )
    { if ( !onlyValidDevices || (device->isReady() && device->dataReceived) )
	devices.push_back( device );
    }
  }

  return devices;
}

LidarDeviceList 
LidarDevices::allDevices() const
{
  LidarDeviceList devices;
  
  for ( int i = 0; i < size(); ++i )
    devices.push_back( (*this)[i] );

  return devices;
}

LidarDeviceList 
LidarDevices::runningOrAllDevices( bool all ) const
{
  if ( all )
    return allDevices();

  return runningDevices();
}

LidarDeviceList 
LidarDevices::devicesInGroup( const char *groupName ) const
{
  LidarDeviceList devices;
  
  for ( int i = 0; i < size(); ++i )
  { LidarDevice *device = (*this)[i];
    if ( deviceInGroup( *device, groupName ) )
      devices.push_back( (*this)[i] );
  }
  
  return devices;
}

LidarDeviceList
LidarDevices::remoteDevices() const
{
  LidarDeviceList devices;
  
  for ( int i = 0; i < size(); ++i )
  { LidarDevice *device = (*this)[i];
    if ( device->inFileName.empty() && !device->inVirtUrl.empty() )
      devices.push_back( (*this)[i] );
  }
  
  return devices;
}



/***************************************************************************
*** 
*** Lidar
***
****************************************************************************/

static size_t
WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{ ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}


static bool
readPlatform()
{
  std::ifstream stream( "/etc/Model" );
  if ( stream.is_open() )
    stream >> g_Model;
  
  if ( g_Model.empty() )
    g_Model = "unknown";

  return true;
}

void (*Lidar::exitHook)() = NULL;

struct sigaction old_action;

static void shutDownDevices()
{
  if ( g_StatusIndicatorSupported )
  { std::string cmd( hardwareDir );
    cmd += "setStatusIndicator.sh stopped";
    system( cmd.c_str() );
  }

  int count = 0;
  if ( g_DeviceList.size() > 0 )
  { for ( int i = ((int)g_DeviceList.size())-1; i >= 0; --i )
      if ( g_DeviceList[i]->isOpen() )
	count += 1;
  }
      
  g_Shutdown = true;

  if ( count > 0 )
  {
    if ( !g_LogFileName.empty() || g_Verbose )
      Lidar::log( "shutting down %d lidar devices", count );

    for ( int i = ((int)g_DeviceList.size())-1; i >= 0; --i )
    { LidarDevice &device( *g_DeviceList[i] );
      if ( device.isOpen() )
	device.close();
    }

    uint64_t startTime = getmsec();
    uint64_t milliSec  = 0;

    bool allClosed = false;
    while ( !allClosed && milliSec < 1500  )
    {
      allClosed = true;
    
      for ( int i = ((int)g_DeviceList.size())-1; i >= 0; --i )
	if ( g_DeviceList[i]->isOpen() )
	  allClosed = false;

      if ( !allClosed )
	usleep( 4*1000 );

      uint64_t currentTime = getmsec();
      milliSec    = currentTime - startTime;
    }
  }
}

static void sigHandler( int sig )
{
  Lidar::error( "Lidar [%d] caught signal %d\n", gettid(), sig );
}

static void sigKill(int sig)
{
  struct sigaction new_action;

  if ( !g_LogFileName.empty() || g_Verbose )
    Lidar::log( "STOP Lidar::sigKill [%d] caught signal %d\n", gettid(), sig );

  new_action.sa_handler = SIG_IGN;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;

  sigaction (sig,  &new_action, NULL);

  exit( 0 );
}

static void setSignalHandler( int sig, struct sigaction &new_action )
{
//  if ( g_Verbose )
//    Lidar::info( "[%d] catch signal %d", gettid(), sig );
  
  if ( sig == SIGPIPE )
  {
    struct sigaction sa;
    sa.sa_handler = sigHandler;
    sigemptyset (&new_action.sa_mask);
    sigaction(sig, &sa, NULL);
  }
  else
  {
    sigaction (sig, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
      sigaction (sig, &new_action, NULL);
  }
}

/* libmicrohttpd
static void
catcher (int sig)
{
}

static void
ignore_sigpipe ()
{
  struct sigaction oldsig;
  struct sigaction sig;

  sig.sa_handler = &catcher;
  sigemptyset (&sig.sa_mask);
#ifdef SA_INTERRUPT
  sig.sa_flags = SA_INTERRUPT;  // SunOS
#else
  sig.sa_flags = SA_RESTART;
#endif
  if (0 != sigaction (SIGPIPE, &sig, &oldsig))
    fprintf (stderr,
             "Failed to install SIGPIPE handler: %s\n", strerror (errno));
}
*/

void
Lidar::setSignalHandlers()
{
  struct sigaction new_action;

  /*
  printf( "SIGINT:  %d\n", SIGINT  );
  printf( "SIGHUP:  %d\n", SIGHUP  );
  printf( "SIGPIPE: %d\n", SIGPIPE );
  printf( "SIGTERM: %d\n", SIGTERM );
  */
  new_action.sa_handler = sigKill;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;

  setSignalHandler( SIGINT,  new_action );
  setSignalHandler( SIGHUP,  new_action );
  setSignalHandler( SIGPIPE, new_action );
  setSignalHandler( SIGTERM, new_action );
}


static void exit_handler(void)
{
  if ( Lidar::exitHook != NULL )
    Lidar::exitHook();
  
  shutDownDevices();
}

bool
Lidar::initialize()
{
  if ( g_IsInitialized )
    return true;

  g_IsInitialized = true;
  
  hardwareDir = LidarDevice::installDir + "hardware/";

  g_PoweringEnabledFileName = hardwareDir + "LidarPower.enable";

  std::string cmd( hardwareDir );
  cmd += "raspiModel.sh"; 
  g_Model = exec( cmd.c_str() );

  if ( g_Model.empty() )
    readPlatform();
  
  if ( g_Verbose )
    Lidar::info( "running on platform: %s", g_Model.c_str() );

  readPoweringSupported();
  std::string isSupported = (g_PoweringSupported ? "true":"false");
  
  if ( g_Verbose )
    Lidar::info( "lidar  powering  supported: %s", isSupported.c_str() );

  if ( g_UseStatusIndicator )
  {
    cmd = hardwareDir;
    cmd += "setStatusIndicator.sh isSupported";
    isSupported = exec( cmd.c_str() );
  
    g_StatusIndicatorSupported = (isSupported == "true");

    if ( g_Verbose )
      Lidar::info( "status indicator supported: %s", isSupported.c_str() );
  }

  setSignalHandlers();

  atexit( exit_handler );

  return( true );
}


void
Lidar::exit()
{
  if ( Lidar::exitHook != NULL )
    Lidar::exitHook();

  shutDownDevices();
}
