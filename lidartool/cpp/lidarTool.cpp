// copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
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


#if USE_LIBLO
#undef  USE_LIBLO
#define HAS_LIBLO 1
#endif

//#undef USE_WEBSOCKETS

#if USE_MOSQUITTO
#undef  USE_MOSQUITTO
#define HAS_MOSQUITTO 1
#endif

#if USE_LUA
#undef  USE_LUA
#define HAS_LUA 1
#endif

#if USE_INFLUXDB
#undef  USE_INFLUXDB
#define HAS_INFLUXDB 1
#endif

#include "lidarTrack.h"
#include "lidarTool.h"
#include "packedPlayer.h"
#include "webAPI.h"

#if USE_WEBSOCKETS
#include "trackableHUB.h"
#endif

#include <limits.h>
#include <dirent.h>
#include <filesystem>

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

class DeviceUI
{
public:
  bool show;
  
  DeviceUI()
    : show( true )
    {}
  
};
  

static LidarDevices g_Devices;
static std::vector<LidarBasisChanges>   g_DeviceBasisChanges;
static std::vector<DeviceUI> 		g_DeviceUI;
static KeyValueMap 			g_DeviceFailed;
static std::string 			defaultDeviceType;
static std::string 			g_PackedInFileName;
static std::string 			g_LidarInFileTemplate;
static std::string 			g_LidarOutFileTemplate;
static std::string 			g_RunningMode( "unknown" );
static std::string 			g_AppStartDate;

static int 				webserver_port   = 8080;
static int 				remote_port      = 8080;
static bool				g_IsStarted      = false;
static bool				g_OpenOnStart    = true;
static bool  				g_UseCheckPoints = false;
static bool  				g_ExpertMode     = false;

static double				g_MaxFps = 60.0;
static int				g_Verbose = 0;
static std::string 			g_ID( "Default" );
static KeyValueMap 			g_UsedGroups;
static std::string			g_HUBHostName( "localhost" );
static bool				g_IsHUB   = false;
static bool				g_HasHUB  = false;
static bool				g_HUBStarted  = false;
static int				g_HUBPort = 5000;
static WebAPI				g_HUBAPI;
static std::string			g_HUBAPIURL;

static std::string 			g_SensorINFileName( "SensorIN.txt" );
static std::string 			g_DefaultReportSpinningScript( "reportSpinningSensors.sh" );
static std::string			g_SpinningReportScript;
static std::string			g_FailureReportScript;
static std::string			g_ErrorLogFile;
static std::string			g_LogFile;
static int			        g_ErrorLogHtmlLines = 20;
static std::string			g_ImageSuffix( ".jpg" );
static std::string			g_LogSuffix  ( ".log" );
static std::string                      uiImageType = "jpg";
static std::string		        uiMimeType  = "image/jpg";
static std::string			bluePrintFileName;
static std::string			bluePrintLoResFileName;
static std::string			bluePrintHiResFileName;
static std::string 		        bluePrintMimeType = "image/jpg";
static std::string			simulationEnvMapFileName;
static rpImg 				simulationEnvMapImg;
static std::string			trackOcclusionMapFileName;
static rpImg 				trackOcclusionMapImg;
static std::atomic_bool			trackOcclusionMapLocked( false );
static std::string			obstacleFileName;
static rpImg 				obstacleImg;
static float            		obstaclePPM = 1;
static float            		obstacleExtentX = 1;
static bool	            		g_UseObstacle = true;

static Matrix3H 			bpMatrix,  bpMatrixInv;
static Matrix3H 			obsMatrix, obsMatrixInv;

static std::string      		obstacleExtent( "1" );
static std::string			nikNamesFileName( "nikNames.json" );
static std::string			nikNamesSimulationModeFileName( "nikNamesSimulationMode.json" );

static KeyValueMap 			deviceNikNames;
static std::string			g_NikNameFileName;
static KeyValueMap	 		blueprints;
static std::string			blueprintsFileName;
static std::string			groupsFileName;
static std::string			g_Config;
static std::string			g_InstallDir( "./" );
static std::string			g_RealInstallDir( "./" );
static std::string			g_HTMLDir( "./html/" );

static LidarTrack       g_Track;
static bool	        g_DoTrack = false;
static std::mutex       g_TrackMutex;


static std::mutex       webMutex;
static bool	        imgInProcess = false;
static std::string      bluePrintExtent( "10" );
static float            bluePrintExtentPixels = 0;
static float            bluePrintExtentX = 10;
static float            bluePrintExtentY = 10;
static float            bluePrintPPM     = 1;
static int 		colChannels = 3;

class frameInfo
{
  public:
  
  uint64_t	timestamp;
  int		frameTime;
  
  frameInfo( uint64_t timestamp, int frameTime )
    : timestamp( timestamp ),
      frameTime( frameTime )
    {}
};

  

static AFPS 	   frameRate;
static const int   defaultFrameTime  = 300;
static const int   minFrameTime      = 1000/20;
static const float maxComputeUsage   = 0.8;
static float  	   computeWeight     = 1.0;
static std::vector<frameInfo> frameTimeVec;
static int 	 frameTimeAverage  = defaultFrameTime;

static const unsigned char g_Color[][4] = { 
  { 255, 128, 128, 255 },
  { 128, 255, 128, 255 },
  { 128, 128, 255, 255 },
  { 255, 128, 255, 255 },
  { 128, 255, 255, 255 },
  
};

static std::map<std::string,LidarPainter> painters;
static uint64_t       webId;

/***************************************************************************
*** 
*** Image
***
****************************************************************************/

#include <iostream>

using namespace cimg_library;

/***************************************************************************
*** 
*** Helper
***
****************************************************************************/

static void 
log( const char *format  )
{
  if ( g_LogFile.empty() && !g_Verbose )
    return;
  
  TrackBase::log( format );
}


static void 
log( const char *format, const char *arg1  )
{
  if ( g_LogFile.empty() && !g_Verbose )
    return;
  
  TrackBase::log( format, arg1 );
}

static void 
log( const char *format, const char *arg1, const char *arg2  )
{
  if ( g_LogFile.empty() && !g_Verbose )
    return;
  
  TrackBase::log( format, arg1, arg2 );
}


static KeyValueMap g_EnvVar;

static bool
replaceEnvVar( std::string &str )
{
  if ( g_EnvVar.size() == 0 )
  { extern char **environ;
    for ( int i = 0; environ[i] != NULL; ++i )
    { std::string envVar( environ[i] );
      char *val = (char*)strchr(envVar.c_str(), '=' );
      if ( val != NULL )
      { 
	std::string value( &val[1] );
	val[0] = '\0';
	
	std::string key( "$(" );
	key += envVar.c_str();
	key += ")";

	if ( g_EnvVar.count(key) )
	  g_EnvVar[key] = value;
	else
	  g_EnvVar.emplace( std::make_pair(key,value) );

//	printf( "%s = %s\n", key.c_str(), value.c_str() );
      }
    }
  }
  
  bool result = false;
  
  for ( KeyValueMap::iterator iter( g_EnvVar.begin() ); iter != g_EnvVar.end(); iter++ )
    if ( replace( str, iter->first, iter->second ) )
      result = true;
  
  return result;
}
  
static void
replaceEnvVar( KeyValueMap &map )
{ 
  for ( KeyValueMap::iterator iter( map.begin() ); iter != map.end(); iter++ )
    replaceEnvVar( iter->second );
}

static std::string
applyDateToString( const char *string, uint64_t timestamp=0 )
{
  if ( strchr( string, '\%' ) == NULL )
    return std::string( string );
     
  if ( timestamp == 0 )
    timestamp = getmsec();
  
  time_t t = timestamp / 1000;
  struct tm timeinfo = *localtime( &t );

  const int maxLen = 2000;
  char buffer[maxLen+1];
  strftime( buffer, maxLen, string, &timeinfo );

//    printf( "templateToFileName %s %s %ld\n", buffer, logFileTemplate.c_str(), timestamp );

  return std::string( buffer );
}

static void
addFrameTime( uint64_t starttime, uint64_t endtime )
{
  frameTimeVec.push_back( frameInfo( starttime, endtime-starttime) );
  
  while ( frameTimeVec.back().timestamp - frameTimeVec.front().timestamp > 1000 ) // keep all infos within the last second
  {
    frameTimeVec.erase( frameTimeVec.begin() );

    if ( frameTimeVec.size() > 0 )
    {
      int sum = 0;
      for ( int i = ((int)frameTimeVec.size())-1; i >= 0; --i )
	sum += frameTimeVec[i].frameTime;

      int   average = sum / frameTimeVec.size();
      float compute = 1.0;

      if ( sum > 1000 * maxComputeUsage )
      { float alpha = sum / (1000 * maxComputeUsage);
	average *= alpha;
	compute /= alpha;
      }

      float weight = 0.25;
	
      frameTimeAverage = (weight * average) + (1.0-weight) * frameTimeAverage;
      computeWeight    = (weight * compute) + (1.0-weight) * computeWeight;

//      printf( "compute: %g -> %g\n", compute, computeWeight );
    
      
      if ( frameTimeAverage < minFrameTime )
	frameTimeAverage = minFrameTime;
    }
    else
      frameTimeAverage = defaultFrameTime;
  }
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


static const char *
getIP()
{
  static char ip[32] = "";
  if ( ip[0] != '\0' )
    return ip;
  
  FILE *f = popen("ip a | grep 'scope global' | grep -v ':' | awk '{print $2}' | cut -d '/' -f1", "r");
  int c, i = 0;
  while ((c = getc(f)) != EOF) i += sprintf(ip+i, "%c", c);
  pclose(f);
  return ip;
}

static const char *
getMAC()
{
  static char mac[32] = "";
  if ( mac[0] == '\0' )
  {
    FILE *f = popen("ip a l eth0 | awk '/ether/ {print $2}' | tr -d '[:space:]'", "r");
    int c, i = 0;
    while ((c = getc(f)) != EOF) i += sprintf(mac+i, "%c", c);
    pclose(f);
    if ( mac[0] == '\0' )
      sprintf( mac, "Undefined" );
  }
  
  if ( mac[0] == '\0' || mac[0] == 'U' )
    return NULL;
  
  return mac;
}

static void
setInstallDir( const char *executable )
{
  g_InstallDir = filePath( executable );
  if ( g_InstallDir.empty() )
    g_InstallDir = "./";

#ifdef PATH_MAX
  const int path_max = PATH_MAX;
#else
  int path_max = pathconf(executable, _PC_PATH_MAX);
  if (path_max <= 0)
    path_max = 4096;
#endif
  
  char buf[path_max];
  char *res = realpath(executable, buf);
  if ( res != NULL )
  {
    g_RealInstallDir = filePath( buf );
    g_HTMLDir        = g_RealInstallDir + "html/";

    LidarDevice::setInstallDir( g_RealInstallDir.c_str() );
  }
}


static bool
isSymLink( std::string deviceName, std::string &otherName )
{
#ifndef _WIN32
  struct stat buf;
  
  if (lstat(otherName.c_str(), &buf) == 0)
  {
    char linkname[buf.st_size + 1];

    readlink(otherName.c_str(), linkname, buf.st_size + 1);
    linkname[buf.st_size] = '\0';

    return deviceName == linkname;
  }
#endif
  return false;
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

static bool
valueFromConfigFile( std::string fileName, const char *match, std::string &result )
{
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
      { std::string value( pair[1] );
	value = trim( value );
	pair = split( value, '#' );
	value = pair[0];
	result = trim( value );
	return true;
      }
    }
  }

  return false;
}

  
static bool
testConfigDir( std::string &dir )
{
  std::string testDir( dir );
  rtrim( testDir, "/" );

  if ( !fileExists( testDir.c_str() ) )
  { 
    if ( g_InstallDir.empty() )
      return false;
    
    testDir = g_InstallDir + testDir;

    if ( !fileExists( testDir.c_str() ) )
      return false;
  }
  
  std::string conf( std::filesystem::path( testDir ).filename() );
  
  testDir += "/";
  
  LidarDevice::configDir = testDir;
  g_Config = conf;

  return true;
}


static bool
testConf( std::string &conf )
{
  if ( conf.empty() )
    return false;

  std::string confDir( conf );
  
  if ( testConfigDir( confDir ) )
    return true;

  confDir = "conf/";
  confDir += conf;

  return testConfigDir( confDir );
}


static void
readConfigDir()
{
  std::string fileName( "config.txt" );
  std::string confDir, conf;

  valueFromConfigFile( fileName, "conf", conf );

  if ( conf.empty() )
  { const char *env = std::getenv( "LIDARCONF" );
    if ( env != NULL && env[0] != '\0' )
      conf = env;
  }
  
  if ( conf.empty() )
  {
    fileName = g_InstallDir + "configDir.txt";

    std::ifstream stream( fileName );
    if ( stream.is_open() )
    {
      if ( g_Verbose )
	TrackGlobal::info( "reading config dir file '%s'", fileName.c_str() );

      std::string dir;
      stream >> dir;
      if ( testConf( dir ) )
	return;
    }
  }
  
  testConf( conf );
}


static bool
writeConfigDir( const char *dirName )
{
  std::string fileName = g_InstallDir + "configDir.txt";
  std::ofstream stream( fileName );
  
  if ( !stream.is_open() )
    return false;
  
  stream << dirName << "\n";

  return true;
}


static bool
writeNikNames()
{ return TrackGlobal::WriteKeyValues( deviceNikNames, g_NikNameFileName.c_str() );
}

static bool
readNikNames()
{ return TrackGlobal::ReadKeyValues( deviceNikNames, g_NikNameFileName.c_str(), false );
}

static void
printNikName( std::string &sn )
{ 
  std::map<std::string,std::string>::iterator iter( deviceNikNames.find(sn) );
    
  std::string nikName;
      
  if ( iter != deviceNikNames.end() )
  { nikName = iter->second;
  }
  else
  {
    for ( int i = 0; i < LidarDevice::maxDevices; ++i )
    {
      std::string dev( "/dev/lidar" );
      dev += std::to_string( i );
      
      if ( !fileExists( dev ) )
      { nikName  = "lidar";
	nikName += std::to_string( i );
	break;
      }
    }
  }

  if ( sn.empty() )
    printf( "%s", nikName.c_str());
  else
    printf( "lidar%s %s", sn.c_str(), nikName.c_str());
}

static bool
removeNikName( const char *nikName )
{ deviceNikNames.remove( nikName );
  return writeNikNames();
}

static bool
clearNikNames()
{ deviceNikNames = KeyValueMap();
  return writeNikNames();
}

static bool
renameNikName( const char *name, const char*newName )
{ deviceNikNames.rename( name, newName );
  return writeNikNames();
}

static bool
setNikName( const char *key, const char *nikName )
{ deviceNikNames.set( key, nikName );
  return writeNikNames();
}

      
/***************************************************************************
*** 
*** BluePrint
***
****************************************************************************/

static bool
writeBlueprints()
{
  float x, y;
  if ( !blueprints.get( "x", x ) )
    blueprints.setDouble( "x", 0.0 );
  if ( !blueprints.get( "y", y ) )
    blueprints.setDouble( "y", 0.0 );

  return TrackGlobal::WriteKeyValues( blueprints, blueprintsFileName.c_str() );
}

static bool
readBlueprints()
{ return TrackGlobal::ReadKeyValues( blueprints, blueprintsFileName.c_str() );
}

static void
setBlueprintValue( const char *key, const char *value )
{ blueprints.set( key, value );
}

static void
removeBlueprintValue( const char *key )
{ blueprints.remove( key );
}

static bool
setBluePrints( bool first=false )
{
  if ( readBlueprints() )
  {
    blueprints.get( "image",       bluePrintFileName );
    blueprints.get( "image_lores", bluePrintLoResFileName );
    blueprints.get( "image_hires", bluePrintHiResFileName );
    blueprints.get( "extent",      bluePrintExtent );

    blueprints.get( "x", bpMatrix.w.x );
    blueprints.get( "y", bpMatrix.w.y );

    blueprints.get( "simulationEnvMap",  simulationEnvMapFileName );
    blueprints.get( "trackOcclusionMap", trackOcclusionMapFileName );
    blueprints.get( "obstacleImage",     obstacleFileName );
    blueprints.get( "obstacleExtent",    obstacleExtent );
    
    if ( obstacleFileName.empty() || obstacleExtent.empty() )
      g_UseObstacle = false;
  }

  int width, height;
  try {
    rpImg img( TrackGlobal::getConfigFileName(bluePrintFileName.c_str()).c_str() );
    width  = img.width();
    height = img.height();
  } catch(CImgException& e) {
    TrackGlobal::error( "can't read blueprint image file %s", bluePrintFileName.c_str() );
    return false;
  }

  if ( first && simulationEnvMapFileName.empty() )
    simulationEnvMapFileName = bluePrintFileName;

  std::string ext = std::filesystem::path(bluePrintFileName.c_str()).extension();

  if ( !ext.empty() )
  {
    ext = std::string(&(ext.c_str()[1]));
    tolower( ext );
    ext = "image/" + ext;
    bluePrintMimeType = ext;
  }

  std::vector<std::string> pair( split(bluePrintExtent,'=',2) );
  if ( pair.size() == 2 )
  { bluePrintExtentPixels = ::atoi( pair[0].c_str() );
    bluePrintExtentX      = ::atof( pair[1].c_str() );
  }
  else
    bluePrintExtentX      = ::atof(pair[0].c_str() );

  colChannels = 4;

  uiImageType = "png";
  uiMimeType  = "image/png";

  if ( bluePrintExtentPixels != 0 )
    bluePrintExtentX *= width / (double)bluePrintExtentPixels;

  bluePrintPPM = width / bluePrintExtentX;
  
  if ( g_Verbose )
    TrackGlobal::info( "using blueprint image %s extent=%s (%dx%d)", bluePrintFileName.c_str(), bluePrintExtent.c_str(), width, height );

  bluePrintExtentY = (bluePrintExtentX * height) / width;

  return true;
}

static bool
createSimulationEnvMap( LidarDevice &device )
{
  rpImg &oImg( simulationEnvMapImg );
  int ow = oImg.width();
  int oh = oImg.height();

  const float maxRadius = device.info.spec.maxRange;
  const float radiusRes = 1.0 / bluePrintPPM;
  const int   size      = device.envSamples.size();

  float angleRes  = asin( 1.0 / (bluePrintPPM*maxRadius) );
  const float minAngleRes = 2.0*M_PI / size;
  if ( angleRes < minAngleRes )
    angleRes = minAngleRes;
  
  int numSteps = 2*M_PI / angleRes + 1;

  LidarSampleBuffer &envRawSamples( device.envRawSamples );

  device.lock();

  for ( int i = ((int)envRawSamples.size())-1; i >= 0; --i )
  {
    LidarSample &envRawSample( envRawSamples[i] );
    envRawSample.quality  = -1;
    envRawSample.angle    = device.angleByAngIndex( i );
    envRawSample.distance = 0;
    envRawSample.coord    = Vector3D( 0, 0, 0 );
    envRawSample.coord.x  += 0.001;
  }

  int lastAngIndex = -1;
  for ( int i = 0; i < numSteps; ++i )
  {
    double angle  = i * angleRes;
    double sina   = sin( angle );
    double cosa   = cos( angle );
    
    int angIndex  = device.angIndexByAngle( angle );
    LidarSample &envRawSample( envRawSamples[angIndex] );

    for ( float r = radiusRes; r <= maxRadius; r += radiusRes )
    {
      float x = r * sina;
      float y = r * cosa;

      Vector3D coordDev( x, y, 0.0 );
      Vector3D coord   ( device.matrix * coordDev );
      Vector3D coordMap = coord - bpMatrix.w;

      int ox =   bluePrintPPM * coordMap.x + ow/2;
      int oy =  -bluePrintPPM * coordMap.y + oh/2;

      if ( ox >= 0 && ox < ow && oy >= 0 && oy < oh )
      {
	if ( oImg( ox, oy, 0, 0 ) > 128 )
        { envRawSample.quality  = 10;
	  //	  envRawSample.angle    = angle;
       	  envRawSample.distance = r;
	  envRawSample.coord    = coord;
	  break;
	}
      }	
    }

    int endIndex = (i < numSteps-1 ? angIndex : envRawSamples.size());

    for ( int fillIndex = lastAngIndex+1; fillIndex < angIndex; ++fillIndex )
    {
      double angle  = fillIndex * angleRes;

      LidarSample &fillSample( envRawSamples[fillIndex] );
      fillSample.quality = envRawSample.quality;

      fillSample.distance = envRawSample.distance;
      float x = fillSample.distance * sin( angle );
      float y = fillSample.distance * cos( angle );

      Vector3D coordDev( x, y, 0.0 );
      Vector3D coord   ( device.matrix * coordDev );
      fillSample.coord = coord;
    }

    lastAngIndex = angIndex;
  }

  device.envValid = true;
  
  for ( int angIndex = LidarDevice::numSamples-1; angIndex >= 0; --angIndex )
    device.envSamples[angIndex] = envRawSamples[angIndex];
  
  //  device.updateEnv();
  device.unlock();
  //  device.processEnv();

  return true;
}

static bool
createSimulationEnvMaps( LidarDeviceList &devices )
{
  bool success = true;
  
  for ( int i = 0; i < devices.size(); ++i )
    if ( !createSimulationEnvMap( *devices[i] ) )
      success = false;
  
  return success;
}

static bool
setSimulationEnvMap()
{
  int width, height;
  try {
    rpImg img( TrackGlobal::getConfigFileName(simulationEnvMapFileName.c_str()).c_str() );
    simulationEnvMapImg = img; 
    width  = img.width();
    height = img.height();
  } catch(CImgException& e) {
    TrackGlobal::error( "can't read simulation environment map image file %s", simulationEnvMapFileName.c_str() );
    return false;
  }

  if ( g_Verbose )
    TrackGlobal::info( "using simulation environment map image %s  (%dx%d)", simulationEnvMapFileName.c_str(), width, height );

  return true;
}

static int
trackableMask( Trackable<BlobMarkerUnion> &trackable )
{
  int maskBits = 0;
    
  if ( trackOcclusionMapLocked )
    false;
  
  rpImg &oImg( trackOcclusionMapImg );
  int ow = oImg.width();
  int oh = oImg.height();

  Vector3D coord( trackable.p[0], trackable.p[1], 0.0 );
  Vector3D coordMap = coord - bpMatrix.w;

  int ox =   bluePrintPPM * coordMap.x + ow/2;
  int oy =  -bluePrintPPM * coordMap.y + oh/2;

  if ( ox >= 0 && ox < ow && oy >= 0 && oy < oh )
  {
    if ( oImg( ox, oy, 0, 3 ) < 128 )
      return maskBits;

    bool red   = (oImg( ox, oy, 0, 0 ) > 128);
    bool green = (oImg( ox, oy, 0, 1 ) > 128);
    bool blue  = (oImg( ox, oy, 0, 2 ) > 128);

    if ( red && green )
     	maskBits |= Trackable<BlobMarkerUnion>::Occluded;
    else
    {
      if ( red )
	maskBits |= Trackable<BlobMarkerUnion>::Portal;

      if ( green )
	maskBits |= Trackable<BlobMarkerUnion>::Green;
    }

    if ( oImg( ox, oy, 0, 2 ) > 128 )
      maskBits |= Trackable<BlobMarkerUnion>::Private;
  }
  
  return maskBits;
}

static bool
setTrackOcclusionMap()
{
  int width, height;

  try {
    rpImg img( TrackGlobal::getConfigFileName(trackOcclusionMapFileName.c_str()).c_str() );
    trackOcclusionMapLocked = true;
    trackOcclusionMapImg = img; 
    trackOcclusionMapLocked = false;
    width  = img.width();
    height = img.height();
  } catch(CImgException& e) {
    TrackGlobal::error( "can't read track occlusion image file %s", trackOcclusionMapFileName.c_str() );
    return false;
  }

  if ( g_Verbose )
    TrackGlobal::info( "using track occlusion image %s  (%dx%d)", trackOcclusionMapFileName.c_str(), width, height );

  g_Track.m_Stage->trackableMask = trackableMask;

  return true;
}

static bool
commitFileToCheckpoint( const char *fileName, uint64_t timestamp )
{
  std::string fromFileName( TrackGlobal::getConfigFileName(fileName).c_str() );

  if ( !fileExists( fromFileName.c_str() ) )
    return false;

  std::string toFileName( TrackGlobal::getConfigFileName(fileName, NULL, NULL, TrackGlobal::WriteCreateCheckPoint, timestamp ) );

  if ( toFileName == fromFileName || toFileName.empty() )
    return false;

  if ( g_Verbose )
    TrackGlobal::info( "copying %s  ->  %s", fromFileName.c_str(), toFileName.c_str() );

  std::filesystem::copy( fromFileName, toFileName );

  return true;
}

static bool
snapshotTrackOcclusionMap( uint64_t timestamp )
{
  std::string fromFileName( TrackGlobal::getConfigFileName(trackOcclusionMapFileName.c_str()).c_str() );

  if ( !fileExists( fromFileName.c_str() ) )
  { TrackGlobal::error( "can't read track occlusion image file %s", trackOcclusionMapFileName.c_str() );
    return false;
  }

  std::string toFileName( TrackGlobal::getConfigFileName(trackOcclusionMapFileName.c_str(), NULL, NULL, TrackGlobal::WriteCreateCheckPoint, timestamp ) );

  if ( toFileName == fromFileName || toFileName.empty() )
    return false;

  if ( g_Verbose )
    TrackGlobal::info( "copying track occlusion image %s  ->  %s", fromFileName.c_str(), toFileName.c_str() );

//  std::filesystem::copy( fromFileName, toFileName );

  return true;
}

static bool
obstacleSimulationCheckOverlap( LidarDevice &device )
{
  rpImg &oImg( obstacleImg );
  int ow = oImg.width();
  int oh = oImg.height();
  
  int owh = ow / 2;
  int ohh = oh / 2;

  Vector3D coordMap = obsMatrixInv * device.matrix.w;

  float maxRange = device.info.spec.maxRange;

  int ox0 =   (coordMap.x-maxRange) * obstaclePPM + owh;
  int oy0 =  (-coordMap.y-maxRange) * obstaclePPM + ohh;

  int ox1 =   (coordMap.x+maxRange) * obstaclePPM + owh;
  int oy1 =  (-coordMap.y+maxRange) * obstaclePPM + ohh;

  if ( ox0 >= ow || ox1 < 0 )
    return false;

  if ( oy0 >= oh || oy1 < 0 )
    return false;

 return true;
}

static bool
obstacleSimulationRay( LidarDevice &device, LidarRawSample &sample, float &angle, float &distance )
{
  rpImg &oImg( obstacleImg );
  int ow = oImg.width();
  int oh = oImg.height();

  float maxRadius = distance;
  float radiusRes = 1.0 / obstaclePPM;

  double sina   = sin( angle );
  double cosa   = cos( angle );
    
  for ( float r = radiusRes; r <= maxRadius; r += radiusRes )
  {
    float x = r * sina;
    float y = r * cosa;

    Vector3D coordDev( x, y, 0.0 );
    Vector3D coord   ( device.matrix * coordDev );
    Vector3D coordMap = obsMatrixInv * coord;

    int ox =   coordMap.x * obstaclePPM + ow/2;
    int oy =  -coordMap.y * obstaclePPM + oh/2;

    if ( ox >= 0 && ox < ow && oy >= 0 && oy < oh )
    { if ( oImg( ox, oy, 0, 0 ) > 0 )
      { distance = r;
	return true;
      }
    }	
  }

  return false;
}


static bool
setObstacles()
{
  if ( !g_UseObstacle || obstacleExtent.empty() )
    return false;

  int width, height;
  try {
    rpImg img( TrackGlobal::getConfigFileName(obstacleFileName.c_str()).c_str() );
    obstacleImg = img; 
    width  = img.width();
    height = img.height();
  } catch(CImgException& e) {
    TrackGlobal::error( "can't read obstacle image file \"%s\"", obstacleFileName.empty() ? "" : obstacleFileName.c_str() );
    return false;
  }

  if ( g_Verbose )
    TrackGlobal::info( "using obstacle image %s  (%dx%d)", obstacleFileName.c_str(), width, height );

  int   obstacleExtentPixels;

  std::vector<std::string> pair( split(obstacleExtent,'=',2) );
  if ( pair.size() == 2 )
  { obstacleExtentPixels = ::atoi( pair[0].c_str() );
    obstacleExtentX      = ::atof( pair[1].c_str() );
  }
  else
  { obstacleExtentPixels = width;
    obstacleExtentX      = ::atof(pair[0].c_str() );
  }
  
  obstaclePPM = obstacleExtentPixels / obstacleExtentX;
  
  return true;
}

/***************************************************************************
*** 
*** Spinning
***
****************************************************************************/

static int g_SpinningReportSec  = 5;

static bool
resolveSensorIN()
{
  std::string fileName( TrackGlobal::configFileName("[conf]/SensorIN.txt") );
  std::ifstream stream( fileName );

  if ( !stream.is_open() )
  { 
    fileName = "./SensorIN.txt";
    stream.open( fileName );
    if ( !stream.is_open() )
      return false;
  }

  g_SensorINFileName = fileName;

  return true;
}

    
static bool
readSensorIN()
{
  std::string fileName( g_SensorINFileName );
  std::ifstream stream( fileName );

  if ( !stream.is_open() )
      return false;

  if ( g_Verbose )
    TrackGlobal::info( "reading sensor INs from %s", fileName.c_str() );

  std::string content;
  stream >> content;
  
  while ( replace( content, "\r", "" ) )
    ;

  std::vector<std::string> lines( split(content,'\n') );

  int d = 0;
  
  for ( int i = 0; i < lines.size(); ++i )
  {
    std::string &line( lines[i] );
    trim( line );
   
    if ( !line.empty() )
      if ( g_Devices.size() > d )
      {
	LidarDevice &device( *g_Devices[i] );
	device.lock();
	device.sensorIN = line;

	if ( device.outDrv != NULL && device.isOpen(false) )
	  device.outDrv->deviceStatusSent = false;
	device.unlock();
	
	d += 1;
      }
  }

  return true;
}



static std::string
getSpinningDevices()
{
  std::string result( "[" );

  LidarDevices &devices( g_Devices );
  bool first = true;

  for ( int d = 0; d < devices.size(); ++d )
  {
    LidarDevice &device( *devices[d] );

    if ( first )
      first = true;
    else
      result += ", ";
      
    result += "{ \"name\": \"";
    result += device.getNikName();
    result += "\", \"id\": ";
    result += std::to_string( d );
    result += ", \"sensorIN\": \"";
    result += device.sensorIN;
    result += "\", \"spinning\": ";
    result += (device.isSpinning() ? "true":"false");
    result += " }";
  }
  
  result += "]";

  return result;
}

static void
reportSpinning()
{
  if ( g_SpinningReportScript.empty() )
    return;

  std::string cmd( g_SpinningReportScript );

  if ( !fileExists( cmd.c_str() ) && cmd[0] != '.' && cmd[0] != '/' )
    cmd = TrackGlobal::configDir + cmd;

  if ( cmd[0] != '.' && cmd[0] != '/' )
    cmd = "./" + cmd;

  cmd= std::string("verbose=") + std::string(g_Verbose?"true":"false") + " " + cmd;

  std::string msg( getSpinningDevices() );
  
  cmd += " '";
  cmd += msg;
  cmd += "' &";
	
  if ( g_Verbose > 0 )
    TrackGlobal::info( "running %s", cmd.c_str() );

  system( cmd.c_str() );
}


/***************************************************************************
*** 
*** Failure
***
****************************************************************************/

static int g_FailureReportSec  = 25;
static int g_WarningReportMSec = 1000;

static std::string
inVirtualUrl( LidarDevice &device )
{
  std::string empty;

  if ( device.inVirtUrl.empty() || device.inFile != NULL )
    return empty;
  
  LidarUrl url( device.inVirtUrl.c_str() );
  if ( !url.isOk() )
    return empty;
  
  std::string restURL = "http://";
  if ( !url.hostname.empty() )
    restURL += url.hostname;
  else
  {
    if ( device.getVirtualHostName().empty() )
      return empty;

    restURL += device.getVirtualHostName();
  }

  restURL += ":" + std::to_string(remote_port);
  
  return restURL;
}

static void
reportFailure( LidarDevice &device, std::string &reason )
{
  if ( reason == "ok" )
  { TrackGlobal::error( "Device '%s' ok", device.getNikName().c_str() );
    log( "DEVICE '%s' ok", device.getNikName().c_str() );
  }
  else
  { TrackGlobal::error( "Failure on Device '%s' Reason: %s", device.getNikName().c_str(), reason.c_str() );
    log( "DEVICE Failure on device '%s' Reason: %s", device.getNikName().c_str(), reason.c_str() );
  }

  std::string conf( g_Config );
  rtrim(conf, "/");

  std::string url( inVirtualUrl(device) );
  if ( url.empty() )
  {
    std::string ip( getIP() );
    trim( ip );
    
    url = "http://";
    url += ip;
    url += ":" + std::to_string(webserver_port);
  }

  char msg[4096];
  sprintf( msg, "deviceName=%s sensorIN=\"%s\" reason=\"%s\" conf=%s runMode=%s url=\"%s\" verbose=%s", device.getNikName().c_str(), device.sensorIN.c_str(), reason.c_str(), conf.c_str(), g_RunningMode.c_str(), url.c_str(), g_Verbose?"true":"false" );

  TrackGlobal::notification( "device", msg );

  if ( g_FailureReportScript.empty() )
    return;

  std::string cmd( g_FailureReportScript );
  if ( !fileExists( cmd.c_str() ) && cmd[0] != '.' && cmd[0] != '/' )
    cmd = TrackGlobal::configDir + cmd;

  if ( cmd[0] != '.' && cmd[0] != '/' )
    cmd = "./" + cmd;
	  
  cmd += " ";
  cmd += " &";
	
  cmd = std::string(msg) + " " + cmd;

  if ( g_Verbose > 0 )
    TrackGlobal::info( "running %s", cmd.c_str() );

  system( cmd.c_str() );
}


static std::string g_AvailableDevices;

static std::set<std::string>
getAvailableDevices()
{
  LidarDeviceList &devices( g_Devices.activeDevices() );

  std::set<std::string> availableDevices;

  for ( int d = ((int)devices.size())-1; d >= 0; --d )
  {
    LidarDevice &device( *devices[d] );

    if ( device.isOpen() && device.isReady() && device.dataReceived )
      availableDevices.emplace( device.getNikName() );
  }

  return availableDevices;
}


static void
updateFailures()
{
  uint64_t now = getmsec();
  for ( int d = ((int)g_Devices.size())-1; d >= 0; --d )
  {
    LidarDevice &device( *g_Devices[d] );
    uint64_t timeDiff = now - device.openTime;

    if ( device.reopenTime == 0 && timeDiff/1000 > g_FailureReportSec )
    {
      bool failure = false;
    
      if ( device.isOpen() )
      { if ( device.isPoweringUp || !device.dataReceived || !device.isReady() )
	  failure = true;
      }
      else if ( !device.errorMsg.empty() )
	failure = true;
      
      std::string nikName( device.getNikName() );

      bool value = false;
      g_DeviceFailed.get( nikName.c_str(), value );

      if ( failure != value )
      {
	g_DeviceFailed.set( nikName.c_str(), failure ? "true": "false" );

	std::string reason( "ok" );
	  
	if ( failure )
	{
	  reason = "reason unknown";
	  
	  if ( device.isOpen() )
          {
	    if ( device.isPoweringUp )
	      reason = "still powering up";
	    else if ( !device.dataReceived )
	      reason = "no data";
	    else if ( !device.isReady() )
	      reason = "not ready";
	  }
	  else
	  {
	    if ( !device.errorMsg.empty() )
	      reason = device.errorMsg;
	  }
	}
	
	reportFailure( device, reason );
      }
    }
  }
}

static void
stopFailures()
{
  for ( int d = ((int)g_Devices.size())-1; d >= 0; --d )
  {
    LidarDevice &device( *g_Devices[d] );

    bool failure = false;
  
    std::string nikName( device.getNikName() );

    g_DeviceFailed.set( nikName.c_str(), "false" );
  }
}

/***************************************************************************
*** 
*** Player
***
****************************************************************************/

static int64_t
playerCurrentTime()
{
  if ( TrackBase::packedPlayer() != NULL )
    return TrackBase::packedPlayerCurrentTime();

  return LidarDevice::fileDriverCurrentTime();
}

static uint64_t
playerTimeStamp()
{
  if ( TrackBase::packedPlayer() != NULL )
    return TrackBase::packedPlayerTimeStamp();

  return LidarDevice::fileDriverTimeStamp();
}

static float
playerPlayPos()
{ 
  if ( TrackBase::packedPlayer() != NULL )
    return TrackBase::packedPlayerPlayPos();
  
  return LidarDevice::fileDriverPlayPos();
}

static void
setPlayerPlayPos( float playPos )
{
  if ( TrackBase::packedPlayer() != NULL )
    TrackBase::setPackedPlayerPlayPos( playPos );
  else
    LidarDevice::setFileDriverPlayPos( playPos);
}

static void
setPlayerSyncTime( uint64_t timestamp=0 )
{
  if ( TrackBase::packedPlayer() != NULL )
    TrackBase::setPackedPlayerSyncTime( timestamp );
  else
    LidarDevice::setFileDriverSyncTime( timestamp );
}

static void
setPlayerPaused( bool paused )
{
  if ( TrackBase::packedPlayer() != NULL )
    return TrackBase::setPackedPlayerPaused( paused );
  else
    return LidarDevice::setFileDriverPaused( paused);
}

static bool
playerIsPaused()
{ 
  if ( TrackBase::packedPlayer() != NULL )
    return TrackBase::packedPlayerIsPaused();

  return LidarDevice::fileDriverIsPaused();
}

static bool
playerAtEnd()
{
  if ( TrackBase::packedPlayer() != NULL )
    return TrackBase::packedPlayerAtEnd();
  
  return LidarDevice::fileDriverAtEnd();
}

static void exitHook()
{
  if ( g_IsStarted && g_DoTrack )
  {
    log( "STOP on Exit Application" );
    TrackBase::notification( "stop", "message=\"Stop on application exit\" runMode=%s verbose=%s", g_RunningMode.c_str(), g_Verbose?"true":"false" );
    g_Track.stop( playerTimeStamp() );
    g_IsStarted = false;
  }

  log( "RUN Exit Application" );
  TrackBase::notification( "run", "message=\"Exit Application\" runMode=%s verbose=%s", g_RunningMode.c_str(), g_Verbose?"true":"false" );

  g_Track.finishObserver();
}

/***************************************************************************
***
*** TrackableHUB
***
****************************************************************************/

#if USE_WEBSOCKETS

void
TrackableHUB::observe( PackedTrackable::Header &header )
{
  if ( header.isType( PackedTrackable::StartHeader ) )
    g_HUBStarted = true;
  else if ( header.isType( PackedTrackable::StopHeader ) )
    g_HUBStarted = false;
  
  g_Track.observe( header );
}


void
TrackableHUB::observe( PackedTrackable::BinaryFrame &frame )
{
  if ( !g_HUBStarted )
  { PackedTrackable::Header header( frame.header.timestamp, PackedTrackable::StartHeader );
    observe( header );
    g_HUBStarted = true;
  }
  
  g_Track.observe( frame );
}

#endif

/***************************************************************************
*** 
*** 
***
****************************************************************************/

static std::string
withRunningMode( const char *message )
{
  if ( g_RunningMode.empty() || g_RunningMode == "unknown" )
    return message;
  
  std::string msg( message );
  
  msg += " (";
  msg += g_RunningMode;
  msg += ")";

  return msg;
}


/***************************************************************************
*** 
*** LidarPainter
***
****************************************************************************/

static const unsigned char red[] = { 255,0,0,255 }, darkRed[] = { 160,0,0,255 }, green[] = { 0,255,0,255 }, darkGreen[] = { 0,160,0,255 }, grayGreen[] = { 70,150,70,255 }, blue[] = { 0,0,255,255 }, lightBlue[] = { 96,96,255,255 }, violet[] = { 128,0,255,255 }, yellow[] = { 255,255,0,255 }, darkerYellow[] = { 255,255,0,255 }, black[] = { 0,0,0,255 }, white[] = { 255,255,255,255 }, darkerGray[] = { 50,50,50,255 }, darkGray[] = { 72,72,72,255 }, midGray[] = { 128,128,128,255 }, lightGray[] = { 192,192,192,255 };
  

static void
scaleColor( unsigned char *color, float gray )
{
  color[0] *= gray;
  color[1] *= gray;
  color[2] *= gray;
  color[3]  = 255;
    
  const float minIntensity = 0.5;
  float intensity = (0.299*color[0] + 0.587*color[1] + 0.114*color[2]);
    
  if ( intensity > 0 && intensity < minIntensity )
  {
    float alpha = minIntensity / intensity;
    color[0] *= alpha;
    color[1] *= alpha;
    color[2] *= alpha;
  }
}

static void
deviceColor( int deviceId, unsigned char *color )
{
  float r = 255 * (1.0-((deviceId>>0)&1));
  float g = 255 * (1.0-((deviceId>>1)&1));
  float b = 255 * (1.0-((deviceId>>2)&1));

  color[0] = (r < 0 ? 0 : r);
  color[1] = (g < 0 ? 0 : g);
  color[2] = (b < 0 ? 0 : b);

  const int channelMin = 148;
  if ( color[0] < channelMin ) color[0] = channelMin;
  if ( color[1] < channelMin ) color[1] = channelMin;
  if ( color[2] < channelMin ) color[2] = channelMin;

  if ( colChannels > 3 )
    color[3] = 255;
}

static void
objectColor( int objectId, unsigned char *color )
{
  float r = 255 * (((objectId>>0)&1)+0.5);
  float g = 255 * (((objectId>>1)&1)+0.5);
  float b = 255 * (((objectId>>2)&1)+0.5);

  color[0] = (r > 255 ? 255 : r);
  color[1] = (g > 255 ? 255 : g);
  color[2] = (b > 255 ? 255 : b);

  if ( colChannels > 3 )
    color[3] = 255;
}

LidarPainter::LidarPainter()
  : matrix(),
    extent         ( 10 ),
    width	   ( 500 ),
    height         ( 500 ),
    canv_width     ( 500 ),
    canv_height    ( 500 ),
    sampleRadius   ( 1 ),
    objectRadius   ( 3 ),
    showGrid       ( true ),
    showPoints     ( true ),
    showLines      ( false ),
    showObjects    ( true ),
    showObjCircle  ( true ),
    showConfidence ( false ),
    showCurvature  ( false ),
    showLifeSpan   ( false ),
    showSplitProb  ( false ),
    showMotion     ( false ),
    showMotionPred ( false ),
    showMarker     ( false ),
    showDevices    ( false ),
    showDeviceInfo ( true ),
    showObserverStatus( true ),
    showTracking   ( true ),
    showRegions    ( true ),
    showStages     ( false ),
    showEnv        ( false ),
    showEnvThres   ( true ),
    showCoverage   ( false ),
    showOutline    ( false ),
    showPrivate    ( true ),
    showControls   ( true ),
    viewUpdated    ( true ),
    layers	   ( TrackGlobal::regions.layers ),
    img            ( NULL ),
    uiImageFileName( "uiImage.jpg" )
{
  if ( layers.size() > 0 )
    layers.emplace("");
}

LidarPainter::~LidarPainter()
{
  if ( img != NULL )
    delete img;
  
  std::remove( uiImageFileName.c_str() );
}


static void
cleanupPainter()
{
  uint64_t now = getmsec();
  
  for ( std::map<std::string,LidarPainter>::iterator iter( painters.begin() ); iter != painters.end(); iter++ )
  { 
    LidarPainter &painter( iter->second );

    if ( now - painter.lastAccess > 60 * 60 * 1000 ) // remove after 1h
    { 
      painters.erase( iter->first );
      iter = painters.begin();
      
      if ( iter == painters.end() )
	break;
    }
  }
}


void
LidarPainter::setUIImageFileName( const char *type, const char *key )
{
  uiImageFileName = "";
#if __LINUX__
  uiImageFileName += "/tmp/";
#endif
  uiImageFileName += "uiImage_";

  uiImageFileName += key;
  uiImageFileName += ".";
  uiImageFileName += type;
}


void
LidarPainter::updateExtent()
{
  extent_x = extent;
  extent_y = extent * height / (double) width;
}

void
LidarPainter::begin()
{
  if ( img != NULL )
    delete img;
  
  img = new rpImg( width, height, 1, colChannels, 0x0);

  updateExtent();  
}

void
LidarPainter::end()
{
}

inline void
LidarPainter::getCoord( float &sx, float &sy, int x, int y )
{
  Vector3D p;

  p.x =  (x - width /2) * extent_x / width;
  p.y =  (y - height/2) * extent_y / height;

  p = matrixInv * p;

  sx = p.x;
  sy = p.y;
}

inline void
LidarPainter::getCoord( int &x, int &y, float sx, float sy )
{
  Vector3D p( matrix * Vector3D(sx,sy,0.0) );
  
  x =  (p.x / extent_x) * width  + width/2;
  y = -(p.y / extent_y) * height + height/2;
}

inline void
LidarPainter::getCanvCoord( int &x, int &y, float sx, float sy )
{
  Vector3D p( matrix * Vector3D(sx,sy,0.0) );
  
  x =  (p.x / extent_x) * canv_width  + canv_width/2;
  y = -(p.y / extent_y) * canv_height + canv_height/2;
}

static const unsigned char colorArray[5][4] = {
  { 255, 255, 255, 255 },
  { 255, 255, 0, 255 },
  { 255, 0, 255, 255 },
  { 0, 255, 255, 255 },
  { 0, 255, 0, 255 }
};


void
LidarPainter::paintBlobMarkerUnion( pv::Trackable<pv::BlobMarkerUnion> &object, int colorIndex, bool showLabel, bool drawMotion, uint64_t timestamp, bool drawConfidence, bool drawCircle )
{
  bool isPrivate = object.isPrivate();
  
  if ( isPrivate && !showPrivate )
    return;
  
  unsigned char c[4] = { 255, 255, 255, 255 };
  const unsigned char *color = c;
  if ( colorIndex >= 0 )
  {
    deviceColor( colorIndex, c );
//    color = colorArray[colorIndex];
  }
  else if ( !object.isActivated )
    color = yellow;
  else if ( timestamp != 0 && object.lastTime != timestamp )
    color = red;
  else if ( isPrivate )
    color = lightBlue;
  else
    color = green;

  int x, y;

  getCoord( x, y, object.p[0], object.p[1] ); 

  if ( drawCircle )
  { int radius = object.size / extent_x * 0.5 * width;
    img->draw_circle ( x, y, radius, color, 1, 0xffffffff );
    if ( !showControls )
    { img->draw_circle ( x, y, radius+1, color, 1, 0xffffffff );  
      img->draw_circle ( x, y, radius+2, color, 1, 0xffffffff );
    }
  }
  
  if ( showLabel )
  {
    char label[100];
    sprintf( label, "tid:%s", object.id().c_str() );

    img->draw_text( x-(int)(strlen(label)*3), y-4, label, color, colorIndex >= 0 ? 0 : black );
  }


  if ( showLifeSpan )
  {
    int time = (int)(object.lastTime - object.firstTime) / 1000;

    char lifeSpan[100];
    sprintf( lifeSpan, "alive:%02d:%02d", time/60, time%60 );

    img->draw_text( x-(int)(strlen(lifeSpan)*3), y+10, lifeSpan, color, colorIndex >= 0 ? 0 : black );
  }

  if ( showSplitProb )
  {
    char splitProb[100];
    sprintf( splitProb, "split:%1.3g", object.splitProb );

    img->draw_text( x-(int)(strlen(splitProb)*3), y-18, splitProb, color, colorIndex >= 0 ? 0 : black );
  }

  if ( drawConfidence )
  {
    char label[100];
    
    sprintf( label, "extent:%1.3g", object.user5 );
    int len = (int)(strlen(label)*3);
    img->draw_text( x-len, y-56, label, color, colorIndex >= 0 ? 0 : black );

    sprintf( label, "curv: %1.3g", object.user3 );
    img->draw_text( x-len, y-44, label, color, colorIndex >= 0 ? 0 : black );

    sprintf( label, "pers: %1.3g", object.user4 );
    img->draw_text( x-len, y-32, label, color, colorIndex >= 0 ? 0 : black );

    sprintf( label, "conf: %1.3g", object.confidence );
    img->draw_text( x-len, y-20, label, color, colorIndex >= 0 ? 0 : black );
  }

  if ( drawMotion && showMotion )
  {
    int x1, y1;
    float weight = 0.5 * g_Track.m_Stage->trackMotionPredict;
    getCoord( x1, y1, object.p[0] + weight * object.motionVector[0], object.p[1] + weight * object.motionVector[1] );
    img->draw_line( x, y, x1, y1, yellow  );
  }
}


void 
LidarPainter::paintStage( pv::TrackableStage<pv::BlobMarkerUnion> &stage, int colorIndex, bool showLabel, bool drawMotion, uint64_t timestamp )
{
  for ( int i = 0; i < stage.size(); ++i )
  {
    paintBlobMarkerUnion( *stage[i], colorIndex, showLabel, drawMotion, timestamp );

    if ( showMotionPred )
    { pv::Trackable<pv::BlobMarkerUnion> &object( *stage[i] );
      int x, y;
      getCoord( x, y, object.predictedPos[0], object.predictedPos[1] );
      static const unsigned char color[4] = { 64, 64, 255, 255 };
      img->draw_circle ( x, y, object.size / extent_x * 0.5 * width, color, 1, 0xffffffff );
    }
  }
  
  stage.lockCurrent();
  
  pv::Trackables<pv::BlobMarkerUnion> &current( *stage.current );

  for ( int i = 0; i < stage.current->size(); ++i )
  { pv::Trackable<pv::BlobMarkerUnion> &object( *current[i] );
    if ( !object.isActivated )
      paintBlobMarkerUnion( object, colorIndex, false, drawMotion, timestamp );
  }

  stage.unlockCurrent();
}

void 
LidarPainter::paintMultiStage( pv::TrackableMultiStage<pv::BlobMarkerUnion> &stage, bool showTracking, bool substages, int colorIndex, bool drawMotion )
{
  if ( showTracking )
    paintStage( stage, -1, true, drawMotion, stage.lastTime );
  
/* hack, because substages are abused by tracking
  stage.lockCurrent();
  
  if ( substages )
    for ( int i = 0; i < stage.subStages.size(); ++i )
      paintStage( *stage.subStages[i], colorIndex-1-i, false );

  stage.unlockCurrent();
*/

  if ( substages && stage.subStages.size() > 0 )
  { pv::Trackables<pv::BlobMarkerUnion> &latest( *stage.subStages[0]->latest );
    for ( int i = 0; i < latest.size(); ++i )
      paintBlobMarkerUnion( *latest[i], latest[i]->user2, false, false );
  }
}

void
LidarPainter::paintObstacles()
{
  if ( obstacleImg.width() == 0 )
    return;
  
  int x, y, rx, ry;
  getCoord(  x,  y, obsMatrix.w.x, obsMatrix.w.y );
  getCoord( rx, ry, obsMatrix.w.x+obstacleExtentX, obsMatrix.w.y );

  const unsigned char color[4] = { 255, 128, 128, 250 };

  img->draw_circle( x, y, (rx-x)/2, color, 1, 0x00ff00ff );
}

void
LidarPainter::paintGrid()
{
  char label[100];

  int x, y;
  int x1, y1;
  int x0, y0;
  int steps = 1;
  
  float sx0, sy0, sx1, sy1;
  const int border = 20;
  
  getCoord( x0, y0, 0.0f, 0.0f );
  getCoord( x1, y1, 1.0f, 1.0f );
  float diff = x1 - x0;

  bool drawHalf       = (diff > 120);
  bool drawHalfFrame  = (diff > 180);
  bool drawHalfLine   = (diff > 80);
  bool dashSecondLine = (diff < 56);
  bool dashTwoLine    = (diff < 32);
  if ( diff < 10 )
  { steps = 10;
    dashSecondLine = false;
    dashTwoLine    = false;
  }
  else if ( diff < 20 )
  { steps = 5;
    dashSecondLine = false;
    dashTwoLine    = false;
  }
  else if ( diff < 40 )
    steps = 2;

  getCoord( sx0, sy0, 0, 0 );
  getCoord( sx1, sy1, width, height );

  for ( float sx = floor(sx0); sx < sx1; sx += 0.5f )
  {
    int sxi = (int)round(sx);

    getCoord( x0, y0, sx, sy0 );
    getCoord( x1, y1, sx, sy1 );

    bool full = true;
    if ( sx-floor(sx) > 0.1 || (steps == 5 && sxi%5) || (steps == 10 && sxi%10) )
      full = false;

    if ( sxi == 0 )
    {
      getCoord( x,  y, 0.0f, 0.0f );
      img->draw_line( x0, y0,  x,  y, darkRed  );
      img->draw_line( x,  y,  x1, y1, red );
    }
    else if ( full || drawHalfLine )
    { bool filled = ((full && !dashTwoLine && !(dashSecondLine&&sxi%2)) || sxi%5 == 0);
      img->draw_line( x0, y0, x1, y1, (full ? darkGray : darkerGray), 1, filled ? -1 : 0xf9f9f9f9 );
      if ( sxi%5 == 0 && steps < 5 )
	img->draw_line( x0+1, y0, x1+1, y1, (full ? darkGray : darkerGray), 1, -1 );
      if ( sxi%10 == 0 && steps < 10 )
	img->draw_line( x0-1, y0, x1-1, y1, (full ? darkGray : darkerGray), 1, -1 );
    }
    
    if ( full || drawHalf )
    {
      getCoord( x,  y,  sx, sx );
      sprintf( label, "%gm", sx );
	
      if ( (full || drawHalfFrame) && ((int)round(sx))%steps == 0 )
      {
	img->draw_text( x+4, 0, label, (sx<0 ? darkGreen:green), 0, 1, 14 );
	img->draw_text( x+4, height-14, label, (sx<0 ? darkGreen:green), 0, 1, 14 );
      }
      if ( round(sx) != 0 && (x > border && width-x-32 > border && y > border && height-y-16 > border) )
	img->draw_text( x+4, y+4, label, (sx<0 ? midGray:lightGray), 0, 1, 16 );
    }
  }

  for ( float sy = floor(sy0); sy < sy1; sy += 0.5f )
  {
    int syi = (int)round(sy);

    getCoord( x0, y0, sx0, sy );
    getCoord( x1, y1, sx1, sy );

    bool full = true;
    if ( sy-floor(sy) > 0.1 || (steps == 5 && syi%5) || (steps == 10 && syi%10) )
      full = false;
      
    if ( syi == 0 )
    {
      getCoord( x,  y, 0.0f, 0.0f );
      img->draw_line( x0, y0,  x,  y, darkGreen  );
      img->draw_line( x,  y,  x1, y1, green );
    }
    else if ( full || drawHalfLine )
    { bool filled = ((full && !dashTwoLine && !(dashSecondLine&&syi%2)) || syi%5 == 0);
      img->draw_line( x0, y0, x1, y1, (full ? darkGray : darkerGray), 1, filled ? -1 : 0xf9f9f9f9 );
      if ( syi%5 == 0 && steps < 5 )
	img->draw_line( x0, y0+1, x1, y1+1, (full ? darkGray : darkerGray), 1, -1 );
      if ( syi%10 == 0 && steps < 10 )
	img->draw_line( x0, y0-1, x1, y1-1, (full ? darkGray : darkerGray), 1, -1 );
    }

    if ( full || drawHalf )
    {
      getCoord( x,  y,  sy, sy );
      sprintf( label, "%gm", sy );
	
      img->draw_text( 1, y+4, label, (sy<0 ? darkRed:red), 0, 1, 14 );
      img->draw_text( width-26-6*(sy<0), y+4, label, (sy<0 ? darkRed:red), 0, 1, 14 );
    }
  }
}

void
LidarPainter::paintAxis()
{
  int x, y;

  const int axisLength  = 6;
  
  img->draw_line( width/2-axisLength, height/2, width/2+axisLength, height/2, violet );
  img->draw_line( width/2, height/2-axisLength, width/2, height/2+axisLength, red );
  
  getCoord( x, y, 0.0, 0.0 );    
  img->draw_line( x-axisLength, y, x+axisLength, y, green  );
  img->draw_line( x, y-axisLength, x, y+axisLength, yellow );
}


static const char *
warning( LidarDevice &device )
{
  if ( device.isOpen() )
  {
    if ( device.isPoweringUp )
      return "powering up";
    else if ( !device.dataReceived )
      return "no data";
    else if ( !device.isReady() )
      return "not ready";
  }
  else
  {
    if ( !device.errorMsg.empty() )
      return device.errorMsg.c_str();
  }

  return "";
}

static const unsigned char *
warningColor( LidarDevice &device, const unsigned char *defaultColor )
{
  bool failed = false;
  g_DeviceFailed.get( device.getNikName().c_str(), failed );

  if ( failed )
    return red;

  if ( device.isOpen() )
  {
    if ( device.isPoweringUp )
      return darkerYellow;
    else if ( !device.dataReceived )
      return red;
    else if ( !device.isReady() )
      return red;
  }
  else
  {
    if ( !device.errorMsg.empty() )
      return red;
  }

  return defaultColor;
}


void
LidarPainter::paintDevice( LidarDevice &device )
{
  char label[200];
  int x, y;
	
  unsigned char devColor[4];
  deviceColor( device.deviceId, devColor );

  const char *msg = warning( device );
  
  bool warning = (msg[0] != '\0');
  
  const unsigned char *color = warningColor( device, white );

  getCoord( x, y, device.matrix.w.x, device.matrix.w.y );

  img->draw_circle( x, y, 5, devColor );

  img->draw_circle( x, y, 6, color, 1, 0xffffffff );
  img->draw_circle( x, y, 7, color, 1, 0xffffffff );

  int textX = x-(int)(strlen(label)*3.5);
  int textY = y + 10;
  
  const unsigned char *backColor = black;

  if ( warning )
    backColor = red;

  sprintf( label, "%s %s", device.getNikName().c_str(), msg );
  img->draw_text( textX, textY, label, devColor, backColor, 1, 16 );
}


void
LidarPainter::paintMarker( LidarDevice &device )
{
  LidarObjects::Marker marker( device.objects.getMarker( device.sampleBuffer() ) );

  unsigned char devColor[4];
  deviceColor( device.deviceId, devColor );

  for ( int m = 0; m < marker.size(); ++m )
  {
    LidarObject &o0( marker[m][0] );
    LidarObject &o1( marker[m][1] );

    char label[100];
    int x0, y0, x1, y1, x2, y2, x3, y3;
	
    getCoord( x0, y0, o0.lowerCoord.x,  o0.lowerCoord.y  );
    getCoord( x1, y1, o0.higherCoord.x, o0.higherCoord.y );
	
    getCoord( x2, y2, o1.lowerCoord.x,  o1.lowerCoord.y  );
    getCoord( x3, y3, o1.higherCoord.x, o1.higherCoord.y );

    img->draw_line( x0, y0, x1, y1, yellow  );
    img->draw_line( x2, y2, x3, y3, yellow  );

    Vector3D center;
  
    center += o0.center;
    center += o1.center;
  
    center /= 2;

    getCoord( x0, y0, center.x, center.y );

    float distance0 = o0.higherCoord.distance(o1.higherCoord);
    float distance1 = o0.lowerCoord.distance(o1.higherCoord);
    float distance2 = o0.higherCoord.distance(o1.lowerCoord);
    float distance3 = o0.lowerCoord.distance(o1.lowerCoord);
    
    float distance = (distance1 > distance0 ? distance1 : distance0);
    distance = (distance2 > distance ? distance2 : distance);
    distance = (distance3 > distance ? distance3 : distance);
    
    float radius = distance / extent_x * 0.5 * width;
	
    img->draw_circle( x0, y0, radius, devColor, 1, 0xffffffff );

    distance = o0.center.distance( o1.center );

    sprintf( label, "dist=%g", distance );
    img->draw_text( x0-radius*0.5, y0, label, devColor, 0, 1, 16 );
  }
}

void
LidarPainter::paintEnv( LidarDevice &device )
{
  bool isEnvScanning = device.isEnvScanning;
    
  if ( device.envValid && (isEnvScanning || (showEnv&&device.useEnv)) )
  {
    int x, y, x1, y1;
    unsigned char color[4];
    float gray = (isEnvScanning ? 0.5 : 0.3);

    deviceColor( device.deviceId, color );

    scaleColor( color, gray );

    float envThreshold = device.envThreshold;
    Matrix3H &matrix( device.matrix );

////// wo kommen die 80 her ????????

    float maxRange = 80;

    for ( int i = device.envSamples.size()-1; i >= 0; --i )
    { LidarSample &sample( device.envSamples[i] );
      if ( sample.quality > 0 && sample.distance > 0 && sample.distance < maxRange )
      { getCoord( x, y, sample.coord.x, sample.coord.y );    
	img->draw_circle( x, y, 1, color );

	if ( showEnvThres && !isEnvScanning )
        {
	  float distance = sample.distance - envThreshold;
	  if ( distance > 0 )
          {
	    Vector3D coord(distance * sin( sample.angle ), distance * cos( sample.angle ), 0 );
	    coord = matrix * coord;

	    getCoord( x1, y1, coord.x, coord.y );

	    img->draw_line( x, y, x1, y1, color );
	  }
	}
      }
    }
  }
}

void
LidarPainter::paintCoverage( LidarDevice &device )
{
  bool isEnvScanning = device.isEnvScanning;
    
  if ( !(showCoverage || showCoveragePoints) || !device.dataValid || isEnvScanning )
    return;

  float sx, sy;
  int x, y, x1, y1;

  unsigned char devColor[4];
  deviceColor( device.deviceId, devColor );

  getCoord( x1, y1, device.matrix.w.x, device.matrix.w.y );

  LidarSampleBuffer &sampleBuffer( device.sampleBuffer() );

  for ( int i = sampleBuffer.size()-1; i >= 0; --i )
  { 
    LidarSample &sample( sampleBuffer[i] );
    
    getCoord( x, y, sample.coord.x, sample.coord.y );    

    if ( showCoverage )
      img->draw_line( x, y, x1, y1, devColor, 0.4 );

    if ( showCoveragePoints )
      img->draw_circle( x, y, objectRadius/2, devColor );
  }
}

void
LidarPainter::paint( LidarDevice &device )
{
  char label[100];

  bool isEnvScanning = device.isEnvScanning;

  float sx, sy;
  int x, y, x1, y1;

  bool lock = !g_Devices.isCalculating;
  if ( lock )
    device.lock();

  if ( device.dataValid )
  {
    unsigned char devColor[4];
    unsigned char objColor[4];
    
    deviceColor( device.deviceId, devColor );

    if ( showObjects )
    { 
      int last_oid = -1;
      for ( int i = device.sampleBuffer().size()-1; i >= 0; --i )
      { int objectId = device.getObjectId( i );
	if ( objectId != 0 )
	{ if ( device.getCoord( i, sx, sy ) )
          { getCoord( x, y, sx, sy );    

	    if ( objectId != last_oid )
	    { objectColor( objectId, objColor );
	      last_oid = objectId;
	    }
	   
	    img->draw_circle( x, y, objectRadius,   devColor );
	    img->draw_circle( x, y, objectRadius/2, objColor );
	  }
	}
      }

      for ( int i = device.numDetectedObjects()-1; i >= 0; --i )
      { 
	LidarObject &object( device.detectedObject( i ) );
	  
	getCoord( x, y, object.lowerCoord[0], object.lowerCoord[1] );    
	img->draw_circle( x, y, objectRadius*1.5, objColor );
	
	getCoord( x, y, object.higherCoord[0], object.higherCoord[1] );    
	img->draw_circle( x, y, objectRadius*1.5, objColor );

	if ( showCurvature )
        {
	  for ( int c = 0; c < object.curvePoints.size(); ++c )
          {
	    Vector2D p( object.curvePoints[c] );
	    
//	    p.print( "p " );
	    
	    getCoord( x, y, p.x, p.y );    
	    img->draw_circle( x, y, objectRadius*1.5, objColor );

	    if ( c > 0 )
	      img->draw_line( x, y, x1, y1, devColor );
	    x1 = x;
	    y1 = y;
	  }
	}
      }

      if ( !showTracking )
	for ( int i = device.numDetectedObjects()-1; i >= 0; --i )
        { 
	  LidarObject &object( device.detectedObject( i ) );
	  
	  getCoord( x, y, object.lowerCoord[0], object.lowerCoord[1] );    

	  sprintf( label, "oid:%02d", object.oid );

	  img->draw_text( x, y, label, objColor, 1, 32 );
	}

      if ( (showObjCircle || showConfidence) && g_Track.m_Stage != NULL && g_Track.m_Stage->subStages.size() > 0 )
      { 
	if ( lock )
	  device.unlock();
	g_TrackMutex.lock();
//	g_Track.m_Stage->lockCurrent();
	pv::Trackables<pv::BlobMarkerUnion> &current( *g_Track.m_Stage->subStages[0]->current );
	for ( int i = 0; i < current.size(); ++i )
	  paintBlobMarkerUnion( *current[i], current[i]->user2, false, false, 0, showConfidence, showObjCircle );
//	g_Track.m_Stage->unlockCurrent();
	g_TrackMutex.unlock();
	if ( lock )
	  device.lock();
      }
    }

    if ( showLines && !isEnvScanning )
    {
      const float darken = (showControls?1.0:0.8);
      
      unsigned char lineColor[4] = { (unsigned char)(darken*devColor[0]), (unsigned char)(darken*devColor[1]), (unsigned char)(darken*devColor[2]), devColor[3] };

      getCoord( x1, y1, device.matrix.w.x, device.matrix.w.y );
      for ( int i = device.sampleBuffer().size()-1; i >= 0; --i )
      { if ( device.getCoord( i, sx, sy ) )
        { getCoord( x, y, sx, sy );    
	  img->draw_line( x, y, x1, y1, lineColor );
	}
      }
    }

    if ( showOutline && !isEnvScanning )
    {
      float sx, sy;
      bool valid = false;
  
      const float darken = (showControls?1.0:0.8);
      
      unsigned char outlineColor[4] = { (unsigned char)(darken*devColor[0]), (unsigned char)(darken*devColor[1]), (unsigned char)(darken*devColor[2]), devColor[3] };

      for ( int i = device.sampleBuffer().size()-1; i >= 0; --i )
      { 
	if ( device.getCoord( i, sx, sy ) )
        {
	  getCoord( x, y, sx, sy );    
	  
	  if ( valid )
	    img->draw_line( x, y, x1, y1, outlineColor );
	  else
	    valid = true;
	
	  x1 = x;
	  y1 = y;
	}  
      }
    }
    
    if ( showPoints )
    { 
      unsigned char color[4];
      color[0] = g_Color[device.deviceId][0];
      color[1] = g_Color[device.deviceId][1];
      color[2] = g_Color[device.deviceId][2];
      color[3] = g_Color[device.deviceId][3];
      scaleColor( color, 1.0 );
    
      for ( int i = device.sampleBuffer().size()-1; i >= 0; --i )
      { if ( device.getCoord( i, sx, sy ) )
        { getCoord( x, y, sx, sy );    
	  img->draw_circle( x, y, sampleRadius, color );
	}
      }
    }

    if ( showMarker )
      paintMarker( device );
  }
  
  if ( lock )
    device.unlock();
}

static const unsigned char g_RegionsColor[][4] =
{
  { 255, 255, 255, 255 },
  { 255, 255, 255, 255 },
  { 255, 255,   0, 255 },
  {  64, 255,  64, 255 },
  {  64, 255,  64, 255 },
  {  64, 255,  64, 255 },
  {  64, 255,  64, 255 },
  { 128, 128, 255, 255 }
};


const unsigned char * 
rect_color( TrackableRegion &region )
{
  return g_RegionsColor[0];
}

void
LidarPainter::paint( TrackableRegion &region )
{
  char label[100];

  int x1, y1, x2, y2;
  getCoord( x1, y1, region.x1(), region.y1() );    
  getCoord( x2, y2, region.x2(), region.y2() );

  const unsigned char *color = rect_color( region );

  if ( region.shape == RegionShapeEllipse )
    img->draw_ellipse( (x1+x2)/2, (y2+y1)/2, (x2-x1)/2, (y1-y2)/2, 0.0, color, 1.0f, 0xf1f1f1f1 );
  else
    img->draw_rectangle( x1, y1, x2, y2, color, 1.0f, 0xf1f1f1f1 );

  sprintf( label, "%s", region.name.c_str() );
  img->draw_text( x1+4, y2, label, color, 0, 1, 14 );

  if ( region.usedByObserver.empty() )
    return;
  
  sprintf( label, "%s", region.usedByObserver.c_str() );
  img->draw_text( x1+4, y1-12, label, color, 0, 1, 12 );
}

void
LidarPainter::paint( TrackableRegions &regions )
{
  if ( TrackGlobal::regions.layers.size() == 0 )
  { for ( int i = 0; i < regions.size(); ++i )
      paint( regions[i] );
  }
  else
  {
    for ( int i = 0; i < regions.size(); ++i )
    { for ( auto &layer: layers )
      { if ( regions[i].hasLayer( layer ) )
        { paint( regions[i] );
	  break;
	}
      }
    }
  }
}


/***************************************************************************
*** 
*** Image
***
****************************************************************************/

class ImageCache
{
public:
  rgbImg	img;
  int 		dx0, dy0; // drawing coordinates
  uint64_t 	timestamp;
  
  ImageCache()
    : img(),
      dx0( 0 ),
      dy0( 0 ),
      timestamp( 0 )
    {}
  
};

static std::map<std::string,ImageCache>  g_ImageCache;

static void
paintImageObserver( LidarPainter &painter, TrackableImageObserver *imageObserver )
{
  uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  
  std::string imgName(imageObserver->name);

  std::map<std::string,ImageCache>::iterator iter( g_ImageCache.find(imgName) );
  ImageCache *cache;

  if ( iter == g_ImageCache.end() )
  { g_ImageCache.emplace(std::make_pair(imgName,ImageCache()));
    iter = g_ImageCache.find( imgName );
  }

  cache = &iter->second;

  if ( painter.viewUpdated || now - cache->timestamp >= imageObserver->reportMSec )
  {
    cache->timestamp = now;
    
    rgbImg img( imageObserver->calcImage() );

    float sx, sy;
    int x0, y0, x1, y1;

    painter.getCoord( x0, y0, imageObserver->rect().x, imageObserver->rect().y-imageObserver->reportDistance ); 
    painter.getCoord( x1, y1, imageObserver->rect().x + imageObserver->rect().width, imageObserver->rect().y + imageObserver->rect().height-imageObserver->reportDistance ); 

    int width  = x1 - x0;
    int height = y0 - y1;
  
    float scale = width / (float)img.width();

    int cx0 = -x0;
    int cy0 = -y1;

    int cx1 = painter.width  - x0;
    int cy1 = painter.height - y1;

    int dx0 =  0;
    int dy0 =  0;

    if ( cx0 < 0 )
    { dx0 = -cx0;
      cx0 = 0;
    }

    if ( cy0 < 0 )
    { dy0 = -cy0;
      cy0 = 0;
    }
  
    int ix0 = floor(cx0 / scale);
    int ix1 = ceil (cx1 / scale);

    if ( ix1 >= img.width() )
      ix1 = img.width()-1;

    int iy0 = floor(cy0 / scale);
    int iy1 = ceil (cy1 / scale);
 
    if ( iy1 >= img.height() )
      iy1 = img.height()-1;

    if ( dx0 == 0 )
    { double frac = cx0 / scale - ix0;
      dx0 = -frac * scale;
    }
  
    if ( dy0 == 0 )
    { double frac = cy0 / scale - iy0;
      dy0 = -frac * scale;
    }
  
    if ( colChannels != img.spectrum() )
    {
      rpImg timg( img.crop(ix0, iy0, 0, 0, ix1, iy1, 0, 2) );
      cache->img = rpImg( timg.width(), timg.height(), 1, colChannels, 0xff );
      
      rpImg &cimg( cache->img );
      for ( int y = cimg.height()-1; y >= 0; --y )
      { for ( int x = cimg.width()-1; x >= 0; --x )
        { cimg( x, y, 0, 0 ) = timg( x, y, 0, 0 );
	  cimg( x, y, 0, 1 ) = timg( x, y, 0, 1 );
	  cimg( x, y, 0, 2 ) = timg( x, y, 0, 2 );
	  if ( colChannels == 4 )
	    cimg( x, y, 0, 2 ) = 0xff;   
        }
      }
    }
    else
      cache->img = img.crop( ix0, iy0, 0, 0, ix1, iy1, 0, colChannels-1 );

    int cwidth  = (ix1-ix0) * scale;
    int cheight = (iy1-iy0) * scale;
    cache->img.resize( cwidth, cheight, -100, -100, (imageObserver->type&TrackableObserver::FlowMap) ? 3 : 2 );

    cache->dx0 = dx0;
    cache->dy0 = dy0;
  }

  const float opacity = ((imageObserver->type&TrackableObserver::FlowMap) ? 1.0 : 0.8);

  painter.img->draw_image( cache->dx0, cache->dy0, cache->img, opacity );

//      uint64_t endTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//      printf( "msec: %ld\n", endTime-now );
}
      
static void
paintImageObserver( LidarPainter &painter, TrackableObserver::ObsvType type )
{
  if ( g_Track.m_Stage->observer == NULL )
    return;
  
  TrackableMultiObserver &multi( *g_Track.m_Stage->observer );
  for ( int i = 0; i < multi.observer.size(); ++i )
    if ( multi.observer[i]->type & type )
    { TrackableImageObserver *imageObserver = static_cast<TrackableImageObserver*>( multi.observer[i] );
      paintImageObserver( painter, imageObserver );
    }
}


static bool
hasObserverOfType( TrackableObserver::ObsvType type )
{
  if ( g_Track.m_Stage->observer == NULL )
    return false;
  
  TrackableMultiObserver &multi( *g_Track.m_Stage->observer );
  for ( int i = 0; i < multi.observer.size(); ++i )
    if ( multi.observer[i]->type & type )
      return true;
  
  return false;
}


/***************************************************************************
*** 
*** httpserver
***
****************************************************************************/

static bool
sendToInVirtual( LidarDevice &device, const char *path )
{
  std::string restURL( inVirtualUrl( device ) );

  if ( restURL.empty() )
    return false;

  restURL += path;

  std::string cmd = "wget \"";
  
  cmd += restURL;
  cmd += "\" -q -O /dev/null &>> /dev/null >> /dev/null &";

  if ( g_Verbose )
    TrackGlobal::info( "running: '%s'", cmd.c_str() );

  system( cmd.c_str() );

  return true;
}

static void
runDevice( LidarDevice *device, bool run )
{
  if ( run )
  { if ( g_DoTrack )
      g_Track.start( playerTimeStamp(), device );
    device->open();
    sendToInVirtual( *device, "/start" );
  }
  else
  { sendToInVirtual( *device, "/stop" );
    device->close();
    if ( g_DoTrack )
      g_Track.stop( playerTimeStamp(), device );
  }
}

static void
rebootNode( LidarDevice *device )
{
  log( "DEVICE rebooting %s...", device->getNikName().c_str() );
  sendToInVirtual( *device, "/reboot?this=true" );
}

static std::vector<std::string> activeGroupNames;

static void
runActiveGroup( bool stop=true )
{
  setPlayerSyncTime();

  LidarDeviceList &devices( g_Devices.activeDevices() );

//  printf( "runActiveGroup( %d ) inactive: %ld\n", stop,g_Devices._inactiveDevices.size()  );
  
  for ( int d = 0; d < devices.size(); ++d )
    runDevice( devices[d], true );

  for ( int d = 0; d < g_Devices._inactiveDevices.size(); ++d )
    runDevice( g_Devices._inactiveDevices[d], false );
}

static void
activateGroup( const char *groupName, bool reRun=true )
{
//  printf( "tool: activateGroup( %s ) reRun=%d\n", groupName, reRun );
  
  if ( g_Devices._activeDevices.groupName != groupName )
  {
    g_Devices.activateGroup( groupName );


    if ( g_Devices._activeDevices.groupName == "all" )
    {
      activeGroupNames.resize( 0 );

      for ( KeyValueMap::iterator iter( g_UsedGroups.begin() ); iter != g_UsedGroups.end(); iter++ )
      { std::string groupName( iter->first );
	activeGroupNames.push_back( groupName );
      }
    }
    else
      activeGroupNames = split( g_Devices._activeDevices.groupName, ',' );

    for ( int i = 0; i < g_DeviceUI.size(); ++i )
      g_DeviceUI[i].show = true;

    if ( reRun )
      runActiveGroup();
  }
}


static bool
isStarted()
{
  bool started = false;

  if ( g_IsHUB )
    started = g_HUBStarted;
  else
  {
    LidarDeviceList devices( g_Devices.activeDevices() );
  
    for ( int i = 0; i < devices.size(); ++i )
      if ( devices[i]->isOpen() )
      { started = true;
	break;
      }
  }

  return started;
}

using namespace httpserver;

static bool 
getBoolArg( const http_request& req, const char *label, bool &value )
{
  std::string string = req.get_arg( label );
  
  return getBool( string.c_str(), value );
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
getFloatArg( const http_request& req, const char *label, float &value )
{
  std::string string = req.get_arg( label );

  if ( string.empty() )
    return false;

  value = std::stof( string );

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
/*
  replace( value, "%20", " ");
  replace( value, "%22", "\"");
  replace( value, "%3C", "<");
  replace( value, "%3E", ">");
  replace( value, "%23", "#");
  replace( value, "%25", "%");
  replace( value, "%7C", "|");
*/

  return true;
}


static std::string
getPainterKey( const http_request& req )
{
  int clientId = 0;
  std::string key( "default" );
  
  std::string cookie;
  
  if ( getIntArg( req, "clientId", clientId ) )
    key = std::to_string( clientId );
  else if ( !(cookie=req.get_cookie("lidartool")).empty() )
    key = cookie;

  return key;
}

static LidarPainter &
getPainter( const http_request& req )
{
  std::string key( getPainterKey( req ) );
  
  std::map<std::string,LidarPainter>::iterator iter( painters.find(key) );
    
  if ( iter == painters.end() )
  { painters.emplace( std::make_pair(key,LidarPainter()) );
    iter = painters.find( key );
    iter->second.setUIImageFileName( uiImageType.c_str(), key.c_str() );
//    printf( "create key %s\n", key.c_str() );
  }
//  printf( "got key %s\n", key.c_str() );
  
  LidarPainter &painter( iter->second );
  painter.lastAccess = getmsec();
  
  return painter;
}

static void
addCheckedButton( std::string &result, const char *idName, const char *name, bool checked )
{
  result += "	  <div class=\"dropdown-item\">\n"
    "	    <input type=\"checkbox\" class=\"form-check-input me-1\" id=\"";
  result += idName;
  result += "\" name=\"";
  result += name;
  result += "\"";

  if ( checked )
    result += " checked=\"checked\"";

  result += "\">\n"
"	    <label class=\"custom-control-label\" for=\""
;
  result += idName;
  result += "\">";
  result += name;
  result += "</label>\n"
"	  </div>\n";
}

static void
addButtonForDevice( std::string &result, LidarDevice &device, const char *rootId, const char *idName, const char *d, const char *groupName="all" )
{
  result += "<li><div class=\"dropdown-item\">"
    "		<button class=\"btn\" name=\"";
  result += device.getNikName();
  result += "\" id=\"";
  result += rootId;
  result += "_";
  result += idName;
  result += groupName;
  result += d;
  result += "\">";
  result += device.getNikName();
  result += "</button>"
"	      </div>"
    "	    </li>";
}


static void
addButtonsForDevices( std::string &result, LidarDeviceList &devices, const char *rootId, const char *memberId, const char *groupName="all" )
{
  result += "<li id=\"";
  result += rootId;
  result += "\">\n";

  for ( int d = 0; d < devices.size(); ++d )
  { std::string dStr( std::to_string( d ) );
    addButtonForDevice( result, *devices[d], rootId, memberId, dStr.c_str() );
  }
 
  result += "</li>\n";
}

static void
addUIForDevice( std::string &result, LidarDevice &device, const char *rootId, const char *idName, const char *d, const char *groupName="all" )
{
  result += "	  <div class=\"dropdown-item\">\n"
    "	    <input type=\"checkbox\" class=\"form-check-input me-1\" content=\"";
  result += idName;
  result += d;
  result += "\" alt=\"";
  result += groupName;
  result += "\" id=\"";
  result += rootId;
  result += "_";
  result += idName;
  result += d;
  result += "\" name=\"";
  result += device.getNikName();
  result += "\"";
  
  if ( strcmp(rootId,"visibleDevices")==0 || (strcmp(rootId,"runDevices")==0 && device.shouldOpen) )
    result += "\" checked=\"checked\"";
  
  result += "\">\n"
"	    <label class=\"custom-control-label\" for=\"";
  result += rootId;
  result += "_";
  result += idName;
  result += d;
  result += "\">";
  result += device.getNikName();
  result += "</label>\n"
"	  </div>\n";
}


static void
addUIForDevices( std::string &result, LidarDeviceList &devices, const char *rootId, const char *memberId, const char *groupName="all" )
{
  result += "<li id=\"";
  result += rootId;
  result += "\">\n";

  for ( int d = 0; d < devices.size(); ++d )
  { std::string dStr( std::to_string( d ) );
    addUIForDevice( result, *devices[d], rootId, memberId, dStr.c_str() );
  }
 
  result += "</li>\n";
}


static void
addMenuForGroup( std::string &result, LidarDeviceList &devices, const char *groupName, const char *rootId, const char *memberId, const char *allNonePrefix=NULL )
{
  int count = 0;
  
  for ( int d = 0; d < devices.size(); ++d )
  { LidarDevice &device( *devices[d] );
    if ( g_Devices.deviceInGroup( device, groupName ) )
      count += 1;
  }

  if ( count < 4 )
    allNonePrefix = NULL;

  if ( allNonePrefix != NULL )
  {
    result += "<li><div class=\"dropdown-item\">"
    "		<button class=\"btn\" name=\"";
    result += groupName;
    result += "\" id=\"";
    result += allNonePrefix;
    result += "All";
    result += groupName;
    result += "\">All</button>"
"	      </div>"
"	    </li>"
"	    <li><div class=\"dropdown-item\">"
    "		<button class=\"btn\" name=\"";
    result += groupName;
    result += "\" id=\"";
    result += allNonePrefix;
    result += "None";
    result += groupName;
    result += "\">None</button>"
"	      </div>"
"	    </li>"
     "	    <li><hr class=\"dropdown-divider\"></li>";
  }

  for ( int d = 0; d < devices.size(); ++d )
  { LidarDevice &device( *devices[d] );

    if ( g_Devices.deviceInGroup( device, groupName ) )
    { std::string dStr( std::to_string( d ) );
      addUIForDevice( result, device, rootId, memberId, dStr.c_str(), groupName );
    }
  }
}

static void
addButtonMenuForGroup( std::string &result, LidarDeviceList &devices, const char *groupName, const char *rootId, const char *memberId, const char *allNonePrefix=NULL )
{
  int count = 0;
  
  for ( int d = 0; d < devices.size(); ++d )
  { LidarDevice &device( *devices[d] );
    if ( g_Devices.deviceInGroup( device, groupName ) )
      count += 1;
  }

  if ( count < 4 )
    allNonePrefix = NULL;

  if ( allNonePrefix != NULL )
  {
    result += "<li><div class=\"dropdown-item\">"
    "		<button class=\"btn\" name=\"";
    result += groupName;
    result += "\" id=\"";
    result += allNonePrefix;
    result += "All";
    result += groupName;
    result += "\">All</button>"
"	      </div>"
"	    </li>"
     "	    <li><hr class=\"dropdown-divider\"></li>";
  }

  for ( int d = 0; d < devices.size(); ++d )
  { LidarDevice &device( *devices[d] );

    if ( g_Devices.deviceInGroup( device, groupName ) )
    { std::string dStr( std::to_string( d ) );
      addButtonForDevice( result, device, rootId, memberId, dStr.c_str(), groupName );
    }
  }
}

static bool g_SubMenuLeft = false;

static void
addMenu( std::string &result, LidarDeviceList &devices, const char *rootId, const char *memberId, const char *allNonePrefix=NULL )
{
  result += "<li class=\"dropdown\" id=\"";
  result += rootId;
  result += "\">\n";

  if ( activeGroupNames.size() <= 1 )
    addMenuForGroup( result, devices, activeGroupNames.size() == 1 ? activeGroupNames[0].c_str() : "all", rootId, memberId );
  else
  {
    for ( int g = 0; g < activeGroupNames.size(); ++g )
    {
      result += "<li class=\"";
      result += rootId;
      result += "-item\"><a class=\"dropdown-item\" href=\"#\">";

      if ( g_SubMenuLeft )
      { result += "<div class=\"laquo\">&laquo; </div> &nbsp; &nbsp;\n";
	result += activeGroupNames[g];
	result += "</a>";
      }
      else
      {
	result += activeGroupNames[g];
	result += "<div class=\"raquo\">&raquo;</div></a>\n";
      }

      result += "<ul class=\"submenu";
      if ( g_SubMenuLeft )
	result += "-left";
      result += " dropdown-menu\">\n";
  
      addMenuForGroup( result, devices, activeGroupNames[g].c_str(), rootId, memberId, allNonePrefix );

      result += "</ul></li>\n";
    }
  }
 
  result += "</li>\n";
}

static void
addButtonMenu( std::string &result, LidarDeviceList &devices, const char *rootId, const char *memberId, const char *allNonePrefix=NULL )
{
  result += "<li class=\"dropdown\" id=\"";
  result += rootId;
  result += "\">\n";

  if ( devices.size() == 0 )
    return;

  if ( activeGroupNames.size() <= 1 )
  {
    addButtonMenuForGroup( result, devices, activeGroupNames.size() == 1 ? activeGroupNames[0].c_str() : "all", rootId, memberId );
  }
  else
  {
    for ( int g = 0; g < activeGroupNames.size(); ++g )
    {
      result += "<li class=\"";
      result += rootId;
      result += "-item\"><a class=\"dropdown-item\" href=\"#\">";

      if ( g_SubMenuLeft )
      { result += "<div class=\"laquo\">&laquo; </div> &nbsp; &nbsp;\n";
	result += activeGroupNames[g];
	result += "</a>";
      }
      else
      {
	result += activeGroupNames[g];
	result += "<div class=\"raquo\">&raquo;</div></a>\n";
      }

      std::string subMenu;
      addButtonMenuForGroup( subMenu, devices, activeGroupNames[g].c_str(), rootId, memberId, allNonePrefix );

      if ( !subMenu.empty() )
      {
	result += "<ul class=\"submenu";
	if ( g_SubMenuLeft )
	  result += "-left";
	result += " dropdown-menu\">\n";
  
	result += subMenu;

	result += "</ul></li>\n";
      }
    }
  }
 
  result += "</li>\n";
}


static webserver *webserv = NULL;

static std::shared_ptr<http_response> 
stringResponse( const char *string, const char *mimeType="text/plain", int errorCode=200 )
{
//  printf( "stringResponse\n" );

  std::shared_ptr<http_response> response = std::shared_ptr<http_response>(new string_response(string,errorCode,mimeType));
  response->with_header("Access-Control-Allow-Origin", "*");
  return response;
}

static std::shared_ptr<http_response> 
htmlResponse( std::string string )
{
//   printf( "htmlResponse\n" );

  std::shared_ptr<http_response> response = std::shared_ptr<http_response>(new string_response(string,200,"text/html"));
  response->with_header("Access-Control-Allow-Origin", "*");
  return response;
}

static std::shared_ptr<http_response> 
jsonResponse( std::string string )
{
//   printf( "jsonResponse\n" );

  std::shared_ptr<http_response> response = std::shared_ptr<http_response>(new string_response(string,200,"application/json"));
  response->with_header("Access-Control-Allow-Origin", "*");
  return response;
}

static std::shared_ptr<http_response> 
fileResponse( std::string path, const char *mimeType, int errorCode=200 )
{
//   printf( "fileResponse\n" );

  std::shared_ptr<http_response> response;
  
  if ( fileExists( path.c_str() ) )
    response = std::shared_ptr<http_response>(new file_response(path,errorCode,mimeType));
  else if ( fileExists( (LidarDevice::configDir+path).c_str() ) )
    response = std::shared_ptr<http_response>(new file_response(LidarDevice::configDir+path,errorCode,mimeType));
  else if ( fileExists( (g_HTMLDir+path).c_str() ) )
    response = std::shared_ptr<http_response>(new file_response(g_HTMLDir+path,errorCode,mimeType));
  else if ( fileExists( (g_InstallDir+path).c_str() ) )
    response = std::shared_ptr<http_response>(new file_response(g_InstallDir+path,errorCode,mimeType));
  else
    response = std::shared_ptr<http_response>(new file_response(path,errorCode,mimeType));
    
  response->with_header("Access-Control-Allow-Origin", "*");
  return response;
}

class sensorIN_resource : public http_resource {
public:

  render_const std::shared_ptr<http_response> render_GET(const http_request& req) {

    std::string fileName( g_SensorINFileName );
    std::ifstream stream( fileName );
    if ( !stream.is_open() )
      return stringResponse( "" );

    std::string content;
    std::getline(stream, content, '\0' );
    
    return stringResponse( content.c_str() );
  }

  render_const std::shared_ptr<http_response> render_POST(const httpserver::http_request& req) {

    std::string post_response;

    auto content = req.get_arg( "sensorIN" );
    
    while ( replace( content, "\r", "" ) )
      ;

    std::string fileName( g_SensorINFileName );
    std::ofstream stream( fileName );
    if ( !stream.is_open() )
      return stringResponse( "error" );
  
    if ( g_Verbose )
      TrackGlobal::info( "writing sensorIN to file %s", g_SensorINFileName.c_str() );

    stream << content;
    stream.close();
    
    readSensorIN();

    return stringResponse( "ok" );
  }
};

  
class lastErrors_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

      //      printf( "lastErrors_resource\n" );

      std::string result;

      std::ifstream stream( g_ErrorLogFile.c_str() );
      if ( stream.is_open() )
      {
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(stream, line))
	  if ( !line.empty() )
	    lines.push_back( line );
	int size = lines.size();
	if ( size > g_ErrorLogHtmlLines )
	  size = g_ErrorLogHtmlLines;

	int length = 10;
	std::string timestamp1( timestampString( "%c", getmsec(), false ) );
	timestamp1[length] = '\0';
	std::string timestamp1Start( timestamp1.c_str() );
	
	std::string timestamp2( timestampString( "%c", getmsec()-24*3600*1000, false ) );
	timestamp2[length] = '\0';
	std::string timestamp2Start( timestamp2.c_str() );

	LidarDeviceList devices( g_Devices.allDevices() );

	for ( int i = 0; i < size; ++i )
        { 
	  std::vector<std::string> pair( split(lines[lines.size()-1-i],']',2) );
	  std::vector<std::string> triplet( split(pair[pair.size()-1],'\'') );

 	  if ( triplet.size() == 3 )
	  {
	    std::string line;
	    std::string okString( " ok" );
	    std::string date( triplet[0] );
	    replace( date, " [Error] Failure on Device ", "" );
	    replace( date, " [Error] Device ", "" );
	    trim( date );
	    
	    bool ok       =  endsWith( triplet[2], okString );
	    bool alertDay = (startsWith( date, timestamp1Start ) || startsWith( date, timestamp2Start ));
	    
	    if ( alertDay )
	    {
	      if ( ok )
		line += "<b class=\"okDay\">";
	      else
		line += "<b class=\"alertDay\">";
	    }
 
	    line += date;

	    if ( alertDay )
	      line += "</b>";

	    line += " ";

	    bool hasAnchor = false;
	    for ( int d = 0; d < devices.size(); ++d )
            {
	      if ( devices[d]->getNikName() == triplet[1] )
	      {
		if ( devices[d]->inFile == NULL )
	        {
		  std::string url( inVirtualUrl( *devices[d] ) );
		  if ( !url.empty() )
	          { std::string anchor( "<a href=\"#\" onclick=\"window.open('" + url + "','" + devices[d]->inVirtUrl + "');\">" );
		    line += anchor;
		    hasAnchor = true;
		  }
		}
		break;
	      }
	    }

	    line += "<b>" + triplet[1] + "</b>";

	    if ( hasAnchor )
	      line += "</a>";

	    if ( ok )
	      line += "<span class=\"deviceOk\">";
	    else
	      line += "<span class=\"deviceError\">";
	      
	    line += triplet[2];
	    line += "</span>";

	    replace( line, "Reason: ", "" );
	    result += line;

	    result += "<br>";
	  }
	}

	if ( !result.empty() )
        { result = "<pre>" + result;
	  result += "</pre>";

	}
      }

      std::shared_ptr<http_response> response = stringResponse( result.c_str() );
  
      return response;
    }
};

class reboot_resource : public http_resource {
public:
  render_const std::shared_ptr<http_response> render(const http_request& req) {
//  printf( "reboot_resource\n" );

      bool yes   = false;
      bool all   = false;
      std::string group;

      getBoolArg( req, "all",  all );
      getStringArg( req, "group", group );

      webMutex.lock();
      
      LidarDeviceList devices( g_Devices.allDevices() );

      for ( int i = ((int)devices.size())-1; i >= 0; --i )
      {
	LidarDevice *device( devices[i] );
	
	bool set = false;
	if ( all || g_Devices.deviceInGroup( *device, group.c_str() ) ||
	     (getBoolArg( req, device->getNikName().c_str(), set ) && set) )
	     rebootNode( device );
      }
	
      webMutex.unlock();

      if ( getBoolArg( req, "this", yes ) && yes )
      { log( "REBOOT rebooting this..." );
       	system( "(sleep 1; sudo reboot) &" );
      }
      
      return stringResponse( "Reboot Devices" );
    }
};

class run_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

      //  printf( "run_resource\n" );


      webMutex.lock();
      
      LidarDeviceList &devices( g_Devices.activeDevices() );

      setPlayerSyncTime();
      
      for ( int d = 0; d < devices.size(); ++d )
      { bool run;

	if ( getBoolArg( req, devices[d]->getIdName().c_str(), run ) )
	  runDevice( devices[d], run );
	else if ( getBoolArg( req, devices[d]->getNikName().c_str(), run ) )
	  runDevice( devices[d], run );
      }

      webMutex.unlock();
      
      return stringResponse( "Run Devices" );
    }
};

class start_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "start_resource\n" );

      log( withRunningMode("START by API").c_str() );

      webMutex.lock();
      
      if ( !g_IsStarted )
      { g_IsStarted = true;
	TrackGlobal::notification( "start", "message=\"Start by API\" runMode=%s verbose=%s", g_RunningMode.c_str(), g_Verbose?"true":"false" );
      }
      
      LidarDeviceList devices( g_Devices.allDevices() );

      setPlayerSyncTime();

      for ( int d = 0; d < devices.size(); ++d )
      {
	devices[d]->open();
	
	sendToInVirtual( *devices[d], "/start" );
      }
	
      if ( g_DoTrack )
	g_Track.start( playerTimeStamp() );

      webMutex.unlock();
      
      return stringResponse( "Started Devices" );
    }
};

class reopen_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "reopen_resource\n" );

      bool all = false;
      getBoolArg( req, "all", all );

      uint64_t	reopenTime = getmsec() + 3000;

      webMutex.lock();
      
      LidarDeviceList devices( g_Devices.allDevices() );

      for ( int d = 0; d < devices.size(); ++d )
      {
	bool reopen;
	if ( all ||
	     (getBoolArg( req, devices[d]->getIdName().c_str(),reopen)&&reopen) ||
	     (getBoolArg( req, devices[d]->getNikName().c_str(),reopen) && reopen) )
        {
	  g_DeviceFailed.set( devices[d]->getNikName().c_str(), "-1" );

	  sendToInVirtual( *devices[d], "/reopen?all=true" ); 
	  devices[d]->close();
	  devices[d]->reopenTime = reopenTime;
	}
      }
	
      webMutex.unlock();

      return stringResponse( "Reopened" );
    }
};

class stop_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "stop_resource\n" );

      log( "STOP by API" );

      webMutex.lock();
      
      if ( g_IsStarted )
      { g_IsStarted = false;
	TrackGlobal::notification( "stop", "message=\"Stop by API\" runMode=%s verbose=%s", g_RunningMode.c_str(), g_Verbose?"true":"false" );
      }
      
      LidarDeviceList devices( g_Devices.allDevices() );

      for ( int d = 0; d < devices.size(); ++d )
      {
	sendToInVirtual( *devices[d], "/stop" );

	devices[d]->close();
      }
      
      if ( g_DoTrack )
	g_Track.stop( playerTimeStamp() );
 
      stopFailures();

      webMutex.unlock();
      
      return stringResponse( "Stopped Devices" );
    }
};

class scanEnv_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "scanEnv_resource\n" );

      std::string scanSec  = req.get_arg("sec");
      int sec = 0;
      
      if ( !scanSec.empty() && std::stoi( scanSec ) > 0 )
	sec = std::stoi( scanSec );

      webMutex.lock();
      
      if ( g_Devices.isSimulationMode() )
      { LidarDeviceList devices( g_Devices.activeDevices() );
	createSimulationEnvMaps( devices );
      }
      else
	g_Devices.scanEnv();
      
      webMutex.unlock();
      
      return stringResponse( "Scanning Environment" );
    }
};

class loadEnv_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "loadEnv_resource\n" );

      webMutex.lock();
      
      g_Devices.loadEnv();
      
      webMutex.unlock();
      
      return stringResponse( "Loading environment" );
    }
};


class saveEnv_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "saveEnv_resource\n" );

      webMutex.lock();
      
      g_Devices.saveEnv( true );
      
      webMutex.unlock();
      
      return stringResponse( "Saving Environment" );
    }
};


class resetEnv_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "resetEnv_resource\n" );

      webMutex.lock();
      
      g_Devices.resetEnv();
      
      webMutex.unlock();
      
      return stringResponse( "Reseting Environment" );
    }
};


class regions_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "regions_resource\n" );
      std::string createName  = req.get_arg("create");

      if ( !createName.empty() )
      { 
	TrackableRegion *rect = TrackGlobal::regions.get( createName.c_str(), true );
	
	float x      = rect->x1();
	float y      = rect->y1();
	float width  = rect->width;
	float height = rect->height;

	getFloatArg( req, "x", x );
	getFloatArg( req, "y", y );
	getFloatArg( req, "width",  width );
	getFloatArg( req, "height", height );

	webMutex.lock();
      
	TrackGlobal::regions.set( createName.c_str(), x+width*0.5, y+height*0.5, width, height );

	webMutex.unlock();

	return jsonResponse( "{ \"success\": true }" );
      }
      
      return jsonResponse( "{}" );
    }
};


class loadRegions_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "loadRegions_resource\n" );

      webMutex.lock();

      TrackGlobal::regions.clear();
      TrackGlobal::loadRegions();
      
      webMutex.unlock();
      
      return stringResponse( "Loading Regions" );
    }
};


class saveRegions_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "saveRegions_resource\n" );

      webMutex.lock();
      
      TrackGlobal::saveRegions();
      
      webMutex.unlock();
      
      return stringResponse( "Saving Regions" );
    }
};


class saveBlueprint_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "saveBlueprint_resource\n" );

      webMutex.lock();
      
      writeBlueprints();
      
      webMutex.unlock();
      
      return stringResponse( "Saving Blueprint" );
    }
};


class deviceList_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render_GET(const http_request& req) {

      //  printf( "deviceList_resource\n" );

      std::string result;

      webMutex.lock();

      LidarDeviceList &devices( g_Devices.activeDevices() );

      char text[1024];
      
      for ( int d = 0; d < devices.size(); ++d )
      {
	LidarDevice &device( *devices[d] );
	std::string deviceType( device.info.detectedDeviceType );
	if ( deviceType.empty() )
        { if ( !device.deviceType.empty() )
	    deviceType = device.deviceType;
	  else
	    deviceType = "unknown";
	}

	std::string sid;
	if ( !device.sensorIN.empty() )
        { sid  = " [SIN=";
	  sid += device.sensorIN;
	  sid += "]";
	}

	bool hasPowerSupport = device.devicePoweringSupported();
	const char *poweringSupported = (hasPowerSupport?"P":"p");

	sprintf( text, "%d: %s (%s,%s)%s ", d+1, device.getNikName().c_str(), deviceType.c_str(), poweringSupported, sid.c_str() );

	const unsigned char *back = midGray;

	if ( device.isOpen() )
        {
	  char tmp[1024] = "";
	  LidarDevice::Info info;
  
	  if ( device.isPoweringUp )
          { sprintf( tmp, "powering up\n" );
	    back = darkerYellow;
	  }
	  else if ( !device.dataReceived )
          { sprintf( tmp, "no data" );
	    back = (device.isReady() ? red : darkerYellow);
	  }
	  else if ( device.isReady() && device.getInfo( info ) )
          { sprintf( tmp, "fps=%d samples=%d", info.average_fps.fps, info.average_samples.average() );
	    back = darkGreen;
	  }
	  else
          { sprintf( tmp, "undefined status" );
	    back = red;
	  }

	  strcat( text, tmp );
	}
	else
        {
	  if ( !device.errorMsg.empty() )
          { strcat( text, device.errorMsg.c_str() );
	    back = red;
	  }
	  else
          {
	    strcat( text, "stopped" );
	    if ( !hasPowerSupport )
	      back = grayGreen;
	  }
	}

	bool failed = false;
	if ( g_DeviceFailed.get( device.getNikName().c_str(), failed ) && failed )
	  back = red;

	if ( !device.outVirtUrl.empty() )
        { strcat( text, " > virtual:" );
	  strcat( text, device.outVirtUrl.c_str() );
	}

	char col[16];
	sprintf( col, "%02x%02x%02x", back[0], back[1], back[2] );

	result += "<div><span class=\"dot\" style=\"background-color: #";
	result += col;
	result += ";\"></span>&nbsp;&nbsp;";
	result += text;
	result += "</div>";
      }
      
      webMutex.unlock();
      
      return htmlResponse( result );
    }
};


static int
getNumFailedDevices()
{
  int result = 0;

  LidarDeviceList &devices( g_Devices.activeDevices() );

  uint64_t now = getmsec();
  for ( int d = 0; d < devices.size(); ++d )
  {
    LidarDevice &device( *devices[d] );
	  
    bool failed = false;
    
    if ( device.isOpen() )
    {
      LidarDevice::Info info;
  
      uint64_t timeDiff = now - device.openTime;
      uint64_t recvDiff = (now < device.receivedTime ? 0 : now - device.receivedTime);

      if ( device.isPoweringUp )
	;
      else if ( !device.dataReceived || recvDiff > g_WarningReportMSec )
	;
      else if ( device.isReady() && device.getInfo( info ) )
	;
      else
	failed = true;
    }
    else
    {
      if ( !device.errorMsg.empty() )
	failed = true;
    }

    if ( !failed )
      g_DeviceFailed.get( device.getNikName().c_str(), failed );
    
    if ( failed )
      result += 1;
  }	

  return result;
}


static std::string
getDeviceHealth()
{
  std::string result;

  static const char *midGray   = "stopped";
  static const char *darkGreen = "ok";
  static const char *yellow    = "warning";
  static const char *red       = "error";
  
  const char *Back = midGray;

  LidarDeviceList &devices( g_Devices.activeDevices() );

  uint64_t now = getmsec();
  for ( int d = 0; d < devices.size(); ++d )
  {
    LidarDevice &device( *devices[d] );
	  
    const char *back = NULL;

    if ( device.isOpen() )
    {
      LidarDevice::Info info;
  
      uint64_t timeDiff = now - device.openTime;
      uint64_t recvDiff = (now < device.receivedTime ? 0 : now - device.receivedTime);

      if ( device.isPoweringUp )
	back = yellow;
      else if ( !device.dataReceived || recvDiff > g_WarningReportMSec )
	back = (device.isReady() && timeDiff/1000 > g_FailureReportSec ? red : yellow);
      else if ( device.isReady() && device.getInfo( info ) )
	back = darkGreen;
      else
	back = red;
    }
    else
    {
      if ( !device.errorMsg.empty() )
	back = red;
    }

    bool failed = false;
    if ( g_DeviceFailed.get( device.getNikName().c_str(), failed ) && failed )
      back = red;
	
    if ( back == red )
      Back = red;
    else if ( back == yellow && Back != red )
      Back = back;
    else if ( back == darkGreen && Back != red && Back != yellow )
      Back = back;
    else if ( back == midGray && Back != red && Back != yellow )
      Back = back;
    else if ( back == NULL )
      Back = back;
  }	

  if ( Back == NULL )
    Back = midGray;

  return Back;
}


class status_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render_GET(const http_request& req) {

      //  printf( "status_resource\n" );

      std::string started( "started" );
      std::string stopped( "stopped" );
      std::string damaged( "damaged" );

      std::string json = "{ \"status\": ";
 
      webMutex.lock();
      
      bool isStarted = false;
      if ( g_IsHUB )
	isStarted = g_HUBStarted;
      else
	isStarted = g_IsStarted;

      std::string status( isStarted?started:stopped );

      int numDevices       = (int)g_Devices.size();
      int numFailedDevices = getNumFailedDevices();

      if ( numFailedDevices >= 2 || (numDevices > 0 && numFailedDevices/(float)numDevices > 0.5 ) )	   
	status = damaged;

      json += "\"" + status + "\"";

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

      json += ", \"appStartDate\": ";
      json += "\"" + g_AppStartDate + "\"";

      json += " }";

      webMutex.unlock();
           
      return jsonResponse( json );
    }
};

class get_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "get_resource\n" );

      std::string region, group;
      
      std::string json = "{";

      webMutex.lock();
      
      LidarPainter &painter( getPainter( req ) );
      
      if ( getStringArg( req, "region",  region ) )
      {  
	TrackableRegion &reg( *TrackGlobal::regions.get( region.c_str(), true ) );

	int x1, y1, x2, y2;
	painter.getCanvCoord( x1, y1, reg.x1(), reg.y1() );    
	painter.getCanvCoord( x2, y2, reg.x2(), reg.y2() );

//	printf( "%d %d %d %d  (%g %g %g %g)\n", x1, y1, x2, y2, reg.x,    reg.y, reg.x2(), reg.y2() );
	
	json += "\"x\": " + std::to_string( (int)x1 ) + ",";
	json += "\"y\": " + std::to_string( (int)y2 ) + ",";
	json += "\"w\": " + std::to_string( (int)(x2-x1) ) + ",";
	json += "\"h\": " + std::to_string( (int)(y1-y2) ) + ",";
	json += "\"name\": \"" + reg.name + "\",";
	json += "\"shape\": \""  + TrackableRegion::regionShape_str( (RegionShape) reg.shape ) + "\"";

//	json = LidarRegsions::toString( TrackGlobal::regions );
	json += " }";
      }
      else
      {
	bool adaptEnv       = false;
	bool useEnv         = false;
	bool useGroups      = false;
	bool hasRemote      = false;
	bool webId          = false;
	bool frameTime      = false;
	bool isStarted      = false;
	bool sensorsStarted = false;
	bool expertMode     = false;
	bool runningMode    = false;
	bool playPos        = false;
	bool hasPlayPos     = false;
	bool hasLidar       = false;
	bool hasSensorIN    = false;
	bool sensorPowerEnabled = false;
	bool isPaused       = false;
	bool conf           = false;
	bool deviceHealth   = false;
	bool numDevices     = false;
	bool numFailedDev   = false;
	bool availableDevices = false;
	bool spinningDevices = false;
	bool useBluePrints  = false;
	bool useOcclusion   = false;
	bool useObstacle    = false;
	bool bluePrintTsf   = false;
	bool obstacleTsf    = false;
	bool appStartDate   = false;
	bool first = true;

	if ( getBoolArg( req, "adaptEnv",  adaptEnv ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"adaptEnv\": ";
	  if ( g_Devices.size() > 0 )
	    json += g_Devices[0]->doEnvAdaption ? "true" : "false";
	  else
	    json += "false";
	}

	if ( getBoolArg( req, "useEnv", useEnv ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"useEnv\": ";
	  json += g_Devices._useEnv ? "true" : "false";
	}

	if ( getBoolArg( req, "useGroups", useGroups ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"useGroups\": ";
	  json += (LidarDeviceGroup::groups.size() > 1 && g_UsedGroups.size() > 1 ? "true" : "false");
	}

	if ( getBoolArg( req, "hasRemote", hasRemote ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"hasRemote\": ";
	  json += (g_Devices.remoteDevices().size() > 0 ? "true" : "false");
	}

	if ( getBoolArg( req, "isStarted", isStarted ) || getBoolArg( req, "sensorsStarted", sensorsStarted ) )
        {
	  bool started = false;

	  if ( g_IsHUB )
	    started = g_HUBStarted;
	  else
	    started = g_IsStarted;

	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  if ( isStarted )
	    json += "\"isStarted\": ";
	  else
	    json += "\"sensorsStarted\": ";
	  json += (started ? "true" : "false");
	}

	if ( getBoolArg( req, "expertMode", expertMode ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"expertMode\": ";
	  json += (g_ExpertMode ? "true" : "false");
	  json += "";
	}

	if ( getBoolArg( req, "runningMode", runningMode ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"runningMode\": \"";
	  json += g_RunningMode;
	  json += "\"";
	}

	if ( getBoolArg( req, "deviceHealth", deviceHealth ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"deviceHealth\": \"";
	  json += getDeviceHealth();
	  json += "\"";
	}

	if ( getBoolArg( req, "numDevices", numDevices ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"numDevices\": \"";
	  json += std::to_string( g_Devices.size() );
	  json += "\"";
	}

	if ( getBoolArg( req, "numFailedDevices", numFailedDev ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"numFailedDevices\": \"";
	  json += std::to_string( getNumFailedDevices() );
	  json += "\"";
	}

	if ( getBoolArg( req, "availableDevices", availableDevices ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"availableDevices\": \"";
	  json += g_AvailableDevices;
	  json += "\"";
	}

	if ( getBoolArg( req, "spinningDevices", spinningDevices ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"spinningDevices\": ";
	  json += getSpinningDevices();
	}

	if ( getBoolArg( req, "appStartDate", appStartDate ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"appStartDate\": \"";
	  json += g_AppStartDate;
	  json += "\"";
	}

	if ( getBoolArg( req, "frameTime", frameTime ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"frameTime\": ";
	  json += std::to_string( frameTimeAverage );
	}

	if ( getBoolArg( req, "hasLidar", hasLidar ) )
        {
	  bool has = (g_Devices.size() > 0 && TrackBase::packedPlayer() == 0);

	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"hasLidar\": ";
	  json += (has ? "true" : "false");
	}

	if ( getBoolArg( req, "hasSensorIN", hasSensorIN ) )
        {
	  bool has = fileExists( g_SensorINFileName.c_str() );
	    
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"hasSensorIN\": ";
	  json += (has ? "true" : "false");
	}

	if ( getBoolArg( req, "sensorPowerEnabled", sensorPowerEnabled ) )
        {
	  bool has = false;

	  std::ifstream stream( "hardware/LidarPower.enable" );
	  if ( stream.is_open() )
          {
	    std::string content;
	    stream >> content;
	    trim(content);
	    has= ( content == "true" );
	  }
	  
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"sensorPowerEnabled\": ";
	  json += (has ? "true" : "false");
	}

	if ( getBoolArg( req, "hasPlayPos", hasPlayPos ) )
        {
	  bool has = (playerPlayPos() >= 0);

	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"hasPlayPos\": ";
	  json += (has ? "true" : "false");
	}

	if ( getBoolArg( req, "playPos", playPos ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	  
	  json += "\"playPos\": ";
	  json += std::to_string( playerPlayPos() );
	}

	if ( getBoolArg( req, "isPaused", isPaused ) )
        {
	  bool isPaused = playerIsPaused();
	  
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"isPaused\": ";
	  json += (isPaused ? "true" : "false");
	}

	if ( getBoolArg( req, "webId",  webId ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"webId\": \"";
	  json += std::to_string( ::webId );
	  json += "\"";
	}

	if ( getBoolArg( req, "conf",  conf ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"conf\": \"";
	  json += g_Config;
	  json += "\"";
	}

	if ( getBoolArg( req, "useBluePrints",  useBluePrints ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"useBluePrints\": ";

	  json += !bluePrintFileName.empty() ? "true" : "false";
	}

	if ( getBoolArg( req, "useOcclusion",  useOcclusion ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"useOcclusion\": ";

	  json += (!trackOcclusionMapFileName.empty() ? "true" : "false");
	}

	if ( getBoolArg( req, "bptsf", bluePrintTsf ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";

	  float sx = 1.0 / painter.extent * bluePrintExtentX * painter.width;
	  float sy = sx;
	  int tx, ty;

	  float x = bpMatrix.w.x - bluePrintExtentX/2;
	  float y = bpMatrix.w.y + bluePrintExtentY/2;

	  painter.getCoord( tx, ty, x, y );    

	  json += "\"bptsf\": [ ";
	  json += std::to_string( sx ) + ", " + std::to_string( sy ) + ", " + std::to_string( tx ) + ", " + std::to_string( ty );
	  json += "]";
	}

	if ( getBoolArg( req, "useObstacle", useObstacle ) )
        {
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  json += "\"useObstacle\": ";
	  json += (g_UseObstacle && !obstacleFileName.empty()) ? "true" : "false";
	}

	if ( getStringArg( req, "group", group ) )
	{
	  std::string group;
	  
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  if ( g_Devices._activeDevices.groupName.empty() )
	    group = "all";
	  else
	    group = g_Devices._activeDevices.groupName;

	  json += "\"group\": \"";
	  json += group;
	  json += "\"";
	}
	
	if ( getStringArg( req, "prevRegion",  region ) || getStringArg( req, "nextRegion",  region ) )
        {  
	  std::string name;
	  
	  if ( first )
	    first = false;
	  else
	    json += ",";
	    
	  for ( int i = 0; i < TrackGlobal::regions.size(); ++i )
	  {
	    if ( TrackGlobal::regions[i].name == region )
	    {
	      if ( getStringArg( req, "prevRegion",  region ) )
		i -= 1;
	      else
		i += 1;

	      if ( i == TrackGlobal::regions.size() )
		i = 0;
	      else if ( i < 0 )
		i = TrackGlobal::regions.size()-1;

	      name = TrackGlobal::regions[i].name;
	      break;
	    }
	  }
	  
	  if ( name.empty() && TrackGlobal::regions.size() > 0 )
	    name = TrackGlobal::regions[0].name;

	  json += "\"name\": \"";
	  json += name;
	  json += "\"";
	}

	json += " }";
      }
      
//      printf( "json: '%s'\n", json.c_str() );

      webMutex.unlock();
           
      return jsonResponse( json );
    }
};


class set_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "set_resource\n" );

      bool  adaptEnv = true;
      bool  useEnv   = true;
      bool  paused   = false;
      bool sensorPowerEnabled = false;
      std::string sensorIN;
      
      float char1 = 1.0;
      float char2 = 0.0;
      float playPos;
      
      bool c1 = getFloatArg( req, "char1", char1 );
      bool c2 = getFloatArg( req, "char2", char2 );
      
      std::string region, group, showLayer;

      webMutex.lock();
      
      LidarPainter &painter( getPainter( req ) );
      
      if ( getStringArg( req, "region", region ) )
      {  
	TrackableRegion &reg( *TrackGlobal::regions.get( region.c_str(), true ) );
	    
	std::string shape;
	if ( getStringArg( req, "shape", shape ) )
	  reg.shape = TrackableRegion::regionShapeByString( shape.c_str() );
	
	int x, y, w, h;
	if ( getIntArg( req, "x",  x ) &&
	     getIntArg( req, "y",  y ) &&
	     getIntArg( req, "w",  w ) &&
	     getIntArg( req, "h",  h ) )
        {
	  float cw = w / (float)painter.canv_width  * painter.extent_x;
	  float ch = h / (float)painter.canv_height * painter.extent_y;
	  float cx =  (x-painter.canv_width/2)  / (float)painter.canv_width  * painter.extent_x - painter.matrix.w.x;
	  float cy = -(y-painter.canv_height/2) / (float)painter.canv_height * painter.extent_y - painter.matrix.w.y -ch;
	
	  reg.x = cx + cw * 0.5;
	  reg.y = cy + ch * 0.5;
	  reg.width  = cw;
	  reg.height = ch;

	  if ( g_Track.updateObserverRegion( region.c_str() ) )
	    painter.viewUpdated = true;
	}
      }
      else if ( getStringArg( req, "showLayer", showLayer ) )
      {
	bool show;
	if ( getBoolArg( req, "show", show ) )
        {
	  if ( showLayer == "No Layer" )
	    showLayer = "";

	  if ( show )
	    painter.layers.emplace( showLayer );
	  else
	    painter.layers.erase( showLayer );
	}
      }
      else if ( getStringArg( req, "group", group ) )
      {
	activateGroup( group.c_str() );
      }
      else if ( c1 || c2 )
      {
	std::string devType;
	getStringArg( req, "devType", devType );
	g_Devices.setCharacteristic( char1, char2, devType.empty() ? NULL : devType.c_str() );
      }
      else if ( getFloatArg( req, "playPos", playPos ) )
      {
	setPlayerPlayPos( playPos );
      }
      else if ( getBoolArg( req, "paused", paused ) )
      {
	setPlayerPaused( paused );
      }
      else if ( getStringArg( req, "sensorIN", sensorIN ) )
      {
	std::string fileName( g_SensorINFileName );
	trim(sensorIN);
	if ( sensorIN.empty() )
	  std::remove( g_SensorINFileName.c_str() );
	else
        {
	  std::ofstream stream( fileName );
	  if ( stream.is_open() )
          { if ( g_Verbose )
	      TrackGlobal::info( "writing sensorIN %s to file %s", sensorIN.c_str(), g_SensorINFileName.c_str() );

	    stream << sensorIN;
	    stream.close();
	  }
	}
	
	readSensorIN();
      }
      else if ( getBoolArg( req, "sensorPowerEnabled", sensorPowerEnabled ) )
      {
	std::ofstream stream( "hardware/LidarPower.enable" );
	if ( stream.is_open() )
	  stream << (sensorPowerEnabled?"true":"false");
      }
      else
      {
	if ( getBoolArg( req, "adaptEnv",  adaptEnv ) )
        {
	  for ( int i = ((int)g_Devices.size())-1; i >= 0; --i )
	    g_Devices[i]->doEnvAdaption = adaptEnv;
	}
	if ( getBoolArg( req, "useEnv",  useEnv ) )
        {
	  g_Devices.useEnv( useEnv );
	}
      }

      
      webMutex.unlock();
      
      return stringResponse( "Set" );
    }
};


class register_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "register_resource\n" );

      std::string registerSec  = req.get_arg("sec");
      int sec = 0;
      bool refine = false;
      
      getBoolArg( req, "refine",  refine );
      
      if ( !registerSec.empty() && std::stoi( registerSec ) > 0 )
	sec = std::stoi( registerSec );
      
      webMutex.lock();
      
      if ( sec > 0 )
	g_Devices.registerSec = sec;
      
      g_Devices.startRegistration( refine );
      
      webMutex.unlock();
      
      return stringResponse( "Registrating environment" );
    }
};

class loadRegistration_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "loadRegistration_resource\n" );

      webMutex.lock();
      
      g_Devices.loadRegistration();
      
      webMutex.unlock();
      
      return stringResponse( "Loading Registration" );
    }
};

class saveRegistration_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "saveRegistration_resource\n" );

      webMutex.lock();
      
      g_Devices.saveRegistration( true );
      
      webMutex.unlock();
      
      return stringResponse( "Saving Registration" );
    }
};

class resetRegistration_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "resetRegistration_resource\n" );

      webMutex.lock();
      
      g_Devices.resetRegistration();
      
      webMutex.unlock();
      
      return stringResponse( "Resetting Registration" );
    }
};

class clearMessage_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "clearMessage_resource\n" );

      webMutex.lock();

      g_Devices.message = "";
      
      webMutex.unlock();
      
      return stringResponse( "Clear Message" );
    }
};

class checkpoint_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "checkpoint_resource\n" );

      std::string checkpoint;  
      if ( getStringArg( req, "commit", checkpoint ) )
      {
	uint64_t timestamp = getmsec();

	commitFileToCheckpoint( bluePrintFileName.c_str(), timestamp );
	commitFileToCheckpoint( trackOcclusionMapFileName.c_str(), timestamp );
	commitFileToCheckpoint( "groups.json", timestamp );
	commitFileToCheckpoint( "nikNames.json", timestamp );

	g_Devices.saveEnv( true, timestamp );
	g_Devices.saveRegistration( true, timestamp );
	
	std::string ts( timestampString( "%Y%m%d-%H:%M:%S", timestamp, false ) );
	return stringResponse( ts.c_str() );
      }
      
      return stringResponse( "Clear Message" );
    }
};

class blueprint_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "blueprint_resource\n" );

      bool useHtml = false;

      std::string userAgent( req.get_header("User-Agent") );
 
      if ( userAgent.find("iPhone") != std::string::npos || userAgent.find("Android") != std::string::npos )
	useHtml = true;

      std::string fileName( useHtml && !bluePrintLoResFileName.empty() ?  bluePrintLoResFileName: bluePrintFileName );
//      printf( "fn: %s\n", fileName.c_str() );
      
      bool reload = false;  
      getBoolArg( req, "reload",  reload );
      if ( reload )
      { setBluePrints();
	return stringResponse( "Reload Blueprint" );
      }

      return fileResponse( TrackGlobal::getConfigFileName(fileName.c_str()), bluePrintMimeType.c_str() );
    }
};

class trackoccl_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

      bool reload = false;  
      getBoolArg( req, "reload",  reload );
      if ( reload )
      { setTrackOcclusionMap();
	return stringResponse( "Reload Track Occlusion Map" );
      }
    
//  printf( "trackoccl_resource\n" );

      return fileResponse( TrackGlobal::getConfigFileName(trackOcclusionMapFileName.c_str()), bluePrintMimeType.c_str() );

    }
};

class image_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "image_resource\n" );

      uint64_t sTime = getmsec();
      
//      printf( "requestor: %s\n", req.get_requestor().c_str() );
      
      webMutex.lock();

      LidarPainter &painter( getPainter( req ) );
      
      bool inProgress = imgInProcess;
      if ( inProgress )
      { 
	std::shared_ptr<http_response> response( fileResponse( painter.uiImageFileName, uiMimeType.c_str() ) );

	webMutex.unlock();

	return response;
      }

      imgInProcess = true;
      
      webMutex.unlock();

      uint64_t startTime = getmsec();

      std::string width  = req.get_arg("width");
      std::string height = req.get_arg("height");
      std::string extent = req.get_arg("extent");
      std::string map    = req.get_arg("map");

      bool showCoverage = painter.showCoverage;
      bool showCoveragePoints = painter.showCoveragePoints;

      getBoolArg( req, "showGrid",        painter.showGrid );
      getBoolArg( req, "showPoints",      painter.showPoints );
      getBoolArg( req, "showLines",       painter.showLines );
      getBoolArg( req, "showEnv",         painter.showEnv );
      getBoolArg( req, "showEnvThres",    painter.showEnvThres );
      getBoolArg( req, "showOutline",     painter.showOutline );
      getBoolArg( req, "showCoverage",    showCoverage );
      getBoolArg( req, "showCoverPoints", showCoveragePoints );
      getBoolArg( req, "showObjects",     painter.showObjects );
      getBoolArg( req, "showObjCircle",   painter.showObjCircle );
      getBoolArg( req, "showConfidence",  painter.showConfidence );
      getBoolArg( req, "showCurvature",   painter.showCurvature );
      getBoolArg( req, "showSplitProb",   painter.showSplitProb );
      getBoolArg( req, "showLifeSpan",    painter.showLifeSpan );
      getBoolArg( req, "showMotion",      painter.showMotion );
      getBoolArg( req, "showMotionPred",  painter.showMotionPred );
      getBoolArg( req, "showMarker",      painter.showMarker );
      getBoolArg( req, "showDevLocation", painter.showDevices );
      getBoolArg( req, "showDeviceInfo",  painter.showDeviceInfo );
      getBoolArg( req, "showObsvStat",    painter.showObserverStatus );
      getBoolArg( req, "showTracking",    painter.showTracking );
      getBoolArg( req, "showRegions",     painter.showRegions );
      getBoolArg( req, "showStages",      painter.showStages );
      getBoolArg( req, "showObstacles",   painter.showObstacles );
      getBoolArg( req, "showPrivate",     painter.showPrivate );
      getBoolArg( req, "showControls",    painter.showControls );

      bool coverageChanged = (painter.showCoverage != showCoverage || painter.showCoveragePoints != showCoveragePoints);
      bool coverage        = (showCoverage || showCoveragePoints);

      painter.showCoverage       = showCoverage;
      painter.showCoveragePoints = showCoveragePoints;

      if ( coverageChanged )
	g_Devices.setUseOutEnv( !coverage );

      if ( !width.empty() && std::stoi( width ) != 0 )
	painter.width = painter.canv_width = std::stoi( width );
      if ( !height.empty() && std::stoi( height ) != 0 )
	painter.height = painter.canv_height = std::stoi( height );
      if ( !extent.empty() && std::stof( extent ) != 0 )
	painter.extent = std::stof( extent );

	  /* do not adapt image size

      float weight = sqrt( computeWeight );
 
//      printf( " weight: %g\n", weight );

      static float    lastWeight = 1.0;
      static uint64_t updateTime = 0;
      static const float step = 0.06;

      bool timedUpdate = (startTime - updateTime > 2000); // update every 2 sec

      if ( weight >= 0.999 || weight > lastWeight+step )
      { 
//	printf( " weight2: %g\n", weight );

	if ( timedUpdate )
	{ lastWeight = weight;
	  updateTime = startTime;
	}
      }
      else
      { 
	float s = (timedUpdate ? step : 3*step);
	
	if ( weight < lastWeight-s )
        { lastWeight = weight;
	  updateTime = startTime;
	}
      }
      
      weight = floor( lastWeight/step + 0.45 ) * step;
      if ( weight > 1.0 )
	weight = 1.0;
      
	  //     printf( "draw weight: %g  (%g)\n", lastWeight, weight );

      painter.width  *= weight;
      painter.height *= weight;
	  */

	  // resize only if blueprint is not visible
      if ( bluePrintFileName.empty() )
      {
	const int maxSize = 1200;
	const int maxArea = maxSize * maxSize;
      
	float A = painter.width * painter.height;
	if ( A > maxArea )
        {
	  float a = sqrt( A/maxArea );
	  
	  painter.width  /= a;
	  painter.height /= a;
	}
      }
      
//      printf( "width: %d %d\n", painter.width, painter.height );    
      painter.begin();
      
      if ( !map.empty() )
      { 
	if ( map == "heatmap" )
	  paintImageObserver( painter, TrackableObserver::HeatMap );
	else if ( map == "flowmap" )
	  paintImageObserver( painter, TrackableObserver::FlowMap );
	else if ( map == "tracemap" )
	  paintImageObserver( painter, TrackableObserver::TraceMap );
	painter.viewUpdated = false;
      }
      
      if ( painter.showGrid )
      { painter.paintGrid();
	painter.paintAxis();
      }

       LidarDeviceList &devices( g_Devices.activeDevices() );

      bool lock = !g_Devices.isCalculating;

      for ( int i = ((int)devices.size())-1; i >= 0; --i )
      {
	char show[100];
	sprintf( show,  "showDevice%d",  i );
	getBoolArg( req, show, g_DeviceUI[i].show  );
	
	if ( g_DeviceUI[i].show )
	{ LidarDevice &device( *devices[i] );
	  painter.paintCoverage( device );
	  painter.paintEnv( device );
	}
      }

      for ( int i = ((int)devices.size())-1; i >= 0; --i )
      { LidarDevice &device( *devices[i] );
	if ( device.isOpen(lock) )
        { if ( g_DeviceUI[i].show )
	    painter.paint( device );
	}
      }
 
      if ( g_UseObstacle && painter.showObstacles )
	painter.paintObstacles();

      if ( painter.showDevices )
      {	for ( int i = ((int)g_Devices.size())-1; i >= 0; --i )
	  //	  if ( g_DeviceUI[i].show )
          {
//	    if ( g_Devices[i]->isOpen(lock) )
	      painter.paintDevice( *g_Devices[i] );
	  }
      }

      const int indent = 5;
      
      if ( (painter.showTracking || painter.showStages) && g_Track.m_Stage != NULL )
      { 
	g_TrackMutex.lock();
	painter.paintMultiStage( *g_Track.m_Stage, painter.showTracking, painter.showStages, sizeof(colorArray)/sizeof(colorArray[0])-1, true );
	g_TrackMutex.unlock();
     }

      if ( painter.showRegions )
	painter.paint( TrackGlobal::regions );

      LidarDevice *scanDevice = NULL;
      for ( int d = 0; d < devices.size(); ++d )
	if ( devices[d]->isEnvScanning )
	  scanDevice = devices[d];

      if ( g_Devices.isRegistering )
      {
	uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	uint64_t milliSec    = currentTime - g_Devices.startTime;
	int secToGo = ceil( ((g_Devices.registerSec * 1000) - milliSec) / 1000.0 );
	std::string msg( "registration in progress: " );
	msg += std::to_string( secToGo );
	painter.img->draw_text( indent, 12, msg.c_str(), white, 0, 1, 15 );
      }
      else if ( !g_Devices.message.empty())
	painter.img->draw_text( indent, 12, g_Devices.message.c_str(), white, 0, 1, 15 );
      else if ( scanDevice != NULL )
      {
	uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	uint64_t milliSec    = currentTime - scanDevice->processStartTime;
	int secToGo = ceil( ((scanDevice->envScanSec * 1000) - milliSec) / 1000.0 );
	std::string msg( "scanning environment: " );
	msg += std::to_string( secToGo );
	painter.img->draw_text( indent, 12, msg.c_str(), white, 0, 1, 15 );
      }
      else if ( !g_Devices.isCalculating )
      {
	char text[1024];
	
	int64_t playTime = playerCurrentTime();
	if ( playTime >= 0 )
        { std::string date( "%a %d %b %Y %H:%M:%S" );
	  date = applyDateToString( date.c_str(), playerTimeStamp() );

	  char time[1000];
	  if ( painter.showControls )
          { sprintf( time, "%s (%02ld:%02ld:%02ld:%02ld)", date.c_str(), playTime/(3600*1000), (playTime/(60*1000))% 60, (playTime/1000)% 60, (playTime/10)%100 );
	    painter.img->draw_text( painter.img->width()-160-160, painter.img->height()-20, time, white, black, 1.0, 15 );
	  }
	  else
          { sprintf( time, "%s", date.c_str() );
	    painter.img->draw_text( painter.img->width()-160-260, 10, time, white, black, 1.0, 38 );
	  }
	}

	sprintf( text, "frame rate: %d", frameRate.fps );
	painter.img->draw_text( indent+5, painter.img->height()-27, text, white, black, 1.0, 15 );

	if ( painter.showObserverStatus && g_Track.m_Stage != NULL && g_Track.m_Stage->observer != NULL )
        {
	  int fontSize   = 14;
	  int fontWidth  = 7;
	  int lineOffset = 13;
	
	  int ix = painter.img->width() - indent - 4*fontWidth;
	  int d = 0;

	  TrackableMultiObserver &multi( *g_Track.m_Stage->observer );
	  for ( int i = 0; i < multi.observer.size(); ++i )
          { TrackableObserver &observer( *multi.observer[i] );
	    if ( !observer.statusMsg.empty() )
            { rpImg img;
	      img.draw_text( 0, 0, observer.statusMsg.c_str(), white, black, 1.0, fontSize );
	      painter.img->draw_text( ix - img.width(), 12 + d++ * lineOffset, observer.statusMsg.c_str(), white, black, 1.0, fontSize );
	    }

	    if ( observer.showSwitchStatus )
	    {
	      for ( int i = observer.rects.numRects()-1; i >= 0; --i )
	      {
		ObsvRect &rect( observer.rects.rect(i) );
		ObsvObjects &objects( rect.objects );
		bool switchVal = objects.validCount;
		std::string statusMsg( "["+observer.name+":"+rect.name+"] " );
		if ( switchVal )
		  statusMsg += " on";
		else
		  statusMsg += " off";

		rpImg img;
		img.draw_text( 0, 0, statusMsg.c_str(), white, black, 1.0, fontSize );
		painter.img->draw_text( ix - img.width(), 12 + d++ * lineOffset, statusMsg.c_str(), white, black, 1.0, fontSize );
	      }
	    }

	    if ( observer.showCountStatus )
	    {
	      for ( int i = observer.rects.numRects()-1; i >= 0; --i )
	      {
		ObsvRect &rect( observer.rects.rect(i) );
		ObsvObjects &objects( rect.objects );
		std::string statusMsg( "["+observer.name+":"+rect.name+"] " );
		statusMsg += std::to_string( objects.validCount );
		rpImg img;
		img.draw_text( 0, 0, statusMsg.c_str(), white, black, 1.0, fontSize );
		painter.img->draw_text( ix - img.width(), 12 + d++ * lineOffset, statusMsg.c_str(), white, black, 1.0, fontSize );
	      }
	    }
	  }
	}

	if ( painter.showDeviceInfo && TrackBase::packedPlayer() == NULL )
        {
	  int fontSize   = 14;
	  int lineOffset = 13;
	
	  while ( lineOffset >= 7 && lineOffset * devices.size() > painter.height-13 )
          { fontSize   -= 1;
	    lineOffset -= 1;
	  }

	  uint64_t now = getmsec();

	  for ( int d = 0; d < devices.size(); ++d )
          { if ( g_DeviceUI[d].show )
            {
	      LidarDevice &device( *devices[d] );
	      std::string deviceType( device.info.detectedDeviceType );
	      if ( deviceType.empty() )
              { if ( !device.deviceType.empty() )
		  deviceType = device.deviceType;
		else
		  deviceType = "unknown";
	      }
	  
	      std::string sid;
	      if ( !device.sensorIN.empty() )
              { sid  = " [SIN=";
		sid += device.sensorIN;
		sid += "]";
	      }

	      bool hasPowerSupport = device.devicePoweringSupported();
	      const char *poweringSupported = (hasPowerSupport?"P":"p");

	      sprintf( text, "%d: %s (%s,%s)%s ", d+1, device.getNikName().c_str(), deviceType.c_str(), poweringSupported, sid.c_str() );

	      uint64_t timeDiff = now - device.openTime;
	      uint64_t recvDiff = (now < device.receivedTime ? 0 : now - device.receivedTime);

	      const unsigned char *back = midGray;

	      if ( device.isOpen() )
	      {
		char tmp[1024] = "";
		LidarDevice::Info info;
  
		if ( device.isPoweringUp )
                { sprintf( tmp, "powering up\n" );
		  back = darkerYellow;
		}
		else if ( !device.dataReceived || recvDiff > g_WarningReportMSec )
                { sprintf( tmp, "no data" );
		  back = (device.isReady() && timeDiff/1000 > g_FailureReportSec ? red : darkerYellow);
		}
		else if ( device.isReady() && device.getInfo( info ) )
	        { sprintf( tmp, "fps=%d samples=%d", info.average_fps.fps, info.average_samples.average() );
		  back = darkGreen;
		}
		else
                { sprintf( tmp, "undefined status" );
		  back = red;
		}

		strcat( text, tmp );
	      }
	      else
	      {
		if ( !device.errorMsg.empty() )
                { strcat( text, device.errorMsg.c_str() );
		  back = red;
		}
		else
                {
		  strcat( text, "stopped" );
		  if ( !hasPowerSupport )
		    back = grayGreen;
		}
	      }

	      bool failed = false;
	      if ( g_DeviceFailed.get( device.getNikName().c_str(), failed ) && failed )
		back = red;

	      if ( !device.outVirtUrl.empty() )
              { strcat( text, " > virtual:" );
		strcat( text, device.outVirtUrl.c_str() );
	      }
	    
	      unsigned char devColor[4];
	      deviceColor( device.deviceId, devColor );

	      const int radius = 4;

	      painter.img->draw_circle( indent+radius, (d+1) * lineOffset + 6, radius, back );
	      painter.img->draw_text( indent+5 + 2*radius, (d+1) * lineOffset, text, devColor, black, 1.0, fontSize );
	    }
	  }
	}
      }

      painter.end();

//      printf( "msec: %d\n", frameTimeAverage );

      webMutex.lock();

	  /*
      if ( colChannels > 3 )
      {
	rpImg &img( *painter.img );

	if ( img.spectrum() > 3 )
        { for ( int y = img.height()-1; y >= 0; --y )
          { for ( int x = img.width()-1; x >= 0; --x )
            { unsigned char alpha = img( x, y, 0, 3 );
	      if ( alpha > 0 )
		img( x, y, 0, 3 ) = 255;
	      else if ( img.depth() > 3 )
		img( x, y, 0, 3 ) = 128;
	      else
		
		img( x, y, 0, 3 ) = ;
	    }
	  }
        }
      }
	  */
      painter.img->save( painter.uiImageFileName.c_str() );

      uint64_t endTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
      addFrameTime( startTime, endTime );

      std::shared_ptr<http_response> response( fileResponse( painter.uiImageFileName, uiMimeType.c_str() ) );

      imgInProcess = false;
      webMutex.unlock();

      return response;
    }
};

class map_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "map_resource\n" );

      std::string name;
      
      if ( !getStringArg( req, "name",  name ) )
	name = "heatmap";

      std::string fileName( name + ".jpg" );
	
      TrackableImageObserver *imageObserver = static_cast<TrackableImageObserver*>(g_Track.m_Stage->getObserver( name.c_str() ));
      if ( imageObserver != NULL )
      {
	webMutex.lock();
      
	rgbImg img( imageObserver->calcImage() );
	
	img.save( fileName.c_str() );

	webMutex.unlock();
      }

      return fileResponse( fileName, "image/jpg" );
    }
  
};

class move_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "move_resource\n" );

      static std::string refDevice;
      
      std::string ref     = req.get_arg("ref");
      if ( !ref.empty() )
      { refDevice = ref;
	return std::shared_ptr<http_response>(new string_response("Move"));
      }

      std::string dxs     = req.get_arg("dx");
      std::string dys     = req.get_arg("dy");
      std::string radius  = req.get_arg("radius");
      std::string control = req.get_arg("control");
      std::string reset   = req.get_arg("reset");
      std::string isDown  = req.get_arg("isDown");
      int dx = (dxs.empty() ? 0 :  std::stoi( dxs ));
      int dy = (dys.empty() ? 0 : -std::stoi( dys ));

      if ( (dx == 0 && dy == 0) && reset != "true" && isDown != "false" )
	return std::shared_ptr<http_response>(new string_response("Move"));
      
      webMutex.lock();
      
      LidarPainter &painter( getPainter( req ) );
      
      float dcx = dx / (float)painter.width  * painter.extent_x;
      float dcy = dy / (float)painter.height * painter.extent_y;
      
      if ( isnan(dcx) || isnan(dcy) )
	return std::shared_ptr<http_response>(new string_response("Move"));

      if ( control == "camera" )
      {
	if ( isDown == "false" )
        {
	}
        else if ( reset == "true" )
        {
	  painter.extent = 10;
	  painter.matrix.id();
	  painter.matrixInv.id();
	  painter.viewUpdated = true;
	}
        else if ( radius.empty() )
        {
	  painter.matrix.w.x += dcx;
	  painter.matrix.w.y += dcy;

	  painter.matrixInv = painter.matrix.inverse();

	  painter.viewUpdated = true;
	}
      }
      else if ( control == "blueprint" )
      {
	if ( isDown == "false" )
        {
	  if ( g_Devices.isSimulationMode() )
	  { LidarDeviceList devices( g_Devices.activeDevices() );
	    createSimulationEnvMaps( devices );
	  }
	}
        else if ( reset == "true" )
        {
	  bpMatrix.id();
	  bpMatrixInv.id();
	}
        else if ( radius.empty() )
        {
	  bpMatrix.w.x += dcx;
	  bpMatrix.w.y += dcy;

	  bpMatrixInv = bpMatrix.inverse();
	}

	blueprints.set( "x", std::to_string(bpMatrix.w.x).c_str() );
	blueprints.set( "y", std::to_string(bpMatrix.w.y).c_str() );
      }
      else if ( control == "obstacle" )
      {
	if ( isDown == "false" )
        {
	}
        else if ( reset == "true" )
        {
	  obsMatrix.id();
	  obsMatrixInv.id();
	}
        else if ( radius.empty() )
        {
	  obsMatrix.w.x += dcx;
	  obsMatrix.w.y += dcy;

	  obsMatrixInv = obsMatrix.inverse();
	}
        else
        {
	  Matrix3H &matrix( obsMatrix );

	  float r = std::stof( radius );
	  
	  if ( r > 1 )
	  { float sin   = dy / r;
	    if ( sin < -1.0 )
	      sin = -1.0;
	    else if ( sin > 1.0 )
	      sin = 1.0;
	    float angle = -asin( sin );
	    Matrix3H rotMatrix( rotZMatrix( angle ) );

	    float wx = matrix.w.x;
	    float wy = matrix.w.y;	    
	    matrix.w.x = 0;
	    matrix.w.y = 0;
	    
	    matrix = rotMatrix * matrix;

	    matrix.w.x = wx;
	    matrix.w.y = wy;

	    obsMatrixInv = matrix.inverse();
	  }
	}
      }
      else 
      {
	LidarDeviceList devices( g_Devices.activeDevices() );

	int d = -1;
	if ( control != "world" ) // moveDeviceX
	{ d = std::atoi( &control[control.length()-1] );
	  if ( d < 0 || d >= devices.size() )
	    d = -1;
	}

	int count  = 0;
	float refX = 0;
	float refY = 0;

	for ( int i = ((int)devices.size())-1; i >= 0; --i )
        { LidarDevice &device( *devices[i] );
	  bool move = false;
	  if ( control == "world" || getBoolArg( req, device.getNikName().c_str(), move ) && move )
	  { refX = device.viewMatrix.w.x;
	    refY = device.viewMatrix.w.y;
	    count += 1;
	  }
	}

	if ( count > 1 )
	{ refX = 0;
	  refY = 0;
	}
	
	painter.viewUpdated = true;

	for ( int i = ((int)devices.size())-1; i >= 0; --i )
        {
	  LidarDevice &device( *devices[i] );
	  bool move = false;
	  if ( control == "world" || getBoolArg( req, device.getNikName().c_str(), move ) && move  )
          {
	    Matrix3H matrix( device.viewMatrix );

	    if ( isDown == "false" )
            {
	      if ( g_Devices.isSimulationMode() )
		createSimulationEnvMap( device );
	    }
	    else if ( reset == "true" )
            {
	      matrix.id();
	      device.setViewMatrix( matrix );
	    }
	    else if ( radius.empty() )
            {
	      Matrix3H rot( painter.matrix );
	      rot.w.x = 0;
	      rot.w.y = 0;
	      rot = rot.transpose();
	
	      Vector3D p( rot * Vector3D( dcx, dcy, 0.0 ) );
	      matrix.w += p;

	      device.setViewMatrix( matrix );
	    }
	    else
            {
	      float r = std::stof( radius );
	      if ( r > 1 )
	      { float sin   = dy / r;
		if ( sin < -1.0 )
		  sin = -1.0;
		else if ( sin > 1.0 )
		  sin = 1.0;
		float angle = -asin( sin );
		Matrix3H rotMatrix( rotZMatrix( angle ) );
		
		matrix.w.x -= refX;
		matrix.w.y -= refY;

		matrix = rotMatrix * matrix;
		
		matrix.w.x += refX;
		matrix.w.y += refY;

		device.setViewMatrix( matrix );
	      }
	    }
	  }
	}
     }
      
      webMutex.unlock();
      
      return stringResponse( "Move" );
    }
};

class changeExtent_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render(const http_request& req) {

//  printf( "changeExtent_resource\n" );

//      g_Devices.saveRegistration();

      std::string dys     = req.get_arg("dy");
      if ( !dys.empty() )
      {
	int dy = std::stoi( dys );
      
	webMutex.lock();
      
	LidarPainter &painter( getPainter( req ) );
      
	float dcy = dy / (float)painter.height * painter.extent_y;
      
	painter.extent += dcy;

	if ( painter.extent < 1 )
	  painter.extent = 1;

//      printf( "extent: %g %g\n", painter.extent, dcy );
	painter.viewUpdated = true;
	painter.updateExtent();  

	webMutex.unlock();
      }
      
      std::string dss     = req.get_arg("ds");
      if ( !dss.empty() )
      {
	float ds = -std::stof( dss );
      
	webMutex.lock();
      
	LidarPainter &painter( getPainter( req ) );
      
	float dcy = ds * painter.extent_y;
      
	painter.extent += dcy;

	if ( painter.extent < 1 )
	  painter.extent = 1;

//      printf( "extent: %g %g\n", painter.extent, dcy );
	painter.viewUpdated = true;
	painter.updateExtent();  

	webMutex.unlock();
      }
      
      return stringResponse( "Change Extent" );
    }
};

class stats_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render_GET(const http_request& req) {

      //  printf( "stats_resource\n" );

      std::string path("." );
      path += req.get_path();

      std::string date    = req.get_arg("date");
      
      if ( !date.empty() )
      {
	std::string fileName( LidarDevice::configDir );
	fileName += "stats/2022/";
	fileName += date;
	fileName += "/stats_";
	fileName += date;
	fileName += ".json";

	return fileResponse( fileName.c_str(), "application/json" );
      }
      
      return stringResponse("File not Found", "text/plain", 404 );
    }
};

class html_resource : public http_resource {
public:
    render_const std::shared_ptr<http_response> render_GET(const http_request& req) {

      //  printf( "html_resource\n" );

      std::string path("." );
      path += req.get_path();

//     printf( "Path: %s\n", path.c_str() );

      if ( endsWithCaseInsensitive( path, "/title.html" ) )
      {
	char hostname[HOST_NAME_MAX+1];
	gethostname(hostname, HOST_NAME_MAX+1);
	
	std::string result( std::string(getIP()) + " (" + hostname + ")"  );
	
	if ( getMAC() != NULL )
        {
	  result += " [";
	  result += getMAC();
	  result += "]";
	}

//	printf( "Path1: %s result='%s'\n", path.c_str(), result.c_str() );

	return stringResponse( result.c_str() );
      }
      else if ( endsWithCaseInsensitive( path, "/availableGroups.html" ) )
      {
	webMutex.lock();
      
	std::string result;
	result += "<li id=\"availableGroups\">\n";

	for ( KeyValueMapDB::iterator dbiter( LidarDeviceGroup::groups.begin() ); dbiter != LidarDeviceGroup::groups.end(); dbiter++ )
        {
	  KeyValueMap &map( dbiter->second );

	  std::string groupName( dbiter->first.c_str() );

	  result += "	  <div class=\"dropdown-item\">\n"
	    "	    <input type=\"checkbox\" class=\"form-check-input me-1\" id=\"avGroup_";
	  result += groupName;
	  result += "\" name=\"";
	  result += groupName;
	  result += "\"";

	  if ( g_Devices.isActive(groupName.c_str()) )
	    result += " checked=\"checked\"";

   	  result += ">\n"
	    "	    <label class=\"custom-control-label\" for=\"avGroup_";
	  result += groupName;
	  result += "\">";
	  result += groupName;
	  result += "</label>\n"
"	  </div>\n";
	}
	result += "</li>\n";

	webMutex.unlock();
      
	return stringResponse( result.c_str() );
      }
      else if ( endsWithCaseInsensitive( path, "/editRegions.html" ) )
      {
	webMutex.lock();
      
	std::vector<TrackableRegion*> regions;
	if ( TrackGlobal::regions.layers.size() == 0 )
        { for ( int i = 0; i < TrackGlobal::regions.size(); ++i )
	    regions.push_back( &TrackGlobal::regions[i] );
	}
	else
        {
	  LidarPainter &painter( getPainter( req ) );

	  for ( int i = 0; i < TrackGlobal::regions.size(); ++i )
          { for ( auto &layer: painter.layers )
            { if ( TrackGlobal::regions[i].hasLayer( layer ) )
              { regions.push_back( &TrackGlobal::regions[i] );
		break;
	      }
	    }
	  }
	}

	std::string result;
	result += "<div id=\"editRegs\">\n";
	
	for ( int r = 0; r < regions.size(); ++r )
	{
	  result += "	  <li><a class=\"dropdown-item\" href=\"#\" name=\"";
	  result += regions[r]->name;
	  result += "\" id=\"editRegion";
	  result += std::to_string( r );
	  result += "\">Edit ";
	  result += regions[r]->name;
	  result += "</a></li>\n";
	}
	result += "</div>\n";

	webMutex.unlock();
      
	return stringResponse( result.c_str() );
      }	
      else if ( endsWithCaseInsensitive( path, "/layers.html" ) )
      {
	std::string result;
	result += "<div id=\"layers\"";
	if ( TrackGlobal::regions.layers.size() == 0 )
	  result += " hidden=\"true\"";
	result += ">\n";
	
	result += "<li><div class=\"dropdown-item\"><button class=\"btn\" id=\"showAllLayers\">All</button></div></li>"
	  "<li><div class=\"dropdown-item\"><button class=\"btn\" id=\"showNoneLayers\">None</button></div></li>"
	  "<li><hr class=\"dropdown-divider\"></li>";

	webMutex.lock();
      
	LidarPainter &painter( getPainter( req ) );

	for ( auto layer: painter.layers )
        { 
	  if ( layer.empty() )
	    layer = "No Layer";

	  std::string id( "showLayer" );

	  id += layer;

	  replace( id, " ", "_");

	  addCheckedButton( result, id.c_str(), layer.c_str(), true );
	}
	
	result += "</div>\n";

	webMutex.unlock();
      
	return stringResponse( result.c_str() );
      }	
      else if ( endsWithCaseInsensitive( path, "/runDevices.html" ) )
      {
	webMutex.lock();
      
	std::string result;

	LidarDeviceList &devices( g_Devices.activeDevices() );

	addMenu( result, devices, "runDevices", "runDevice", "runGroup" );

	webMutex.unlock();
      
	return stringResponse( result.c_str() );
      }
      else if ( endsWithCaseInsensitive( path, "/rebootNodes.html" ) )
      {
	webMutex.lock();
      
	std::string result;

	LidarDeviceList devices( g_Devices.remoteDevices() );

	addButtonMenu( result, devices, "rebootNodes", "rebootNode", "rebootGroup" );

	webMutex.unlock();
      
	return stringResponse( result.c_str() );
      }
      else if ( endsWithCaseInsensitive( path, "/movableDevices.html" ) )
      {
	webMutex.lock();
      
	std::string result;

	g_SubMenuLeft = true;
	LidarDeviceList &devices( g_Devices.activeDevices() );
	addMenu( result, devices, "movableDevices", "moveDevice", "moveGroup" );
	g_SubMenuLeft = false;

//	addUIForDevices( result, devices, "movableDevices", "moveDevice", false );

	webMutex.unlock();
      
	return stringResponse( result.c_str() );
      }
      else if ( endsWithCaseInsensitive( path, "/visibleDevices.html" ) )
      {
	webMutex.lock();
      
	std::string result;

	g_SubMenuLeft = true;
	LidarDeviceList &devices( g_Devices.activeDevices() );
	addMenu( result, devices, "visibleDevices", "showDevice", "showGroup" );
	g_SubMenuLeft = false;

//	addUIForDevices( result, devices, "visibleDevices", "showDevice", true );

	webMutex.unlock();
      
	return stringResponse( result.c_str() );
      }
      else if ( endsWithCaseInsensitive( path, "/displayOptions.html" ) )
      {
	std::string result;
	
	webMutex.lock();

	std::vector<std::string> mapName = { "heatmap", "flowmap", "tracemap" };
	
	for ( int i = 0; i < mapName.size(); ++i )
        {
	  if ( hasObserverOfType( i == 0 ? TrackableObserver::HeatMap : (i == 1 ? TrackableObserver::FlowMap : TrackableObserver::TraceMap ) ) )
	  {
	    const char *name = mapName[i].c_str();
	  
	    result += "	  <div class=\"dropdown-item\">\n"
	      "	    <input type=\"checkbox\" class=\"form-check-input me-1\" id=\"map_";
	    result += name;
	    result += "\" name=\"";
	    result += name;
	    result += "\">\n"
		        "	    <label class=\"custom-control-label\" for=\"map_";
	    result += name;
	    result += "\">";
	    result += name;
	    result += "</label>\n"
	      "	  </div>\n";
	  }
	}

	webMutex.unlock();
      
	return stringResponse( result.c_str() );
      }
      else if ( path == "./" || path == "./index.html" )
      {
	std::shared_ptr<http_response> response( fileResponse( "index.html", "text/html" ) );

	std::string lidartool( "lidartool" );
	std::string cookie( std::to_string(getmsec()&0xffffffff) );
	response->with_cookie( lidartool, cookie );
	
	return response;
      }
      else if ( path == "./settings" )
	return fileResponse( "settings.html", "text/html" );
      else  if ( endsWithCaseInsensitive( path, ".html" ) )
	return fileResponse( path, "text/html" );
      else if ( endsWithCaseInsensitive( path, ".js" ) )
	return fileResponse( path, "text/javascript" );
      else if ( endsWithCaseInsensitive( path, ".json" ) )
	return fileResponse( path, "application/json" );
      else if ( endsWithCaseInsensitive( path, ".css" ) )
	return fileResponse( path, "text/css" );
      else if ( fileExists( path ) )
      {
	if ( endsWithCaseInsensitive( path, ".jpg" ) ||
	     endsWithCaseInsensitive( path, ".jpeg" ) )
	  return fileResponse( path, "image/jpeg" );
	else if ( endsWithCaseInsensitive( path, ".png" ) )
	  return fileResponse( path, "image/png" );

	return fileResponse( path, "tex/plain" );
      }
      else if ( endsWithCaseInsensitive( path, ".jpg" ) ||
		endsWithCaseInsensitive( path, ".jpeg" ) )
	return fileResponse( path, "image/jpeg" );

      return stringResponse("File not Found", "text/plain", 404 );
    }
};

static start_resource   start_r;
static reopen_resource reopen_r;
static stop_resource    stop_r;
static run_resource     run_r;
static reboot_resource  reboot_r;
static lastErrors_resource lastErrors_r;
static sensorIN_resource sensorIN_r;
static scanEnv_resource scanEnv_r;
static resetEnv_resource resetEnv_r;
static loadEnv_resource loadEnv_r;
static saveEnv_resource saveEnv_r;
static regions_resource regions_r;
static loadRegions_resource loadRegions_r;
static saveRegions_resource saveRegions_r;
static saveBlueprint_resource saveBlueprint_r;
static deviceList_resource deviceList_r;
static status_resource status_r;
static get_resource get_r;
static set_resource set_r;
static clearMessage_resource clearMessage_r;
static register_resource register_r;
static loadRegistration_resource loadRegistration_r;
static saveRegistration_resource saveRegistration_r;
static resetRegistration_resource resetRegistration_r;
static move_resource move_r;
static checkpoint_resource checkpoint_r;
static changeExtent_resource changeExtent_r;
static image_resource   image_r;
static blueprint_resource blueprint_r;
static trackoccl_resource trackoccl_r;
static map_resource     map_r;
static stats_resource   stats_r;
static html_resource    html_r;

static void
runWebServer()
{
  webId = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  
  const int max_threads = 20;
  webserv = new webserver(create_webserver(webserver_port).max_threads(max_threads));

  webserv->register_resource("/start", &start_r);
  webserv->register_resource("/startSensors", &start_r);
  webserv->register_resource("/reopen", &reopen_r);
  webserv->register_resource("/restartSensors", &reopen_r);
  webserv->register_resource("/stop",  &stop_r);
  webserv->register_resource("/stopSensors",  &stop_r);
  webserv->register_resource("/run",  &run_r);
  webserv->register_resource("/reboot",  &reboot_r);

  webserv->register_resource("/scanEnv",  &scanEnv_r);
  webserv->register_resource("/resetEnv", &resetEnv_r);
  webserv->register_resource("/loadEnv",  &loadEnv_r);
  webserv->register_resource("/saveEnv",  &saveEnv_r);
  webserv->register_resource("/clearMessage",  &clearMessage_r);

  webserv->register_resource("/regions",      &regions_r);
  webserv->register_resource("/loadRegions",  &loadRegions_r);
  webserv->register_resource("/saveRegions",  &saveRegions_r);

  webserv->register_resource("/saveBlueprint",  &saveBlueprint_r);

  webserv->register_resource("/deviceList",  &deviceList_r);
  webserv->register_resource("/status",  &status_r);
  webserv->register_resource("/get",  &get_r);
  webserv->register_resource("/set",  &set_r);

  webserv->register_resource("/register",          &register_r);
  webserv->register_resource("/loadRegistration",  &loadRegistration_r);
  webserv->register_resource("/saveRegistration",  &saveRegistration_r);
  webserv->register_resource("/resetRegistration", &resetRegistration_r);

  webserv->register_resource("/checkpoint",  &checkpoint_r);

  webserv->register_resource("/move",  &move_r);
  webserv->register_resource("/changeExtent",  &changeExtent_r);

  webserv->register_resource("/image", &image_r);
  webserv->register_resource("/blueprint", &blueprint_r);
  webserv->register_resource("/trackoccl", &trackoccl_r);
  webserv->register_resource("/map",   &map_r);

  webserv->register_resource("/{*.html}", &html_r);
  webserv->register_resource("/settings", &html_r);
  webserv->register_resource("/stats", &stats_r);
  webserv->register_resource("/lastErrors", &lastErrors_r);
  webserv->register_resource("/sensorIN", &sensorIN_r);
  webserv->register_resource("/", &html_r);

  webserv->start(false);
}

/***************************************************************************
*** 
*** Main
***
****************************************************************************/

static LidarDevice *dummyDevice = NULL;
static LidarDevice *Device = NULL;
static LidarDevice *device()
{
  if ( Device == NULL )
  {
    Device = new LidarDevice();

    g_Devices.push_back( Device );
    g_DeviceUI.push_back( DeviceUI() );
  }
  
  return Device;
}

static void exit_handler(void)
{
  g_Track.exit();
}

static void printTrackingHelp( int argc, const char *argv[] )
{
  printf( "\nTRACKING:\n" );
  printf( " +track\t\t\t\t\t\tswitch on tracking (the default is off)\n" );

  printf( "\ntracking parameters:\n" );
  g_Track.m_Stage->printArgHelp();
/*
  printf( "\nsending tracking data:\n" );
  printf( " +out target\t\t\tsend the detected blobs to the given target\n" );
  printf( "  osc://hostname:port\t\tsends blobs as OSC messages\n" );
  printf( "  json.udp://hostname:port\tsends blobs as json messages via UDP protocol\n" );
  printf( "  json.tcp://hostname:port\tsends blobs as json messages via TCP protocol\n" );

  printf( "\n +filter [:filter1,filter2,...]\tblob output with filters\n" );

  g_Track.blobFilter = Filter::BlobFilter();
  g_Track.blobFilter.printFilterHelp( "track" );
*/
}

static void printObserverHelp( int argc, const char *argv[] )
{
  printf( "\nOBSERVER MANAGEMENT BY CONFIGURATION FILE %s:\n", TrackGlobal::observerFileName.c_str() );
  printf( " +listObservers\t\tlist existing observers\n" );
  printf( " +setObserverValues \tname [:filter] @parameter=value ...\tset observer values. creates the  observerif it does not exist\n" );
  printf( " +renameObserver \toldName newName\t\t\trename existing observer\n" );
  printf( " +removeObserver \tobserverName\t\tdeletes an existing observer\n" );
  printf( " +removeObserverValue \tobserverName parameterName\t\tdelete a single observer parameter\n" );

  printf( "\nobserver usage:\n" );
  printf( " +useObserver\t\tname\tswitch on usage of observer with name 'name' defined in the configuration file. if name is 'all', all observers are used\n" );
  printf( " +useObservers\t\t\tswitch on usage of all observers defined in the configuration file. Same as +useObserver all\n" );

  printf( "\nOBSERVER DEFINITION IN COMMAND LINE:\n" );
  printf( " +observer\t\t[:filter] @{type=type,name=name,param1=value1,...}\t\tdefinition and usage of observer\n" );
  printf( "  available types:\tfile\t\twrites log information into a file\n" );
  printf( "\t\t\tbash\t\tcalls a bash script if the occupation or number of people in a region changes\n" );
#if HAS_LIBLO
  printf( "\t\t\tosc\t\tsends osc messages to a server\n" );
#endif
#if HAS_MOSQUITTO
  printf( "\t\t\tmqtt\t\tpublishes infos to an MQTT server\n" );
#endif
#if USE_WEBSOCKETS
  printf( "\t\t\twebsocket\topens a websocket port as server\n" );
#endif
#if HAS_LUA
  printf( "\t\t\tlua\t\truns lua scripts\n" );
#endif
#if HAS_INFLUXDB
  printf( "\t\t\tinfluxdb\tpublishes infos to an influxdb server\n" );
#endif
  printf( "\t\t\theatmap\t\twrites heatmap images to the file system\n" );
  printf( "\t\t\tflowmap\t\twrites flowmap images to the file system\n" );
}

static void printRegionsHelp( int argc, const char *argv[] )
{
  printf( "\nREGIONS management in configuration file:\n" );
  printf( " +listRegions\t\t\t\tlist existing regions\n" );
  printf( " +setRegion  \t\tname [@x=PosX] [@y=PosY] [@width=Width] [@height=Height] [@shape=Rectangle|Ellipse] [@tags=tag1,tag2,...] [@layers=layer1,layer2,...] \tset region parameter. Creates the region if it does not exist.\n" );
//  printf( " +setRegionEdge \tname edge\tset region edge. edge can be one out of none, Left, Right, Top, Bottom\n" );
//  printf( " +setRegionShape \tname shape\tset region shape. shape can be one out of Rectangle or Ellipse\n" );
//  printf( " +setRegionTags \tname tags\tset region tags. tags is a comma separated list (no spaces) of tag names\n" );
//  printf( " +setRegionLayers \tname layers\tset region layers. layers is a comma separated list (no spaces) of layer names\n" );
  printf( " +removeRegion  \tname\t\tremove region by name.\n" );
  printf( "\n" );
  printf( " +setRegionsFile \tfileName.json\tuse fileName.json for definition of regions (default=%s)\n", TrackGlobal::regionsFileName.c_str() );

  printf( "\nregions usage:\n" );
  printf( " +useRegions\t\tswitch on usage of regions\n" );
}

static void printDefaultsHelp( int argc, const char *argv[] )
{
  printf( "\nDEFAULTS management in configuration file %s:\n", TrackGlobal::defaultsFileName.c_str() );
  printf( " +setDefault     name value\tsets a default value\n" );
  printf( " +setDefaultArgs name\t\tsets the whole command line as default value. can be used in command line with ^name\n" );
  printf( " +removeDefault  name\t\tremoves a default value\n" );
  printf( " +listDefaults\t\t\tlists all defaults\n" );
}

static void printNikNamesHelp( int argc, const char *argv[] )
{
  printf( "\nNIK NAMES management in configuration file %s:\n", g_NikNameFileName.c_str() );
  printf( " +setNikName    nikName device\t\tsets the nikname for device (e.g. +snn left lidar0 will replace devicename left by lidar0)\n" );
  printf( " +renameNikName oldName newName\t\trename existing nikname\n" );
  printf( " +removeNikName nikName\t\t\tremoves nikname(e.g. +removeNikName left)\n" );
  printf( " +listNikNames\t\t\t\tlists niknames and the devices they currently point to\n" );
  printf( " +clearNikNames\t\t\t\tempty configuration file, remove all niknames\n" );
#if __LINUX__
  printf( " +setNikNameBySerial nikName device\tsets the nikname for device by its serial number (e.g. +setNikNameBySerial ttyUSB0 will replace ttyUSB0 by lidarSERIALNUMBER the next time the device is plugged in)\n" );
#endif
}

static void printSimulationModeHelp( int argc, const char *argv[] )
{
  printf( "\nDEVICE SIMULATION related args:\n" );
  printf( " +simulationMode|+s\t\t\tturns on simulation mode\n" );
  printf( "\nIn simulation mode:\n" );
  printf( "    nik names are read from %s%s\n", g_InstallDir.c_str(), nikNamesSimulationModeFileName.c_str() );
  printf( "    devices must have a deviceType qualifier\n" );
  printf( "    devices generate generic scan data and interact with simulated obstacles\n" );
}

static void printGroupsHelp( int argc, const char *argv[] )
{
  printf( "\nGROUPS management in configuration file %s:\n", groupsFileName.c_str() );
  printf( " +assignDeviceToGroup   groupName deviceName\t\tadds deviceName to the group groupName\n" );
  printf( " +removeDeviceFromGroup groupName deviceName\t\tremoves deviceName from the group groupName\n" );
  printf( " +renameDeviceInGroup   groupName oldName newName\trenames a device in group groupName\n" );
  printf( " +renameDeviceInGroups  oldName   newName\t\trenames a device in all groups\n" );
  printf( " +removeGroup           groupName\t\t\tremoves the group groupName from the configuration file\n" );
  printf( " +listGroups\t\t\t\t\t\tlists all groups currently defined\n" );
  printf( " +clearGroups\t\t\t\t\t\tempty configuration file, remove all groups\n" );
  printf( "\nusing device groups at programm start:\n" );
  printf( "\n +g groupName\t\tinclude all devices defined in group groupName (if groupName is 'all', then all groups are added)\n" );
  printf( " -g groupName\t\texclude all devices defined in group groupName\n" );
  printf( " +useGroups\t\tsame as '+g all'\n" );
}

static void printBlueprintHelp( int argc, const char *argv[] )
{
  printf( "\nBLUEPRINT management in configuration file %s:\n", blueprintsFileName.c_str() );
  printf( " +setBlueprint                  imageFileName [pixel=]extent\tsets the blueprint image file with extent in meter\n" );
  printf( " +setBlueprintImage             imageFileName\t\t\tsets the blueprint image file\n" );
  printf( " +setBlueprintExtent            [pixel=]extent\t\t\tsets the blueprint extent in meter\n" );
  printf( " +setBlueprintSimulationEnvMap  imageFileName\t\t\tsets the blueprint simulation environment image file\n" );
  printf( " +setBlueprintTrackOcclusionMap imageFileName\t\t\tsets the blueprint tracking occlusion image file\n" );
  printf( " +setBlueprintObstacle          imageFileName [pixel=]extent\tsets the obstacle image file with extent in meter\n" );
  printf( " +setBlueprintObstacleImage     imageFileName\t\t\tsets the obstacle image file\n" );
  printf( " +setBlueprintObstacleExtent    [pixel=]extent\t\t\tsets the obstacle extent in meter\n" );
  printf( " +listBlueprints\t\t\t\t\t\tlists all blueprints currently defined\n" );
  printf( " +useBlueprints\t\t\t\t\t\t\tuse blueprint for display in web browser ui\n" );
  printf( "\n" );
  printf( "  the extent parameter determines how long a pixel is. If no pixel number is given, te extent relates to the full pixel width of the image. If you now the distance of two pixel in the blueprint image in meter (e.g. 5m), determine the  the number of pixels between them (i.g. 137px) and set the extent by pixel=distance (e.g. +setBlueprintExtent 137=5)\n" );
  printf( "\n" );
}

static void printLoggingHelp( int argc, const char *argv[] )
{
  printf( "\nLOGGING:\n" );
  printf( " +log fileName\tswitch on logging of tracking to fileName\n" );
  printf( "  if fileName has 'date' formatting, a new file is created when file name changes depending on the current time\n" );
  printf( "  these fileNames can be used as shortcuts:\n" );
  printf( "\tday|daily\t\tsynonym for 'log/log_%%Y-%%m-%%d'\n" );
  printf( "\thour|hourly\t\tsynonym for 'log/log_%%Y-%%m-%%d-%%H'\n" );
  printf( "\tminute|minutely\t\tsynonym for 'log/log_%%Y-%%m-%%d-%%H:%%M'\n" );
  printf( "\n" );
  printf( " +logSuffix suffix\t\tsuffix to append log file name in case a synonym is used (default: %s)\n", g_LogSuffix.c_str() );
  printf( " +logDistance distance\tlog move event when center moved about distance (in meter) (default: %g)\n", g_Track.logDistance );

  printf( "\n" );
  printf( " +trackHeatMap fileName\tswitch on tracking heatMap to fileName (given without suffix)\n" );
  printf( "  if fileName has 'date' formatting, a new file is created when file name changes depending on the current time\n" );
  printf( "  these fileNames can be used as shortcuts:\n" );
  printf( "\tday|daily\t\tsynonym for 'heatmap/heatmap_%%Y-%%m-%%d'\n" );
  printf( "\thour|hourly\t\tsynonym for 'heatmap/heatmap_%%Y-%%m-%%d-%%H'\n" );
  printf( "\tminute|minutely\t\tsynonym for 'heatmap/heatmap_%%Y-%%m-%%d-%%H:%%M'\n" );
  printf( "\tnone\t\t\tdo not log to a file, just isplay it in the user interface\n" );
  printf( "\n" );
  printf( " +imageSuffix suffix\t\tsuffix to append imageLog file name in case a synonym is used (default: %s)\n", g_ImageSuffix.c_str() );
  printf( " +trackImageRes resolution\tarea in m each pixel is covering (default: %g)\n", g_Track.imageSpaceResolution );
}

static void printProcessingHelp( int argc, const char *argv[] )
{
  printf( "\nLIDAR DATA PROCESSING parameters\n" );
  g_Devices.printArgHelp();
  dummyDevice->printArgHelp();
  printf( "\n" );
//  printf( " +adaptEnv\tset the environment adaptive. the environment samples adapt to objects which reside in the tracking area for a longer time\n" );
}

static void printDevicesHelp( int argc, const char *argv[] )
{
  printf( "\nDEVICES:\n" );

  printf( " +d [deviceType:]device \tdevice to read data from. can be a serial port e.g. %s\n", LidarDevice::getDefaultSerialDevice().c_str() );
  printf( "\t\tor the number of the serial device to read data from. e.g. %d for %s\n", 0, LidarDevice::getDefaultSerialDevice(0).c_str() );
  printf( "\t\tdevice - is a non existent device and will be skipped\n" );
  
  LidarUrl::printHelp( " +d ", "\n\t\tvirtual input lidar device to read data from in the format below. if hostname is defined, a conection request is send to the host on the given port, otherwise it listens for data\n", false );

  printf( "\n" );
  LidarUrl::printHelp( " +virtual ", "\n\t\tvirtual output lidar device to send data to. if hostname is defined, data is send to the host on the given port, otherwise it listens for connection requests\n", true );

//  printf( "\n" );
//  printf( " +b baudrate \tbaudrate of the last serial device defined (115200, default=256000)\n" );
/*
  printf( "\n" );
  printf( "rplidar specific:\n" );
  printf( " rplidar.mode id \t\tscan mode id to be used for the last serial device defined. run with +v to see available scan modes\n" );
  printf( " rplidar.pwm speed \tspeed to be used for the last serial device defined. 720 is a good value for A3 devices\n" );
*/
  printf( "\n" );
    
#if __LINUX__
  printf( " +listDevices\tlists available serial devices\n" );
#endif
  printf( " +sn   device \tprint serial number of device. can be a serial port e.g. %s\n", LidarDevice::getDefaultSerialDevice().c_str() );
  printf( " +info device \tprint info of device. can be a serial port e.g. %s\n", LidarDevice::getDefaultSerialDevice().c_str() );
  
//  printNikNamesHelp( argc, argv );
#if __LINUX__
  printf( "\n" );
  printf( "linux udev:\n" );
  printf( " +udev device\tprint udev symbolic device names for a device\n" );
#endif
}

static void printHelp( int argc, const char *argv[] )
{
  printf( "usage: %s help topic\n", argv[0] );
  printf( "  topic is one out of:\n" );
  printf( "	general  	general usage information\n" );
  printf( "	devices  	sensor device usage\n" );
  printf( "	niknames	nik name usage\n" );
  printf( "	groups  	groups of sensors usage\n" );
  printf( "	processing  	sensor data processing parameter\n" );
  printf( "	tracking	tracking arguments\n" );
//  printf( "	defaults	default value usage\n" );
  printf( "	regions		regions  usage\n" );
  printf( "	observer	observer usage\n" );
  printf( "	blueprint	blueprint usage\n" );
  printf( "	simulation  	sensor simulation usage\n" );
//  printf( "	logging		logging details\n" );
  printf( "	all  		all helps\n" );
}

static void printGeneralHelp( int argc, const char *argv[], bool printAll=false )
{
  if ( printAll )
    printHelp( argc, argv );

  printf( "\nGENERAL:\n" );
  
  printf( " +v [1..3] verbose start. the higher the given level (default=1), the more is reported\n" );
  if ( printAll )
  { printf( "\n" );
    printf( " +d [deviceType:]device \tdevice to read data from. can be a serial port e.g. %s\n", LidarDevice::getDefaultSerialDevice().c_str() );
    printf( "\t\tor the number of the serial device to read data from. e.g. %d for %s\n", 0, LidarDevice::getDefaultSerialDevice(0).c_str() );
    printf( " -openOnStart\tdo not open devices on startup\n" );
  }
  printf( " +fps framesPerSec\tsets the maximum frame rate to process and track/report lidar data (default=%g)\n", g_MaxFps );
  
//  printf( "\n" );
//  printf( " +powerOff\tswitches the devices power off and waits until it is killed. this prevents the A1 devices from restarting the motor\n" );
//  printf( "\n" );

  printf( "\n" );
  printf( "CONFIGURATION:\n" );
  printf( " +conf          dir\tin this program call use directory dir for storing configuration files (current=%s)\n", LidarDevice::configDir.c_str() );
  printf( " +setConfDir    dir\tuse directory dir as default for storing configuration files (current=%s)\n", LidarDevice::configDir.c_str() );
  printf( " +createConfDir dir\tcreate directory for storing configuration files\n" );

  printf( " +listConfDir\tprint directory for storing configuration files\n" );

  printf( "\n" );
  printf( "CHECKPOINTS:\n" );
  printf( " +useCheckPoint [checkpoint|latest|fitting]\tread environment, registration and blueprint data from checkpoint folders\n" );

  printf( "\n" );
  printf( "USER INTERFACE:\n" );
  printf( " +webport    port\tport to be used for Web API (default=%d). if more than one instance runs on the same computer. ports have to be different\n", webserver_port );
  printf( "\t\t\tif port is -, then the webserver is not started\n" );
  printf( " +remoteport port\tvirtual devices webport for remote controlling (default=%d)\n", remote_port );

  printf( "\n" );
  printf( "MESSAGING:\n" );
  printf( " +failureReportScript scriptFile\trun scriptFile with device nik name as first and reason string as second argument on detected failure of device\n" );
  printf( " +failureReportSec     sec\t\tseconds to wait until reporting a device failure (default=%d)\n", g_FailureReportSec );
  printf( " +errorLogFile         fileName\t\tfile for writing errors to (default=stderr)\n" );
  printf( " +logFile              fileName\t\tfile for writing log messages to (default=stdout)\n" );
  printf( " +notificationScript   scriptFile\trun scriptFile with tags as first and message string as second argument on notifications\n" );
  printf( " +spinningReportScript scriptFile\trun scriptFile with spinning device information in json format as argument (default=%s)\n", g_DefaultReportSpinningScript.c_str() );
  printf( " +spinningReportSec    sec\t\tinterval in seconds to report spinning devices (default=%d)\n", g_SpinningReportSec );
}


static void printAllHelp( int argc, const char *argv[] )
{
  printGeneralHelp( argc, argv, false );
  
  printDevicesHelp( argc, argv );
  
  printNikNamesHelp( argc, argv );

  printGroupsHelp( argc, argv );

  printProcessingHelp( argc, argv );
  
//  printDefaultsHelp( argc, argv );
  
  printTrackingHelp( argc, argv );

  printRegionsHelp( argc, argv );

  printObserverHelp( argc, argv );

  printBlueprintHelp( argc, argv );

  printSimulationModeHelp( argc, argv );

//  printHelp( argc, argv );

//  printLoggingHelp( argc, argv );
}


static void
resolveDevice( LidarDevice *device, std::string &deviceName )
{
  device->outFileName = g_LidarOutFileTemplate;

  std::string deviceType( defaultDeviceType );
	
  device->setDeviceParam( TrackGlobal::defaults );
  
  const char *col = NULL;
  if ( strstr(deviceName.c_str(), "virtual:") == NULL &&
       strstr(deviceName.c_str(), "file:") == NULL &&
       (col=strstr(deviceName.c_str(), ":")) != NULL )
  {
    deviceType.assign(deviceName.c_str(), col);
    std::string tmp( &col[1] );
    deviceName = tmp;
  }
       
  std::vector<std::string> pair( split(deviceName,'=',2) );
  if ( pair.size() == 2 )
    deviceName  = pair[0];

  std::map<std::string,std::string>::iterator iter( deviceNikNames.find(deviceName) );   
  if ( iter != deviceNikNames.end() )
  { device->nikName = iter->first;
    deviceName      = iter->second;
  }

  if ( !g_LidarInFileTemplate.empty() )
  { device->inFileName = g_LidarInFileTemplate;
    device->deviceName = "";
  }

  if ( strstr(deviceName.c_str(), "file:") )
    device->inFileName  = deviceName;

  if ( strstr(deviceName.c_str(), "virtual:") )
    device->inVirtUrl  = deviceName;
  else
  {
    const char *col = strstr(deviceName.c_str(), ":");
    if ( col != NULL )
    {
      deviceType.assign(deviceName.c_str(), col);
      std::string tmp( &col[1] );
      deviceName = tmp;
    }

    std::map<std::string,std::string>::iterator iter( deviceNikNames.find(deviceName) );   
    if ( iter != deviceNikNames.end() )
    { device->nikName = iter->first;
      deviceName      = iter->second;
    }

    device->setDeviceType( deviceType.c_str() );
    device->deviceName = deviceName;
  }

  if ( pair.size() == 2 )
    device->nikName = pair[1];
}

static void
addDevice( const char *devName )
{
  std::string deviceName( devName );
  std::string deviceType( defaultDeviceType );
	
  Device = new LidarDevice();

  resolveDevice( Device, deviceName );

  if ( deviceName == "-" )
  { delete Device;
    return;
  }

  for ( int i = 0; i < g_Devices.size(); ++i )
    if ( g_Devices[i]->getNikName() == Device->getNikName() )
    { delete Device;
      return;
    }

  if ( g_Verbose )
    TrackGlobal::info( "adding device '%s'", deviceName.c_str() );

  g_Devices.push_back ( Device );
  g_DeviceUI.push_back( DeviceUI() );
}

static void
usedGroupsString( std::string &groupName )
{
  groupName = "";

  for ( KeyValueMap::iterator iter( g_UsedGroups.begin() ); iter != g_UsedGroups.end(); iter++ )
  {
    if ( !groupName.empty() )
      groupName += ",";
    
    groupName += iter->first;
  }
}

static std::set<std::string> g_ExcludeGroups;

static void
addGroup( const char *group, bool addDevices=true )
{
  if ( g_IsHUB )
    return;
  
  std::string groupName( group );

  for ( KeyValueMapDB::iterator dbiter( LidarDeviceGroup::groups.begin() ); dbiter != LidarDeviceGroup::groups.end(); dbiter++ )
  {
    if ( !g_ExcludeGroups.count(dbiter->first) && (groupName == "all" || groupName == dbiter->first) )
    {
      g_UsedGroups.set( dbiter->first.c_str(), "true" );

      if ( addDevices )
      {
	KeyValueMap &map( dbiter->second );

	for ( KeyValueMap::iterator iter( map.begin() ); iter != map.end(); iter++ )
        { const std::string &deviceName( iter->first );
  	  addDevice( deviceName.c_str() );
	}
      }
    }
  }
}

static void
readDefaults()
{ 
  TrackGlobal::readDefaults(); 
  TrackGlobal::getDefault( "deviceType", defaultDeviceType );
  TrackGlobal::getDefault( "failureReportSec", g_FailureReportSec );
}

int main( int argc, const char *argv[] )
{
  g_AppStartDate = timestampString( "%c", getmsec(), false );
  
  cimg::exception_mode(0);

  Lidar::error 		= TrackGlobal::error;
  Lidar::warning	= TrackGlobal::warning;
  Lidar::log   		= TrackGlobal::log;
  Lidar::info		= TrackGlobal::info;
  Lidar::notification 	= TrackGlobal::notification;

  bool powerOff	     = false;
  bool loopPowerOff  = true;
  bool isInfo	     = false;
  bool playExitAtEnd = false;
  
  setInstallDir( argv[0] );
  
  readConfigDir();

  for ( int i = 0; i < argc; ++i )
  {
    if ( strcmp(argv[i],"+conf") == 0 )
    { std::string conf( argv[++i] );
      if ( !testConf( conf ) )
      { TrackGlobal::error( "setting config: directory %s does not exist", conf );
	exit( 0 );
      }
    }
    else if ( strcmp(argv[i],"+confAlt") == 0 )
    { const char *dir = argv[++i];
      if ( fileExists( dir ) )
	LidarDevice::configDirAlt = dir;
      else
      { TrackGlobal::error( "setting config alt: directory %s does not exist", dir );
	exit( 0 );
      }
    }
  }
  
  if ( g_Config.empty() )
  { std::string confDir( "conf/default" );
    if ( fileExists( confDir.c_str() ) )
      testConfigDir( confDir );
    else
    {
      confDir = "conf";
      if ( fileExists( confDir.c_str() ) )
	testConfigDir( confDir );
      else
	LidarDevice::configDir = "./";
    }
  }

  if ( LidarDevice::configDir[LidarDevice::configDir.length()-1] != '/' )
    LidarDevice::configDir += "/";

  if ( !LidarDevice::configDirAlt.empty() && LidarDevice::configDirAlt[LidarDevice::configDirAlt.length()-1] != '/' )
    LidarDevice::configDirAlt += "/";

  TrackGlobal::configDir        = LidarDevice::configDir;
  TrackGlobal::defaultsFileName = LidarDevice::getConfigFileName( "defaults.json" );

   if ( !TrackGlobal::setDefaults( argc, argv ) )
    exit( 0 );

  if ( !TrackGlobal::parseDefaults( argc, argv ) )
    exit( 0 );

  if ( g_Verbose )
    TrackGlobal::info( "using config dir %s", TrackGlobal::configDir );

  std::vector<std::string> argList;
  for ( int i = 0; i < argc; ++i )
  {
    if ( strcmp(argv[i],"+conf") == 0 )
    { ++i;
    }
    else if ( strcmp(argv[i],"+confAlt") == 0 )
    { ++i;
    }
    else if ( strcmp(argv[i],"+v") == 0 )
    { 
      int level = 1;
      
      if ( i < argc-1 && isdigit(argv[i+1][0]) )
	level = atoi(argv[++i]);

      g_Verbose = level;
      LidarDevice::setVerbose( level );
      TrackGlobal::setVerbose( level );

#ifdef TRACKABLE_WEBSOCKET_OBSERVER_H
      TrackableHUB::setVerbose( level );
#endif
    }
    else if ( strcmp(argv[i],"+id") == 0 )
    { 
      g_ID = argv[++i];
    }
    else if ( strcmp(argv[i],"+setRegionsFile") == 0 )
    { 
      TrackGlobal::regionsFileName = argv[++i];
    }
    else if ( strcmp(argv[i],"+simulationMode") == 0 || strcmp(argv[i],"+s") == 0 )
    { 
      g_Devices.setSimulationMode( true );
      nikNamesFileName = nikNamesSimulationModeFileName;
    }
    else if ( strcmp(argv[i],"+useSimulationRange") == 0 )
    { 
      g_Devices.setUseSimulationRange( true );
    }
    else if ( strcmp(argv[i],"+useObstacle") == 0 )
    { 
      g_UseObstacle = true;
    }
    else if ( strcmp(argv[i],"-useObstacle") == 0 )
    { 
      g_UseObstacle = false;
    }
    else if ( strcmp(argv[i],"+runMode") == 0 )
    { 
      g_RunningMode = argv[++i];
    }
    else if ( strcmp(argv[i],"+useCheckPoint") == 0 )
    { 
      std::string checkpoint( argv[++i] );
      LidarDevices::setReadCheckPoint( checkpoint.c_str() );
      TrackBase::setReadCheckPoint( checkpoint.c_str() );
    }
    else if ( strcmp(argv[i],"+useStatusIndicator") == 0 )
    { 
      LidarDevice::setUseStatusIndicator( true );
    }
#ifdef TRACKABLE_WEBSOCKET_OBSERVER_H
    else if ( strcmp(argv[i],"+hub") == 0 )
    { 
      argList.push_back( std::string(argv[i]) );
      argList.push_back( std::string(argv[i+1]) );

      std::string val( argv[++i] );
      std::vector<std::string> endpoint( split(val,':') );
      
      while ( endpoint.size() >= 2 && endpoint[0].empty() )
	endpoint.erase( endpoint.begin() );
      
      if ( endpoint.size() == 1 )
	g_HasHUB = true;
      else
	g_IsHUB = true;
    }
#endif
    else
      argList.push_back( std::string(argv[i]) );
  }

  const char **argV = new const char*[argList.size()+1];
  for ( int i = 0; i < argList.size(); ++i )
    argV[i] = strdup( argList[i].c_str() );
  
  argc = argList.size();
  argv = argV;


  if ( LidarDevice::configDir[LidarDevice::configDir.length()-1] != '/' )
    LidarDevice::configDir += "/";

  if ( !LidarDevice::configDirAlt.empty() && LidarDevice::configDirAlt[LidarDevice::configDirAlt.length()-1] != '/' )
    LidarDevice::configDirAlt += "/";

  TrackGlobal::configDir        = LidarDevice::configDir;
  TrackGlobal::defaultsFileName = TrackGlobal::getConfigFileName( "defaults.json" );
  TrackGlobal::observerFileName = TrackGlobal::getConfigFileName( "observer.json" );
  TrackGlobal::regionsFileName  = TrackGlobal::getConfigFileName( TrackGlobal::regionsFileName.c_str() );
  blueprintsFileName  		= TrackGlobal::getConfigFileName( "blueprints.json" );
  groupsFileName  		= TrackGlobal::getConfigFileName( "groups.json" );
  g_NikNameFileName             = TrackGlobal::getConfigFileName( nikNamesFileName.c_str() );

//  printf( "regions: %s\n", TrackGlobal::regionsFileName.c_str() );

  Lidar::initialize();

  dummyDevice = new LidarDevice();

  for ( int i = 1; i < argc; ++i )
  {
    bool success = false;

    if ( strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"-help") == 0 || strcmp(argv[i],"+h") == 0 || strcmp(argv[i],"+help") == 0 )
    { printHelp( argc, argv );
      printf( "\n" );
      success = true;
    } 
    else if ( strcmp(argv[i],"help") == 0 )
    {
      success = true;

      if ( i == argc-1 )
	printGeneralHelp( argc, argv, true );
      else 
      { i += 1;

	std::string topic( argv[i] );
	tolower( topic );

	if ( topic == "all" )
	  printAllHelp( argc, argv );
	else if ( topic == "general" )
	  printGeneralHelp( argc, argv );
	else if ( topic == "device" || topic == "devices" )
	  printDevicesHelp( argc, argv );
	else if ( topic == "group" || topic == "groups" )
	  printGroupsHelp( argc, argv );
	else if ( topic == "simulation" )
	  printSimulationModeHelp( argc, argv );
	else if ( topic == "processing" )
	  printProcessingHelp( argc, argv );
	else if ( topic == "tracking" )
	  printTrackingHelp( argc, argv );
	else if ( topic == "defaults" )
	  printDefaultsHelp( argc, argv );
	else if ( topic == "nikname" || topic == "niknames" )
	  printNikNamesHelp( argc, argv );
	else if ( topic == "observer" )
	  printObserverHelp( argc, argv );
	else if ( topic == "region" || topic == "regions" )
	  printRegionsHelp( argc, argv );
	else if ( topic == "blueprint" || topic == "blueprints" )
	  printBlueprintHelp( argc, argv );
//	else if ( topic == "logging" )
//	  printLoggingHelp( argc, argv );
	else
	  printHelp( argc, argv );
      }
    } 

    if ( success )
    { delete dummyDevice;
      exit( 0 );
    }
  }

  for ( int i = 1; i < argc; ++i )
  {
    if ( strcmp(argv[i],"+listConfDir") == 0 )
    {
      printf( "configDir=%s\n", LidarDevice::configDir.c_str() );
      exit( 0 );
    }
    else if ( strcmp(argv[i],"+createConfDir") == 0 )
    {
      std::string dir( argv[++i] );

      if ( fileExists( dir.c_str() ) )
      { printf( "directory %s already exists\n", dir.c_str() );
      }
      else
      { printf( "creating directory %s\n", dir.c_str() );
	std::filesystem::create_directory( dir.c_str() );
      }
      exit( 0 );
    }
    else if ( strcmp(argv[i],"+setConfDir") == 0 )
    {
      std::string dir( argv[++i] );

      if ( !fileExists( dir.c_str() ) )
      { printf( "directory %s does not exists\n", dir.c_str() );
	printf( "run\n" );
	printf( "  %s +createConfDir %s\n", argv[0], dir.c_str() );
		printf( "for creating the directory\n" );
	exit( 0 );
      }
      
      writeConfigDir( dir.c_str() );
      exit( 0 );
    }
    else if ( strcmp(argv[i],"+sn") == 0 ||
#if __LINUX__
	 strcmp(argv[i],"+udev") == 0 || 
#endif
	 strcmp(argv[i],"+info") == 0 )
    { 
      LidarDevice *device = new LidarDevice();
      std::string deviceName( argv[i+1] );
      std::string deviceType;

      const char *col = strstr(deviceName.c_str(), ":");
      if ( col != NULL )
      { deviceType.assign(deviceName.c_str(), col);
	std::string tmp( &col[1] );
	deviceName = tmp;
      }
      if ( !deviceType.empty() )
	device->setDeviceType( deviceType.c_str() );

      device->setUARTPower( true, deviceName.c_str() );

      if ( strcmp(argv[i],"+sn") == 0 )
      { std::string sn = device->getSerialNumber( deviceName.c_str() );
	
	if ( sn.empty() )
	{ readDefaults(); 
	  TrackGlobal::getDefault( "deviceType", deviceType );
	  if ( !deviceType.empty() )
	  { device->setDeviceType( deviceType.c_str() );
	    sn = device->getSerialNumber( deviceName.c_str() );
	  }
	}
	
	if ( sn.empty() )
	  TrackGlobal::error( "failed to read serial number from device %s", deviceName.c_str() );
	else
	  printf( "%s\n", sn.c_str() );
      }
#if __LINUX__
      else if ( strcmp(argv[i],"+udev") == 0 )
      { 
	std::string sn;
	i += 1;

	if ( !g_Verbose )
	  LidarDevice::setVerbose( -1 );

	readNikNames();

	usleep( 500000 );
	
	if ( device->openDeviceMSLidar( true ) )
	  ;
	else if ( device->openDeviceLDLidar( true ) )
	  ;
	else
	{
	  usleep( 2000000 );
	  device->driverType = LidarDevice::YDLIDAR;
	  sn = device->getSerialNumber( deviceName.c_str() );

	  if ( sn.empty() )
	  { device->driverType = LidarDevice::RPLIDAR;
	    sn = device->getSerialNumber( deviceName.c_str() );
	    if ( sn.empty() )
	      device->openDeviceLSLidar( true );
	  }
	}
	printNikName( sn );
      }
#endif
      else
	device->dumpInfo( deviceName.c_str() );

      device->setUARTPower( false, deviceName.c_str() );

      delete device;
      exit( 0 );
    }
    else if ( TrackGlobal::parseArg( i, argv, argc ) )
    {}
    else if ( strcmp(argv[i],"+uuidHeader") == 0 )
    { 
      uuid_app_id_t app_id;
      memset( app_id, 0, 6 );
      strncpy( (char*)app_id, argv[++i], 6 );

      UUID::set_app_id( app_id );
    }
    else if ( strcmp(argv[i],"+expert") == 0 )
    { 
      g_ExpertMode = true;
    }
    else if ( strcmp(argv[i],"+packedPlay") == 0 )
    { 
      g_PackedInFileName = argv[++i];
      g_DoTrack = true;
    }
    else if ( strcmp(argv[i],"+lidarPlay") == 0 )
    { 
      std::string fileTemplate = argv[++i];

      if ( std::filesystem::is_directory( fileTemplate ) )
      {
	std::filesystem::path path( std::filesystem::relative( std::filesystem::canonical(fileTemplate) ) );

	std::filesystem::path calib( path );
	calib /= std::filesystem::path("conf");

	if ( LidarDevice::configDirAlt.empty() && std::filesystem::is_directory( calib ) )
        { LidarDevice::configDirAlt = calib;
	  if ( LidarDevice::configDirAlt[LidarDevice::configDirAlt.length()-1] != '/' )
	    LidarDevice::configDirAlt += "/";
	}
	
	fileTemplate = path;
	fileTemplate += "/";
	fileTemplate += path.filename();
	fileTemplate += "_%nikname.lidar";
      }

      g_LidarInFileTemplate = fileTemplate;
    }
    else if ( strcmp(argv[i],"+playExitAtEnd") == 0 )
    { 
      playExitAtEnd = true;
    }
    else if ( strcmp(argv[i],"+lidarRecord") == 0 )
    { 
      g_LidarOutFileTemplate = argv[++i];
      if ( g_LidarOutFileTemplate == "default" )
	g_LidarOutFileTemplate = "%default";
    }
    else if ( strcmp(argv[i],"+setNikName") == 0 )
    { 
      std::string key( argv[++i] );

      readNikNames();

      setNikName( key.c_str(), argv[++i] );

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+setNikNameBySerial") == 0 )
    { 
      std::string key       ( argv[++i] );
      std::string deviceName( argv[++i] );

      readNikNames();

      LidarDevice *device = new LidarDevice();

      std::string sn = device->getSerialNumber( deviceName.c_str() );
      if ( sn.empty() )
	TrackGlobal::error( "failed to read serial number from device %s", deviceName.c_str() );
      else
	setNikName( sn.c_str(), key.c_str() );
      
      delete device;
      exit( 0 );
    }
    else if ( strcmp(argv[i],"+removeNikName") == 0 )
    { 
      readNikNames();
      removeNikName( argv[++i] );
      exit( 0 );
    }
    else if ( strcmp(argv[i],"+clearNikNames") == 0 )
    { 
      clearNikNames();
      exit( 0 );
    }
    else if ( strcmp(argv[i],"+renameNikName") == 0 )
    { 
      std::string oldName( argv[++i] );
      std::string newName( argv[++i] );

      readNikNames();
      renameNikName( oldName.c_str(), newName.c_str() );
    
      LidarDeviceGroup::read( groupsFileName.c_str() );
      LidarDeviceGroup::renameDevice( oldName.c_str(), newName.c_str() );
      LidarDeviceGroup::write( groupsFileName.c_str() );

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+listNikNames") == 0 )
    {
      printf( "\nnikNameFile=%s\n", g_NikNameFileName.c_str() );

      readNikNames();

      for ( std::map<std::string,std::string>::iterator iter( deviceNikNames.begin() );
	    iter != deviceNikNames.end();
	    iter++ )
      {
	std::string deviceName( LidarDevice::resolveDeviceName( iter->second.c_str() ) );
      
	printf( "\n" );
	printf( "key=%s name=%s", iter->first.c_str(), iter->second.c_str() );

	std::string virt(  "virtual:" );

	if ( deviceName[0] == '/' || startsWithCaseInsensitive(deviceName,virt) )
	  printf( " device=%s", deviceName.c_str() );

	printf( "\n" );
      }

      printf( "\n" );
      exit( 0 );
    }
    else if ( strcmp(argv[i],"+setBlueprint") == 0 )
    { 
      std::string     image( argv[++i] );
      std::string    extent( argv[++i] );

      readBlueprints();
      blueprints.set( "image",  image.c_str() );
      blueprints.set( "extent", extent.c_str() );
      writeBlueprints();

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+setBlueprintImage") == 0 )
    { 
      std::string     image( argv[++i] );

      readBlueprints();
      blueprints.set( "image",  image.c_str() );
      writeBlueprints();

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+setBlueprintExtent") == 0 )
    { 
      std::string    extent( argv[++i] );

      readBlueprints();
      blueprints.set( "extent", extent.c_str() );
      writeBlueprints();

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+setBlueprintSimulationEnvMap") == 0 )
    { 
      std::string     image( argv[++i] );

      readBlueprints();
      blueprints.set( "simulationEnvMap",  image.c_str() );
      writeBlueprints();

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+setBlueprintTrackOcclusionMap") == 0 )
    { 
      std::string     image( argv[++i] );

      readBlueprints();

      blueprints.set( "trackOcclusionMap", image.c_str() );

      writeBlueprints();

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+setBlueprintObstacle") == 0 )
    { 
      std::string     image( argv[++i] );
      std::string    extent( argv[++i] );

      readBlueprints();
      blueprints.set( "obstacleImage",  image.c_str() );
      blueprints.set( "obstacleExtent", extent.c_str() );
      writeBlueprints();

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+setBlueprintObstacleImage") == 0 )
    { 
      std::string     image( argv[++i] );

      readBlueprints();
      blueprints.set( "obstacleImage",  image.c_str() );
      writeBlueprints();

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+setBlueprintObstacleExtent") == 0 )
    { 
      std::string    extent( argv[++i] );

      readBlueprints();
      blueprints.set( "obstacleExtent", extent.c_str() );
      writeBlueprints();

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+obstacle") == 0 )
    {
      obstacleFileName = argv[++i];
      obstacleExtent   = argv[++i];
    }
    else if ( strcmp(argv[i],"+listBlueprint") == 0 )
    {
      printf( "\nblueprintFile=%s\n", blueprintsFileName.c_str() );

      readBlueprints();

      for ( KeyValueMap::iterator iter( blueprints.begin() ); iter != blueprints.end(); iter++ )
      {
	const std::string   &key( iter->first );
	const std::string &value( iter->second );
	if ( key == "image" )
        {
	  std::string info;
	  try { rpImg img( TrackGlobal::getConfigFileName(value.c_str()).c_str() );
	    info = std::to_string(img.width()) + " x " + std::to_string(img.height());
	  } catch(CImgException& e) {
	    info = "Error: failed to read file";
	  }
	    
	  printf( "  %s=\"%s\"   %s", key.c_str(), value.c_str(), info.c_str() );
	  printf( "\n" );
	}
	else if ( key != "x" && key != "y" )
        {
	  printf( "  %s=\"%s\"   ", key.c_str(), value.c_str() );
	  printf( "\n" );
	}
      }
      
      printf( "\n" );
      exit( 0 );
    }
    else if ( strcmp(argv[i],"+assignDeviceToGroup") == 0 )
    { 
      std::string  group( argv[++i] );
      std::string device( argv[++i] );

      LidarDeviceGroup::read( groupsFileName.c_str() );
      LidarDeviceGroup::addDevice( group.c_str(), device.c_str() );
      LidarDeviceGroup::write( groupsFileName.c_str() );

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+removeDeviceFromGroup") == 0 )
    { 
      std::string  group( argv[++i] );
      std::string device( argv[++i] );

      LidarDeviceGroup::read( groupsFileName.c_str() );
      LidarDeviceGroup::removeDevice( group.c_str(), device.c_str() );
      LidarDeviceGroup::write( groupsFileName.c_str() );

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+renameDeviceInGroups") == 0 )
    { 
      std::string oldName( argv[++i] );
      std::string newName( argv[++i] );
    
      LidarDeviceGroup::read( groupsFileName.c_str() );
      LidarDeviceGroup::renameDevice( oldName.c_str(), newName.c_str() );
      LidarDeviceGroup::write( groupsFileName.c_str() );

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+renameDeviceInGroup") == 0 )
    { 
      std::string  group( argv[++i] );
      std::string oldName( argv[++i] );
      std::string newName( argv[++i] );
    
      LidarDeviceGroup::read( groupsFileName.c_str() );
      LidarDeviceGroup::renameDevice( group.c_str(), oldName.c_str(), newName.c_str() );
      LidarDeviceGroup::write( groupsFileName.c_str() );

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+removeGroup") == 0 )
    { 
      std::string  group( argv[++i] );

      LidarDeviceGroup::read( groupsFileName.c_str() );
      LidarDeviceGroup::removeGroup( group.c_str() );
      LidarDeviceGroup::write( groupsFileName.c_str() );

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+clearGroups") == 0 )
    { 
      LidarDeviceGroup::clearGroups();
      LidarDeviceGroup::write( groupsFileName.c_str() );

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+renameGroup") == 0 )
    { 
      std::string oldName( argv[++i] );
      std::string newName( argv[++i] );
    
      LidarDeviceGroup::read( groupsFileName.c_str() );
      LidarDeviceGroup::renameGroup( oldName.c_str(), newName.c_str() );
      LidarDeviceGroup::write( groupsFileName.c_str() );

      exit( 0 );
    }
    else if ( strcmp(argv[i],"+listGroups") == 0 )
    {
      printf( "\ngroupFile=%s\n", groupsFileName.c_str() );
      
      readDefaults(); 
	  
      LidarDeviceGroup::read( groupsFileName.c_str() );

      for ( KeyValueMapDB::iterator dbiter( LidarDeviceGroup::groups.begin() ); dbiter != LidarDeviceGroup::groups.end(); dbiter++ )
      {
	KeyValueMap &map( dbiter->second );

	printf( "\n" );
	printf( "group=%s\n", dbiter->first.c_str() );
        for ( KeyValueMap::iterator iter( map.begin() ); iter != map.end(); iter++ )
        {
	  const std::string   &key( iter->first );
	  const std::string &value( iter->second );

	  std::string deviceName( key );

	  LidarDevice *device = new LidarDevice();
	  resolveDevice( device, deviceName );
	  std::string baseName( device->getBaseName() );
	  delete device;

	  printf( "  %s=%s", value.c_str(), key.c_str() );
	  if ( key != baseName )
	    printf( " -> %s", baseName.c_str() );
	  printf( "\n" );
	}
      }

      printf( "\n" );
      exit( 0 );
    }
#if __LINUX__
    else if ( strcmp(argv[i],"+listDevices") == 0 )
    {
      std::string dirName( "/dev/" );
      printf( "\n" );

      readNikNames();

      for ( int i = 0; i < LidarDevice::maxDevices; ++i )
      {
	LidarDevice::Info info;

	std::string deviceNameUSB( "ttyUSB" );
	std::string deviceNameACM( "ttyACM" );
	std::string deviceNameS  ( "ttyS" );
	deviceNameUSB += std::to_string( i );
	deviceNameACM += std::to_string( i );
	deviceNameS   += std::to_string( i );
	LidarDevice *device = new LidarDevice();
	
	std::string fullDeviceNameUSB( dirName + deviceNameUSB );
	std::string fullDeviceNameACM( dirName + deviceNameACM );
	std::string fullDeviceNameS  ( dirName + deviceNameS   );
//	device->setUARTPower( true, fullDeviceName.c_str() );

	if ( device->getInfo( info, fullDeviceNameUSB.c_str() ) || 
	     device->getInfo( info, fullDeviceNameACM.c_str() ) || 
	     device->getInfo( info, fullDeviceNameS.c_str() ) )
	{
	  std::string sn = device->getSerialNumber(info);
	  std::string fullDeviceName( device->resolveDeviceName( device->deviceName.c_str() ) );

	  printf( "device=%s\n", fullDeviceName.c_str() );
	  printf( " driver=%s\n", LidarDevice::driverTypeString(info.detectedDriverType) );
	  if ( !info.detectedDeviceType.empty() )
	    printf( " model=%s\n", info.detectedDeviceType.c_str() );

	  if ( sn != "00000000000000000000000000000000" )
	    printf( " serial=%s\n", sn.c_str() );
	  
	  struct dirent *entry;
	  DIR *dp = opendir( dirName.c_str() );
	  if ( dp != NULL )
          {
	    while ( entry=readdir(dp) )
	    { std::string otherName( dirName + entry->d_name );
	      if ( isSymLink( device->deviceName, otherName ) )
		printf( " link=%s\n", otherName.c_str() );
	    }
	    
	    closedir(dp);
	  }
	  
	  std::map<std::string,std::string>::iterator iter( deviceNikNames.find(sn) );
	  if ( iter != deviceNikNames.end() )
	    printf( " nikName=%s\n", iter->second.c_str() );
	    

	  printf( "\n" );
	}

//	device->setUARTPower( false, fullDeviceName.c_str() );

	delete device;
      }
      
      exit( 0 );
    }
#endif
    else if ( strcmp(argv[i],"+trackImageRes") == 0 )
    {
      g_Track.imageSpaceResolution = atof( argv[++i] ); 
    }
    else if ( strcmp(argv[i],"+logFilter") == 0 )
    { g_Track.logFilter = argv[++i];
    }
    else if ( strcmp(argv[i],"+logSuffix") == 0 )
    { g_LogSuffix = argv[++i];
      if ( !g_LogSuffix.empty() && g_LogSuffix[0] != '.'  )
	g_LogSuffix = "." + g_LogSuffix;
    }
    else if ( strcmp(argv[i],"+imageSuffix") == 0 )
    { g_ImageSuffix = argv[++i];
      if ( !g_ImageSuffix.empty() && g_ImageSuffix[0] != '.' )
	g_ImageSuffix = "." + g_ImageSuffix;
    }
    else if ( strcmp(argv[i],"+logDistance") == 0 )
    {
      g_Track.logDistance = atof( argv[++i] ); 
    }
    else if ( strcmp(argv[i],"+useRegions") == 0 )
    { 
      TrackGlobal::loadRegions();
    }
    else if ( strcmp(argv[i],"-g") == 0 )
    {
      std::string groupN( argv[++i] );
      g_ExcludeGroups.emplace( groupN );
    }
  }

  std::string reportSpinningScript( "[conf]/" );
  reportSpinningScript += g_DefaultReportSpinningScript;
  reportSpinningScript = TrackGlobal::configFileName( reportSpinningScript.c_str() );
  if ( fileExists( reportSpinningScript.c_str() ) )
    g_SpinningReportScript = reportSpinningScript;
  else
  { if ( fileExists( g_DefaultReportSpinningScript.c_str() ) )
      g_SpinningReportScript = g_DefaultReportSpinningScript;
  }

  std::string groupName;

  TrackGlobal::readDefaults();
  readNikNames();

  replaceEnvVar( deviceNikNames );
  replaceEnvVar( TrackGlobal::defaults );

  if ( !g_IsHUB )
    LidarDeviceGroup::read( groupsFileName.c_str() );

  TrackGlobal::getDefault( "deviceType", defaultDeviceType );
  TrackGlobal::getDefault( "webserver_port", webserver_port );
  TrackGlobal::getDefault( "remote_port",    remote_port );
  TrackGlobal::getDefault( "track", g_DoTrack );
  
  uiImageType = "jpg";
  uiMimeType  = "image/jpg";

  for ( int i = 1; i < argc; ++i )
  {
    if ( strcmp(argv[i],"+d") == 0 )
    { std::string deviceName( argv[++i] );

      std::string commandlineGroup( "Commandline" );
      LidarDeviceGroup::addDevice( commandlineGroup.c_str(), deviceName.c_str() );
      addGroup( commandlineGroup.c_str(), false );
      addDevice( deviceName.c_str() );
    }
    else if ( strcmp(argv[i],"+g") == 0 )
    {
      std::string groupN( argv[++i] );
      addGroup( groupN.c_str() );
    }
    else if ( strcmp(argv[i],"-g") == 0 )
    {
      ++i;
    }
    else if ( strcmp(argv[i],"+useGroups") == 0 )
    {
      addGroup( "all" );
    }
    else if ( strcmp(argv[i],"+b") == 0 || strcmp(argv[i],"+p") == 0 )
    { int baudrateOrPort = atoi( argv[++i] );
      device()->baudrateOrPort = baudrateOrPort;
    }
    else if (strcmp(argv[i],"rplidar.pwm") == 0 )
    { int pwm = atoi( argv[++i] );
      device()->motorPWM = pwm;
    }
    else if (strcmp(argv[i],"+powerOff") == 0 )
    { powerOff = true;
    }
    else if (strcmp(argv[i],"+adaptEnv") == 0 )
    { dummyDevice->doEnvAdaption = true;
    }
    else if ( strcmp(argv[i],"rplidar.mode") == 0 )
    { device()->rplidar.scanMode = argv[++i];
    }
    else if ( strcmp(argv[i],"+env") == 0 )
    { device()->envFileName = argv[++i];
    }
    else if ( strcmp(argv[i],"+mtx") == 0 )
    { device()->matrixFileName = argv[++i];
    }
    else if ( strcmp(argv[i],"+webport") == 0 || strcmp(argv[i],"+wp") == 0 )
    {
      const char *portString = argv[++i];
      webserver_port = (strcmp( portString, "-" ) == 0 ? -1 : std::atoi(portString) );
    }
    else if ( strcmp(argv[i],"+remoteport") == 0 || strcmp(argv[i],"+rp") == 0 )
    { remote_port = atoi( argv[++i] );
    }
    else if ( strcmp(argv[i],"+track") == 0 )
    { g_DoTrack = true;
    }
    else if ( strcmp(argv[i],"+uuidHeader") == 0 )
    { i += 1;
    }
    else if ( strcmp(argv[i],"+expert") == 0 )
    { 
    }
    else if ( strcmp(argv[i],"+runMode") == 0 )
    { i += 1;
    }
    else if ( strcmp(argv[i],"+packedPlay") == 0 )
    { i += 1;
    }
    else if ( strcmp(argv[i],"+lidarPlay") == 0 )
    { i += 1;
    }
    else if ( strcmp(argv[i],"+playExitAtEnd") == 0 )
    { 
    }
    else if ( strcmp(argv[i],"+lidarRecord") == 0 )
    { i += 1;
    }
/*
    else if ( strcmp(argv[i],"+logSuffix") == 0 )
    { i += 1;
    }
    else if ( strcmp(argv[i],"+logFilter") == 0 )
    { i += 1;
    }
    else if ( strcmp(argv[i],"+logDistance") == 0 )
    { i += 1;
    }
    else if ( strcmp(argv[i],"+log") == 0 )
    { 
      bool useSuffix = true;
      
      std::string fileName( argv[++i] );
      if ( fileName == "month" || fileName == "monthly" )
	fileName = "log/log_%monthly";
      else if ( fileName == "week" || fileName == "weekly" )
	fileName = "log/log_%weekly";
      else if ( fileName == "day" || fileName == "daily" )
	fileName = "log/log_%daily";
      else if ( fileName == "hour" || fileName == "hourly" )
	fileName = "log/log_%hourly";
      else if ( fileName == "minute" || fileName == "minutely" )
	fileName = "log/log_%minutely";
      else
	useSuffix = false;
      
      if ( useSuffix )
	fileName += g_LogSuffix;

      log( "INFO logging events to %s", fileName.c_str() );

      KeyValueMap descr;
      descr.set( "type", "file" );
      descr.set( "file", fileName.c_str() );

      parseArg( i, argv, argc, descr );


      g_Track.addObserver( descr, RegionsAsLogMask );
    }
    else if ( strcmp(argv[i],"+imageSuffix") == 0 )
    { i += 1;
    }
    else if ( strcmp(argv[i],"+trackImageRes") == 0 )
    { i += 1;
    }
    else if ( strcmp(argv[i],"+trackHeatMap") == 0 || strcmp(argv[i],"+trackTraceMap") == 0 )
    {
      TrackGlobal::loadRegions();


      std::string mapType = (strcmp(argv[i],"+trackHeatMap") == 0 ? "heatmap" : "tracemap" );

      TrackableRegion *mapRect = TrackGlobal::regions.get( mapType.c_str() );
      
      if ( mapRect == NULL )
      { mapRect         = TrackGlobal::regions.get( mapType.c_str(), true );
	mapRect->type   = RegionsAsImage;
	TrackGlobal::saveRegions();
      }

      bool useSuffix = true;
      
      std::string fileName( argv[++i] );
      if ( fileName == "month" || fileName == "monthly" )
	fileName = mapType+"/"+mapType + "_%monthly";
      else if ( fileName == "week" || fileName == "weekly" )
	fileName = mapType+"/"+mapType + "_%weekly";
      else if ( fileName == "day" || fileName == "daily" )
	fileName = mapType+"/"+mapType + "_%daily";
      else if ( fileName == "hour" || fileName == "hourly" )
	fileName = mapType+"/"+mapType + "_%hourly";
      else if ( fileName == "minute" || fileName == "minutely" )
	fileName = mapType+"/"+mapType + "_%minutely";
      else
      { useSuffix = false;
	if ( fileName == "none" )
	  fileName = "";
      }
      if ( useSuffix )
	fileName += g_ImageSuffix;

      if ( g_Verbose && !fileName.empty() )
	TrackGlobal::info( "logging %s to %s", mapType.c_str(), fileName.c_str() );

      KeyValueMap descr;
      descr.set( "type",   mapType.c_str() );
      descr.set( "name",   mapType.c_str() );
      descr.set( "region", mapType.c_str() );
      descr.set( "seed",   "0.5" );

//      descr.set( "file",  fileName.c_str()  );

      if ( mapType == "heatmap" )
	descr.set( "backgroundColor", "70ff0000" );

      parseArg( i, argv, argc, descr );

      g_Track.addObserver( descr );
    }
*/
    else if ( strcmp(argv[i],"+useObserver") == 0 || strcmp(argv[i],"+useObservers") == 0 )
    {
      std::string name( "all" );
      if ( strcmp(argv[i],"+useObserver") == 0 )
	name = argv[++i];

      TrackGlobal::readObservers();

      bool all = (name == "all");
      
      for ( KeyValueMapDB::iterator dbiter( TrackGlobal::observers.begin() ); dbiter != TrackGlobal::observers.end(); dbiter++ )
      { KeyValueMap &descr( dbiter->second );
	if ( all || name == dbiter->first )
	{ descr.set( "name", dbiter->first.c_str() );
	  if ( !g_RunningMode.empty() )
	    descr.set( "runMode", g_RunningMode.c_str() );
	  g_Track.addObserver( descr );
	}
      }
    }
#ifdef TRACKABLE_WEBSOCKET_OBSERVER_H
    else if ( strcmp(argv[i],"+hub") == 0 )
    { 
      std::string val( argv[++i] );
      std::vector<std::string> endpoint( split(val,':') );
      
      while ( endpoint.size() >= 2 && endpoint[0].empty() )
	endpoint.erase( endpoint.begin() );
      
      if ( endpoint.size() == 1 )
      {
	KeyValueMap descr;
	std::string maxFPS( std::to_string(g_MaxFps*1.1) );
	descr.set( "type", "packedwebsocket" );
	descr.set( "port", endpoint[0].c_str() );
	descr.set( "continuous", "1" );
	descr.set( "maxFPS", maxFPS.c_str() );
	descr.set( "fullFrame", "1" );

	parseArg( i, argv, argc, descr );
	
	g_Track.addObserver( descr );
      }
      else
      {
	TrackGlobal::catchSigPipe();

	TrackableHUB *hub = TrackableHUB::instance();
	hub->setEndpoint( endpoint[0].c_str(), std::atoi(endpoint[1].c_str()) );

	g_HUBAPI.setThreaded( true );
	
	std::string url( "http://" );
	url += endpoint[0];
	url += ":";
	if ( endpoint.size() > 2 )
	  url += endpoint[2];
	else
	  url += std::to_string(webserver_port-1);
	url += "/get?availableDevices=true";

	g_HUBAPIURL = url;
      }
    }
#endif
    else if ( strcmp(argv[i],"+oscAudio") == 0 )
    {
      std::string region( argv[++i] );

      KeyValueMap descr;
      descr.set( "type", "osc" );
      descr.set( "name", region.c_str() );
      descr.set( "maxFPS",  "1" );
      descr.set( "region", region.c_str() );
      descr.set( "filter", "frame=status,switch=1,region=" );
      parseArg( i, argv, argc, descr );

      std::string url( "osc.udp://" );
      url += argv[++i];
      descr.set( "url", url.c_str() );

      g_Track.addObserver( descr );
    }
    else if ( strcmp(argv[i],"+udpSwitch") == 0 )
    {
      std::string region( argv[++i] );
      std::string name( "udp_sw_" );
      name += region;

      KeyValueMap descr;
      descr.set( "type", "udp" );
      descr.set( "name", name.c_str() );
      descr.set( "maxFPS",  "1" );
      descr.set( "region", region.c_str() );
      descr.set( "scheme", "(frame_begin ? <switch> == 1) lidar/switch <region> <switch>" );
      parseArg( i, argv, argc, descr );

      descr.set( "url", argv[++i] );

      g_Track.addObserver( descr );
    }
    else if ( strcmp(argv[i],"+observer") == 0 )
    {
      KeyValueMap descr;
      parseArg( i, argv, argc, descr );
      
      if ( !g_RunningMode.empty() )
	descr.set( "runMode", g_RunningMode.c_str() );

      g_Track.addObserver( descr );
    }
    else if ( strcmp(argv[i],"+uniteBlobs") == 0 )
    { 
      g_Track.uniteMethod = LidarTrack::UniteBlobs;
    }
    else if ( strcmp(argv[i],"+uniteStages") == 0 )
    { 
      g_Track.uniteMethod = LidarTrack::UniteStages;
    }
    else if ( strcmp(argv[i],"+uniteObjects") == 0 )
    { 
      g_Track.uniteMethod = LidarTrack::UniteObjects;
    }
    else if ( strcmp(argv[i],"+radialDisplacement") == 0 )
    { 
      LidarTrack::setRadialDisplacement( std::atof( argv[++i] ) );
    }
    else if ( strcmp(argv[i],"+useRegions") == 0 )
    {}
    else if ( strcmp(argv[i],"+fps") == 0 )
    { 
      g_MaxFps = std::atof( argv[++i] );
    }
    else if ( strcmp(argv[i],"+bluePrint") == 0 )
    {
      bluePrintFileName = argv[++i];
      bluePrintExtent   = argv[++i];

      setBluePrints( true );
    }
    else if ( strcmp(argv[i],"+simulationEnvMap") == 0 )
    {
      simulationEnvMapFileName = argv[++i];
    }
    else if ( strcmp(argv[i],"+trackOcclusionMap") == 0 )
    {
      trackOcclusionMapFileName = argv[++i];
    }
    else if ( strcmp(argv[i],"+obstacle") == 0 )
    {
      obstacleFileName = argv[++i];
      obstacleExtent   = argv[++i];
    }
    else if ( strcmp(argv[i],"+useBlueprints") == 0 )
    {
      setBluePrints( true );
    }
    else if ( strcmp(argv[i],"+spinningReportScript") == 0 )
    { 
      g_SpinningReportScript = TrackGlobal::configFileName( argv[++i] );
    }
    else if ( strcmp(argv[i],"+spinningReportSec") == 0 )
    { 
      g_SpinningReportSec = ::atoi( argv[++i] );
    }
    else if ( strcmp(argv[i],"+failureReportScript") == 0 )
    { 
      g_FailureReportScript = TrackGlobal::configFileName( argv[++i] );
    }
    else if ( strcmp(argv[i],"+failureReportSec") == 0 )
    { 
      g_FailureReportSec = ::atoi( argv[++i] );
    }
    else if ( strcmp(argv[i],"+virtual") == 0 )
    { 
      const char *deviceName = argv[++i];
      
      device()->outVirtUrl = deviceName;
    }
    else if ( strcmp(argv[i],"+file") == 0 )
    { 
      const char *fileName = argv[++i];
      
      device()->outFileName = fileName;
    }
    else if ( strcmp(argv[i],"-openOnStart") == 0 )
    { 
      g_OpenOnStart = false;
    }
    else if ( strcmp(argv[i],"+errorLogFile") == 0 )
    { 
      const char *fileName = argv[++i];
      g_ErrorLogFile = fileName;
      TrackGlobal::setErrorFileName( TrackGlobal::configFileName(fileName).c_str() );
    }
    else if ( strcmp(argv[i],"+logFile") == 0 )
    { 
      const char *fileName = argv[++i];
      g_LogFile = fileName;
      TrackGlobal::setLogFileName( TrackGlobal::configFileName(fileName).c_str() );
    }
    else if ( strcmp(argv[i],"+notificationScript") == 0 )
    { 
      const char *notificationScript = argv[++i];
      TrackGlobal::setNotificationScript( TrackGlobal::getConfigFileName(notificationScript).c_str() );
    }
    else if ( g_Track.m_Stage->parseArg( i, (const char **) argv, argc ) )
    {
    }
    else if ( g_Devices.parseArg( i, (const char **) argv, argc ) )
    {
    }
    else if ( dummyDevice->parseArg( i, (const char **) argv, argc ) )
    {
    }
    else if ( strcmp(argv[i],"+v") == 0 )
    { 
    }
    else 
    {
      TrackGlobal::error( "unknown option: %s", argv[i] );  
      delete dummyDevice;
      exit( 0 );
    }
  }

  if ( g_Devices.isSimulationMode() )
  {
    if ( !simulationEnvMapFileName.empty() )
      setSimulationEnvMap();

    if ( g_UseObstacle && !obstacleFileName.empty() )
    { if ( !fileExists( TrackGlobal::getConfigFileName(obstacleFileName.c_str()).c_str() ) )
	TrackGlobal::error( "obstacle image does not exist: %s", obstacleFileName.c_str()  );
      else
	setObstacles();
    }
    if ( obstacleImg.width() > 0 )
    { LidarDevice::obstacleSimulationRay 	  = obstacleSimulationRay;
      LidarDevice::obstacleSimulationCheckOverlap = obstacleSimulationCheckOverlap;
    }
  }

      // out of center
  obsMatrix.w.x += 0.5;
  obsMatrix.w.y += 1.0;
  obsMatrixInv = obsMatrix.inverse();

  if ( !trackOcclusionMapFileName.empty() )
  {
    if ( !fileExists( TrackGlobal::getConfigFileName(trackOcclusionMapFileName.c_str()).c_str() ) )
      TrackGlobal::error( "trackOcclusionMap image does not exist: %s", trackOcclusionMapFileName.c_str()  );
    else
      setTrackOcclusionMap();
  }
  usedGroupsString( groupName );

  if ( !g_IsHUB && !groupName.empty() )
    LidarDeviceGroup::resolveDevices( resolveDevice );
  
  if ( !g_PackedInFileName.empty() )
  {
    PackedPlayer *player = new PackedPlayer();
    if ( !player->open( g_PackedInFileName.c_str() ) )
    { std::cerr << "Error opening file " << g_PackedInFileName.c_str() << "\n";
      exit( 1 );
    }

    TrackBase::setPackedPlayer( player );
  }
  else if ( !g_IsHUB && g_Devices.size() == 0 ) // create a device if not already created
    device();

  g_Devices.loadRegistration( true );
  g_Devices.copyArgs( dummyDevice );

  delete dummyDevice;

  g_Devices.setObjectTracking( g_DoTrack );

  if ( powerOff )
  { for ( int d = 0; d < g_Devices.size(); ++d )
    {
      g_Devices[d]->powerOff = powerOff;
      g_Devices[d]->deviceId = d;
      if ( g_Devices[d]->openDevice() )
      { sendToInVirtual( *g_Devices[d], "/stop" );
//	g_Devices[d]->closeDevice();
      }
    }
    
    while ( loopPowerOff )
      usleep( 1000 * 1000 );

    exit( 0 );
  }

  for ( int d = 0; d < g_Devices.size(); ++d )
  { g_Devices[d]->deviceId = d;
    if ( g_Devices[d]->outVirtUrl.empty() )
      g_Devices[d]->readEnv();
  }

  if ( !groupName.empty() )
    activateGroup( groupName.c_str(), false );
  else
    activateGroup( "all", false );
    
  setPlayerSyncTime();

  log( "RUN Run Application" );
  TrackGlobal::notification( "run", "message=\"Run Application\" runMode=%s verbose=%s", g_RunningMode.c_str(), g_Verbose?"true":"false" );

  if ( !g_SpinningReportScript.empty() )
  { resolveSensorIN();
    readSensorIN();
  }
  
  LidarDeviceList devices;
    
  Lidar::exitHook = exitHook;
  g_IsStarted = false;
  if ( TrackBase::packedPlayer() == NULL && g_OpenOnStart )
  { 
    log( withRunningMode("START on application start").c_str() );
    TrackGlobal::notification( "start", "message=\"Start on application start\" runMode=%s verbose=%s", g_RunningMode.c_str(), g_Verbose?"true":"false" );

    devices = g_Devices.activeDevices();
    for ( int d = 0; d < devices.size(); ++d )
    {
      devices[d]->open();
      
      if ( !g_AvailableDevices.empty() )
	g_AvailableDevices += ",";
      g_AvailableDevices += devices[d]->getNikName();
    }
    
    for ( int d = 0; d < devices.size(); ++d )
      sendToInVirtual( *devices[d], "/start" );

    g_IsStarted = true;
  }

  if ( webserver_port > 0 )
    runWebServer();

  atexit( exit_handler );
  
  g_Track.markUsedRegions();
  
  uint64_t usecPerFrame = 1000 * 1000 / g_MaxFps;
  uint64_t updateFailureTime  = 0;
  uint64_t updateSpinningTime = 0;
  
  bool trackStarted = false;

  while ( webserv == NULL || webserv->is_running() )
  {
    uint64_t startTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

#if USE_WEBSOCKETS
    if ( g_IsHUB )
    {
      if ( g_HUBAPI.isReady() )
      {
	if ( g_HUBAPI.hasResponded() )
        {
	  //	  printf( "hasResponded 1\n" );

	  if ( g_HUBAPI.hasReturnData() )
	  {
	    //	    printf( "hasReturnData 1\n" );

	    rapidjson::Document json;
	    if ( json.Parse<0>(g_HUBAPI.returnDataStr().c_str()).HasParseError() )
	      TrackGlobal::error( "%s parse error", g_HUBAPI.returnDataStr() );
	    else
	    {
	      //	      printf( "parse ok 1\n" );
	      
	      if ( json.IsObject() )
		rapidjson::fromJson( json, "availableDevices", g_AvailableDevices );
	      else if ( json.IsArray() )
	      {
		for ( int i = 0; i < ((int)json.Size()); ++i )
                { rapidjson::Value &valJson( json[i] );
		  if ( rapidjson::fromJson( valJson, "availableDevices", g_AvailableDevices ) )
		    break;
		}
	      }
	    //	    printf( "available(%s) returned: %s\n", g_HUBAPIURL.c_str(), g_AvailableDevices.c_str() );

	      //	      printf( "parse ok 2\n" );
	      
	      std::vector<std::string> avDev( split(g_AvailableDevices,',') );
	      std::set<std::string> availableDevices;
	
	      for ( int i = 0; i < avDev.size(); ++i )
		availableDevices.emplace( avDev[i] );

	      g_Track.updateOperational( availableDevices );

	      //	      printf( "parse ok 3\n" );
	    }

	    //	    printf( "hasReturnData 2\n" );
	  }

	  //	  printf( "hasResponded 2\n" );

	  g_HUBAPI.clearReturnData();
	}

	if ( startTime - updateFailureTime > 3000000 ) // every 3 seconds
        {
	  g_HUBAPI.get( g_HUBAPIURL.c_str() );
	  updateFailureTime = startTime;
	}
      }
      
      TrackableHUB::instance()->update();
    }
    else
#endif
    {
      g_Devices.update();
    
      if ( startTime - updateFailureTime > 500000 ) // every half second
      { 
	if ( g_IsStarted )
	  updateFailures();
	std::set<std::string> availableDevices( getAvailableDevices() );

	g_AvailableDevices = "";
	for ( auto &deviceName: availableDevices )
        { if ( !g_AvailableDevices.empty() )
	    g_AvailableDevices += ",";
	  g_AvailableDevices += deviceName;
	}

	g_Track.updateOperational( availableDevices );
	updateFailureTime = startTime;
      }

      if ( (startTime - updateSpinningTime)/1000000 > g_SpinningReportSec )
      { reportSpinning();
	updateSpinningTime = startTime;
      }

//    printf( "%d %d %d\n", (g_DoTrack && !playerIsPaused()), (g_LidarInFileTemplate.empty() && g_PackedInFileName.empty()), playerTimeStamp() != 0 );

      if ( (g_DoTrack && !playerIsPaused()) &&
	   ((g_LidarInFileTemplate.empty() && g_PackedInFileName.empty()) || playerTimeStamp() != 0) )
      {
	if ( !trackStarted )
        { if ( g_OpenOnStart )
	    g_Track.start( playerTimeStamp() );
	  else
	    g_Track.startAlwaysObserver( playerTimeStamp() );
	  trackStarted = true;
	}

	g_TrackMutex.lock();

	bool isEnvScanning = false;	
	for ( int d = 0; d < devices.size(); ++d )
	  if ( devices[d]->isEnvScanning )
          { isEnvScanning = true;
	    break;
	  }

	if ( g_Devices.isRegistering || g_Devices.isCalculating || isEnvScanning )
	  g_Track.reset();
	else
	  g_Track.track( g_Devices, playerTimeStamp() );
      
	g_TrackMutex.unlock();
      }
    }
    
    webMutex.lock();
    cleanupPainter();
    webMutex.unlock();

    if ( playExitAtEnd && playerAtEnd() )
      break;

    uint64_t currentTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t diff        = currentTime - startTime;

    if ( diff < usecPerFrame-300 && usecPerFrame-diff-200 > 0 )
      usleep( usecPerFrame-diff-200 );

//    webMutex.lock();
    frameRate.tick( getmsec() );
//    webMutex.unlock();
  }
  
  webMutex.lock();
  cleanupPainter();

  if ( g_DoTrack )
    g_Track.stop( playerTimeStamp() );
  g_IsStarted = false;

  Lidar::exit();

  webMutex.unlock();

  return 0;
}



