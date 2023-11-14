// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef _LIDAR_KIT_H_
#define _LIDAR_KIT_H_

/***************************************************************************
*** 
*** DEBUG
***
****************************************************************************/

#define DEBUG 0

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif
#include <csignal>
#include <functional>
#include <mutex>
#include <thread>

#include "keyValueMap.h"
#include "Vector.h"

#include "rplidar.h" //RPLIDAR standard sdk, all-in-one header
#include "sl_lidar.h"

#include "ldlidarDriver.h"
#include "ydlidarDriver.h"
#include "lslidarDriver.h"
#include "mslidarDriver.h"

#include "lidarVirtDriver.h"
#include "lidarFileDriver.h"

/***************************************************************************
*** 
*** DECLARATIONS
***
****************************************************************************/

class FPS
{
public:
  uint64_t last;
  int      fps;
  int      count;
  
  int      numFrames;
  
  FPS( int frames=1 ) 
    : last     ( 0 ),
      fps      ( 0 ),
      count    ( 0 ),
      numFrames( frames )
  {}
  
  void	tick( uint64_t now=0 )
  {
    if ( now == 0 )
      now = getmsec();

    if ( ++count >= numFrames )
    { count = 0;
      if ( now-last > 0 )
	fps = numFrames * 1000 / (now-last);
      last = now;
    }
  }
};

class AFPS : public FPS
{
  public:
  AFPS( int frames=8 ) : FPS( frames ) {}  
};


class ASAMPLES
{
public:
  int      	   numFrames;
  std::vector<int> samples;

  ASAMPLES( int frames=15 ) 
    : numFrames( frames )
  {}
  
  void	tick( int numSamples )
  {
    if ( samples.size() >= numFrames )
      samples.erase(samples.begin());

    samples.push_back( numSamples );
  }

  int	average( int defaultResult=1150 ) const
  {
    if ( samples.size() == 0 )
      return defaultResult;
    
    long int sum = 0;
    for ( int i = samples.size()-1; i >= 0; --i )
      sum += samples[i];
    
    return sum / samples.size();
  }
  
};


/***************************************************************************
*** 
*** LidarObject
***
****************************************************************************/

class LidarSampleBuffer;

class LidarObject
{
public:
  int		lowerIndex;
  int		higherIndex;
  int		oid;
  int		user;
  bool		isSplit;
  float		confidence;
  float		personSized;
  float		curvature;
  float		extent;
  float		closest;
  float		angle;
  Vector3D	center;
  Vector3D	lowerCoord;
  Vector3D	higherCoord;
  Vector3D	normal;
  uint64_t	timeStamp;

  std::vector<Vector2D> curvePoints;

  LidarObject( int lower=0, int higher=0, float extent=0.0f, int oid=0 )
  : lowerIndex ( lower ),
    higherIndex( higher ),
    oid        ( oid ),
    isSplit    ( false ),
    confidence ( 0.0 ),
    personSized( 0.0 ),
    curvature  ( 0.0 ),
    extent     ( extent ),
    closest    ( 0.0 ),
    angle      (),
    center     (),
    normal     ( 0.0, 1.0, 0.0 ),
    lowerCoord (),
    higherCoord(),
    timeStamp  ( 0 )
    {}
  
  float lineScatter( LidarSampleBuffer &sampleBuffer ) const;
  void  calcCurvature( LidarSampleBuffer &sampleBuffer );
  
  float distance( const LidarObject &other ) const
  {
    float distance0 = lowerCoord.distance(other.lowerCoord)  + higherCoord.distance(other.higherCoord);
    float distance1 = lowerCoord.distance(other.higherCoord) + higherCoord.distance(other.lowerCoord);

    return distance0 < distance1 ? distance0 : distance1;
  }
  
  float minDistance( const LidarObject &other ) const
  {
    float d, distance = lowerCoord.distance(other.lowerCoord);
    if ( (d=lowerCoord.distance(other.center)) < distance ) distance = d;
    if ( (d=lowerCoord.distance(other.higherCoord)) < distance ) distance = d;

    if ( (d=center.distance(other.lowerCoord)) < distance ) distance = d;
    if ( (d=center.distance(other.center)) < distance ) distance = d;
    if ( (d=center.distance(other.higherCoord)) < distance ) distance = d;

    if ( (d=higherCoord.distance(other.lowerCoord)) < distance ) distance = d;
    if ( (d=higherCoord.distance(other.center)) < distance ) distance = d;
    if ( (d=higherCoord.distance(other.higherCoord)) < distance ) distance = d;

    return distance;
  }
  
  void update()
  {
    center = (lowerCoord+higherCoord) * 0.5;
    angle  = ((Vector2D&)center).angle();
  }
  
  LidarObject &operator +=( const Vector3D &offset )
  {
    lowerCoord  += offset;
    higherCoord += offset;
    update();
    return *this;
  }
  
  LidarObject &operator *=( const Matrix3H &matrix )
  {
    lowerCoord  *= matrix;
    higherCoord *= matrix;
    update();
    return *this;
  }
  
  static float	maxMarkerDistance;

};

class LidarObjects : public std::vector<LidarObject>
{
protected:
  static bool compareAngle(const LidarObject& o1, const LidarObject& o2) { return o1.angle < o2.angle; }
  static bool compareSize (const LidarObject& o1, const LidarObject& o2) { return o1.lowerCoord.distance(o1.higherCoord) < o2.lowerCoord.distance(o2.higherCoord); }

  float angleOfMostDistantCoord() const;
  
public:

  class Marker : public std::vector<LidarObjects>
  {
    public:
    float calcTransformTo( const Marker &other, Matrix3H &meMatrix, Matrix3H &otMatrix ) const;
  };
  
  

  LidarObjects &operator +=( const Vector3D &offset );
  LidarObjects &operator *=( const Matrix3H &matrix );

  void	sortByAngle()
  { std::sort( begin(), end(), compareAngle );
  }

  void	sortBySize()
  { std::sort( begin(), end(), compareSize );
  }

  float distance( const LidarObjects &other ) const;

  void		setTimeStamp( uint64_t timestamp );

  Vector3D      calcCenter() const;
  void		calcCurvature       ( LidarSampleBuffer &sampleBuffer );
  
  
  bool		calcRotationTo      ( const LidarObjects &other, float &minAngle, float &minDistance, float angleOffset=0.0f ) const;
  bool		calcRotationRangeTo ( const LidarObjects &other, float &minAngle, float &minDistance, float angleRange, float &angleOffset, int numSamples ) const;
  bool		calcTransformTo     ( const LidarObjects &other, Matrix3H &meMatrix, Matrix3H &otMatrix, float &minDistance ) const;
  
  LidarObjects  unscatter( LidarSampleBuffer &sampleBuffer ) const;
  
  Marker 	getMarker( LidarSampleBuffer &sampleBuffer ) const;

};


/***************************************************************************
*** 
*** LidarSample
***
****************************************************************************/

class LidarSample
{
public:
  Vector3D	coord;
  float		angle;
  float		distance;
  int		quality;
  
  int		oid;
  int		accumCount;
  int		sourceIndex;
  int		sourceQuality;
  bool		touched;
  

  LidarSample()
  : coord   ( 0, 0, 0 ),
    angle   ( 0 ),
    distance( 0 ),
    quality ( 0 ),
    oid     ( 0 ),
    accumCount( 0 ),
    sourceIndex( 0 ),
    sourceQuality( 0 ),
    touched( false )
  {}
  
  bool isValid() const
  { return quality > 0; }
  
  bool read ( std::ifstream &stream );
  bool write( std::ofstream &stream );

};

/***************************************************************************
*** 
*** LidarSampleBuffer
***
****************************************************************************/

typedef std::vector<bool> LidarSampleValidBuffer;


class LidarSampleBuffer : public std::vector<LidarSample> 
{
public:
#if DEBUG
  LidarSampleBuffer( int size=3*360 )
#else
  LidarSampleBuffer( int size=3*1024 )
#endif
  : std::vector<LidarSample>( size, LidarSample() )
  {}
  
  LidarSampleBuffer &operator +=( const Vector3D &offset );
  LidarSampleBuffer &operator *=( const Matrix3H &matrix );

  bool read ( std::ifstream &stream );
  bool write( std::ofstream &stream );

  bool read ( const char *fileName );
  bool write( const char *fileName );
};


/***************************************************************************
*** 
*** LidarBasisChange
***
****************************************************************************/

class LidarBasisChange
{
public:
  Matrix3H 			matrix;
  int 				valid;
  float				error;

  LidarBasisChange()
  : matrix(),
    valid( false ),
    error( 0.0 ) 
  {}
  
};


/***************************************************************************
*** 
*** LidarBasisChanges
***
****************************************************************************/

class LidarBasisChanges : public std::vector<LidarBasisChange>
{
public:
};

/***************************************************************************
*** 
*** LidarSpec
***
****************************************************************************/

class LidarSpec 
{
public:
  float maxRange;
  int	numSamples;
  float scanFreq;
  int   minQuality;
  int   envMinQuality;

  LidarSpec()
  : maxRange     ( 9.0 ),
    numSamples   ( 439 ),
    scanFreq     ( 9.5 ),
    minQuality   ( 0 ),
    envMinQuality( 0 )
  {}
  

};

/***************************************************************************
*** 
*** LidarDevice
***
****************************************************************************/

class LidarDevice
{
public:

  enum CheckPointMode {

    NoCheckPoint           = 0,
    ReadCheckPoint         = (1<<0),
    WriteCheckPoint        = (1<<1),
    CreateCheckPoint       = (1<<2),
    WriteCreateCheckPoint  = (WriteCheckPoint|CreateCheckPoint)
  
  };

  static const char *RPLidarTypeName;
  static const char *YDLidarTypeName;
  static const char *LDLidarTypeName;
  static const char *LSLidarTypeName;
  static const char *MSLidarTypeName;
  static const char *UndefinedTypeName;
  
  enum DriverType
  {
    UNDEFINED = 0,
    RPLIDAR   = 1,
    YDLIDAR   = 2,
    LDLIDAR   = 3,
    LSLIDAR   = 4,
    MSLIDAR   = 5
  };

  enum ConnectionType
  {
    UNKNOWN   = 0,
    UART      = 1,
    USB       = 2
  };

  rp::standalone::rplidar::RPlidarDriver *rpSerialDrvStopped;
  rp::standalone::rplidar::RPlidarDriver *rpSerialDrv;
  YDLidarDriver 			 *ydSerialDrv;
  LDLidarDriver 			 *ldSerialDrv;
  LSLidarDriver 			 *lsSerialDrv;
  MSLidarDriver 			 *msSerialDrv;

  LidarVirtualDriver	 *inDrv;
  LidarVirtualDriver	 *outDrv;
  LidarInFile 		 *inFile;
  LidarOutFile 		 *outFile;

  ConnectionType	 connectionType;
  DriverType		 driverType;
  std::string		 deviceType; 
  
  std::string		 deviceName;  
  std::string		 inVirtUrl;  
  std::string		 inVirtHostName;  
  std::string		 outVirtUrl;  
  std::string		 inFileName;  
  std::string		 outFileName;  
  std::string		 envFileName;  
  std::string		 matrixFileName;  
  std::string		 nikName;  
  std::string		 baseName;
  std::string		 idName;
  std::string		 sensorIN;
  int			 inVirtPort;
  bool			 inVirtSensorPower;
  
  struct 
  {
    std::string		 scanMode;  
    int			 scanModeId;
    rp::standalone::rplidar::RplidarScanMode 	 outUsedScanMode;
  } rplidar;
  
  YDLidarParam		 ydlidar;
  
  _u32 			 baudrateOrPort;
  _u16 			 motorPWM;
  _u16 			 currentMotorPWM;
  float			 motorSpeed;
  float			 currentMotorSpeed;
  bool			 usePWM;
  int			 pwmChip;
  int			 pwmChannel;
  bool			 motorState;
  bool			 motorCtrlSupport;
  bool			 isPoweringUp;
  bool   		 powerOff;
  bool   		 ready;
  bool   		 dataReceived;
  bool   		 isSimulationMode;
 
  std::string		 errorMsg; 
  
  int 			 sampleBufferIndex;
  std::vector<LidarSampleBuffer>      samples;
  LidarObjects 	 	 objects;

  std::mutex		 mutex;
  std::thread		*thread;
  bool			 exitThread;
  
  uint64_t 		 timeStamp;
  uint64_t 		 openTime;
  uint64_t 		 startTime;
  uint64_t 		 receivedTime;
  uint64_t 		 processStartTime;
  uint64_t 		 motorStartTime;
  uint64_t 		 reopenTime;

  int			 oidCount;
  int			 oidMax;
  
  LidarBasisChanges	 basisChanges;
  
  struct Info 
  {
    rplidar_response_device_info_t devinfo;
    int          samplesPerScan;
    FPS          fps;
    AFPS 	 average_fps;
    ASAMPLES	 average_samples;
    std::string	 detectedDeviceType; 
    DriverType	 detectedDriverType;
    LidarSpec    spec;
    
    void tick( uint64_t now=0 )
    {
      if ( now == 0 )
	now = getmsec();
      fps.tick( now );
      average_fps.tick( now );
      average_samples.tick( samplesPerScan );
    }

  } info;
  
  struct TrackInfo
  {
    double distance;
    int    detectedIndex;
    int    objectIndex;
  };

  class UARTAutoPower 
  {
    public:
    LidarDevice *device;
    UARTAutoPower( LidarDevice *device ) : device( device )
    { device->setUARTPower( true ); }
    ~UARTAutoPower() 
    { device->setUARTPower( false ); }
  };
  
  bool			 setUARTPower( bool on, const char *deviceName=NULL );

  static bool 		 compareTrackInfo(const TrackInfo& t1, const TrackInfo& t2) { return t1.distance < t2.distance; }
  bool			 setDeviceParam( std::map<std::string, std::string> map );
  bool			 setDeviceDefaultParam( const char *deviceType );

  bool			 openVirtualDevice( LidarVirtualDriver *&virtDrv, const char *deviceName, bool isInDevice );
  bool 			 openLocalDevice();
  bool 			 openDevice();

  void 			 closeLocalDevice();
  void 			 closeVirtualDevice( LidarVirtualDriver *&virtDrv, std::string &url );
  void 			 closeDevice();

  bool			 scanSimulation( LidarRawSampleBuffer &sampleBuffer, float distance, int numSamples, float scanFreq, bool coverage=true );
  bool			 scanSimulation( LidarRawSampleBuffer &sampleBuffer, bool coverage=true );

  bool 			 openDeviceRplidar( bool tryOpen=false );
  void 			 closeDeviceRplidar();
  bool 			 scanRplidar( LidarRawSampleBuffer &sampleBuffer );
  bool 			 guessDeviceTypeRplidar( Info &info );
  bool 			 getInfoRplidar ( Info &info, const char *deviceName, bool dumpInfo );

  bool 			 openDeviceYDLidar( bool tryOpen=false );
  bool 			 scanYDLidar( LidarRawSampleBuffer &sampleBuffer );
  bool 			 getInfoYDLidar ( Info &info, const char *deviceName, bool dumpInfo );
  void 			 closeDeviceYDLidar();

  bool 			 openDeviceLDLidar( bool tryOpen=false );
  LDLidarDriver 	*openLDLidarDriver( const char *model="ld06" );
  bool 			 scanLDLidar( LidarRawSampleBuffer &sampleBuffer );
  bool 			 getInfoLDLidar ( Info &info, const char *deviceName, bool dumpInfo );
  void 			 closeDeviceLDLidar();

  bool 			 openDeviceLSLidar( bool tryOpen=false );
  bool 			 scanLSLidar( LidarRawSampleBuffer &sampleBuffer );
  bool 			 getInfoLSLidar ( Info &info, const char *deviceName, bool dumpInfo );
  void 			 closeDeviceLSLidar();

  bool 			 openDeviceMSLidar( bool tryOpen=false );
  bool 			 scanMSLidar( LidarRawSampleBuffer &sampleBuffer );
  bool 			 getInfoMSLidar ( Info &info, const char *deviceName, bool dumpInfo );
  void 			 closeDeviceMSLidar();

  static std::map<std::string, std::string> readDeviceParam( const char *type );

  bool 			 readDeviceParam( std::ifstream &stream );
  static const char	*driverTypeString( int driverType ); 
  const char		*driverTypeString() const; 

  void			 setSpec( const char *type, float maxRange, int numSamples, float scanFreq );
  void			 setSpec( int driverType, const char *deviceType );
  void			 setSpec( const char *deviceType );

  float 		 calcEnvConfidence( const LidarSample &sample ) const;

  bool			 setDeviceType( const char *deviceType );
  std::string 	 	&getVirtualHostName();  
  std::string 	 	 getDeviceType() const;
  ConnectionType	 getConnectionType( const char *devName=NULL );
  static std::string 	 resolveDeviceName( const char *deviceName );  
  static std::string 	 getBaseName( std::string &deviceName, const std::string inVirtUrl, bool asFileName=false );  
  static std::string 	 getBaseName( std::string &deviceName, bool asFileName=false );  
  std::string 		 getBaseName( bool asFileName=false );  
  std::string 		 getNikName ( bool asFileName=false );  
  std::string 		 getIdName  ();  
  static std::string 	 getDefaultSerialDevice( int id=-1 );  
  std::string 		 getEnvFileName();  
  std::string 		 getMatrixFileName();  

  bool			 checkInVirtHostName();
  
public:

#if DEBUG
  static const int numSamples       = 3*360;
#else
  static const int numSamples       = 3*1024;
#endif
  static const int numScanSamples   = 3*1024;
  static const int numSampleBuffers = 3;

  static const int defaultMotorPWM  = 600;
  const float defaultMotorSpeed     = 10.0f;

  static const int maxDevices       = 10;

  int			 deviceId;
  double		 char1;
  double		 char2;
  
  Matrix3H		 matrix;
  Matrix3H		 matrixInverse;
  Matrix3H		 deviceMatrix;
  Matrix3H		 viewMatrix;
  LidarSampleBuffer   &sampleBuffer( int i=-1 ) const;

  LidarSampleBuffer	 envSamples;
  LidarSampleBuffer	 envRawSamples;
  LidarSampleBuffer	 envErodedSamples;
  LidarSampleBuffer	 envDSamples;
  uint64_t 		*envTimeStamps;
    

  LidarSampleBuffer	 accumSamples;
  float 		 envThreshold;
  float	 		 envFilterMinDistance;
  float	 		 envScanSec;
  float 		 envAdaptSec;
  float	 		 envFilterSize;
  float	 		 objectMaxDistance;
  float	 		 objectMinExtent;
  float	 		 objectMaxExtent;
  float	 		 objectTrackDistance;

  bool		 	 shouldOpen;
  bool		 	 openFailed;
  bool		 	 dataValid;
  bool		 	 envValid;
  bool		 	 useEnv;
  bool		 	 useOutEnv;
  bool		 	 useOutEnvBak;
  bool		 	 envOutDirty;
  bool		 	 useTemporalDenoise;
  bool		 	 doObjectDetection;
  bool		 	 doObjectTracking;
  bool		 	 doEnvAdaption;
  
  bool		 	 isEnvScanning;
  bool		 	 isAccumulating;
  int			 maxAccumCount;
  
  float	 		 objectMaxDistanceBak;
  float	 		 objectMaxExtentBak;
  bool			 scanOnce;
  
  void setMatrix( const Matrix3H &matrix );
  void ThreadFunction();
  void updateEnv();
  void adaptEnv();
  void erodeEnv ( LidarSampleBuffer &srcSamples, LidarSampleBuffer &dstSamples, int steps=5  );
  void smoothEnv( LidarSampleBuffer &samples, int steps=5 );
  void processEnv();
  void envChanged();
  void sendOutEnv();

  int  angIndex( int angIndex ) const
  { return (angIndex+numSamples) % numSamples; }

  int  addAngIndex( int angIndex, int i ) const
  { return (angIndex+i+numSamples) % numSamples; }

  int angIndexByAngle( float angle ) const
  { return angIndex( round( angle / (2*M_PI) * (numSamples-1) ) ); }

  bool isEnvSample( LidarSample &sample ) const;
  bool isTempNoiseSample( int angIndex ) const;
  bool addDetectedObject( LidarObjects &objects, int lowerIndex, int higherIndex, bool isSplit=false );
  void addDetectedObject( LidarObjects &objects, int lowerIndex, int higherIndex, float extent, float closest, bool isSplit=false );
  void detectObjects();
  bool scanValid( int angIndex ) const;
  LidarObjects visibleObjects( const LidarObjects &other ) const;

public:
  LidarDevice();
  ~LidarDevice();

  bool isOpen( bool lock=true );
  bool isReady( bool lock=true );
  bool isLocalDevice() const;
  bool isVirtualDevice() const;
  bool isSpinning();
  bool isPoweringSupported();
  bool devicePoweringSupported();
  
  bool open();
  void close();

  void setMotorState( bool state );
  void setMotorPWM  ( int pwm );
  void setMotorSpeed( float speed );

  bool scan();

  bool writeEnv( const char *path=NULL, uint64_t timestamp=0 );
  bool readEnv ( const char *path=NULL );
  void resetEnv();

  bool writeMatrix( const char *path=NULL, uint64_t timestamp=0 );
  bool readMatrix ( const char *path=NULL );

  float angleByAngIndex( int angIndex ) const
  { return (angIndex+0.5) * (2*M_PI) / (double)(numSamples); }

  bool checkHealth();

  bool  isValid( int angIndex ) const;
  bool  coordVisible( const Vector3D &coord ) const;
  bool  coordVisible( float x, float y ) const;
  bool  getCoord( int angIndex, float &x, float &y ) const;
  int   getObjectId( int angIndex ) const;

  LidarDevice &operator *=( const Matrix3H &matrix );

  int   numDetectedObjects() const
  { return objects.size(); }
  
  LidarObject &detectedObject( int objectIndex )
  { return objects[objectIndex]; }

  void setDeviceMatrix( const Matrix3H &deviceMatrix );
  void setViewMatrix  ( const Matrix3H &viewMatrix   );
  void setCharacteristic( double char1, double char2=0.0, const char *devType=NULL );

  float	calcTransformTo( const LidarDevice &other, Matrix3H &meMatrix, Matrix3H &otMatrix, bool refine=false ) const;

  void lock();
  void unlock();
  
  void scanEnv();
  void setUseOutEnv ( bool useOutEnv );
  void cleanupAccum( int registerSec );
  void setAccum( bool set );
    
  bool getInfo ( Info &info, const char *deviceName=NULL, bool dumpInfo=false );
  static void dumpInfo( Info &info );
  void dumpInfo( const char *deviceName=NULL );
  bool parseArg( int &i, const char *argv[], int &argc );
  void printArgHelp() const;
  void copyArgs( LidarDevice *argDevice );

  static std::string getSerialNumber( LidarDevice::Info &info );
  std::string	getSerialNumber( const char *deviceName=NULL );

  static int  verbose();
  static void setVerbose( int level );
  static void setInstallDir( const char *path );
  static void setUseStatusIndicator( bool set );

  static std::string defaultDeviceType;
  static std::string configDir;
  static std::string configDirAlt;
  static std::string installDir;
  static std::string getConfigFileName( const char *fileName, const char *suffix=NULL, const char *path=NULL, CheckPointMode checkPointMode=NoCheckPoint, uint64_t timestamp=0 );

  static bool (*obstacleSimulationRay)( LidarDevice &device, LidarRawSample &sample, float &angle, float &distance );
  static bool (*obstacleSimulationCheckOverlap)( LidarDevice &device );

  static int64_t  fileDriverCurrentTime();
  static uint64_t fileDriverTimeStamp();
  static float    fileDriverPlayPos();
  static void     setFileDriverPlayPos( float playPos );
  static void     setFileDriverSyncTime( uint64_t timestamp=0 );
  static void     setFileDriverPaused( bool paused );
  static bool     fileDriverIsPaused();
  static bool     fileDriverAtEnd();
  std::string     getFileDriverFileName( const char *outFileTemplate, uint64_t timestamp=0 );
  
};


/***************************************************************************
*** 
*** LidarDeviceList
***
****************************************************************************/

class LidarDeviceList : public std::vector<LidarDevice*>
{
public:
  std::string	groupName;

  void		addMember   ( LidarDevice *device );
  bool		isMember    ( LidarDevice *device ) const;

};

/***************************************************************************
*** 
*** LidarDeviceGroup
***
****************************************************************************/

class LidarDeviceGroup : public LidarDeviceList
{
public:
  static void	addDevice   ( const char *groupName, const char *deviceName );
  static void	removeDevice( const char *groupName, const char *deviceName );
  static void	removeDevice( const char *deviceName );
  static void	removeGroup ( const char *groupName );
  static void	renameDevice( const char *groupName, const char *oldName, const char *newName );
  static void	renameDevice( const char *oldName, const char *newName );
  static void	renameGroup ( const char *oldName, const char *newName );
  static void	clearGroups ();

  static bool 	read 	    ( const char *fileName, bool reportError=false );
  static bool 	write	    ( const char *fileName );

  static void   resolveDevices( void(*resolveDevice)( LidarDevice *device, std::string &deviceName ) );

  static KeyValueMapDB groups;

};

/***************************************************************************
*** 
*** LidarDevices
***
****************************************************************************/

class LidarDevices : public std::vector<LidarDevice*>
{
public:

  float			envScanSec;
  float			registerSec;
  float			markerMatchDifference;

  uint64_t		startTime;
  bool			isRegistering;
  bool			isCalculating;
  bool			refineRegistration;
  bool			_useEnv;

  Matrix3H		viewMatrix;

  LidarDeviceList 	_activeDevices;
  LidarDeviceList 	_inactiveDevices;

  static std::string	message;

  void initBasisChanges();
  bool basisChangesComplete() const;
  bool calculateBasisChanges();
  bool deviceInGroup( LidarDevice &device, const char *groupName ) const;
  LidarDeviceList  runningOrAllDevices( bool all ) const;

  LidarDevices()
  : std::vector<LidarDevice*>(),
    envScanSec ( 15 ),
    registerSec( 10 ),
    markerMatchDifference( 0.2 ),
    isRegistering( false ),
    _useEnv      ( true )
  {}
  
  void		setViewMatrix( Matrix3H &matrix, bool all=false );
  void 		setAccum( bool set, bool all=false );
  void 		setSimulationMode( bool set );
  void 		setUseSimulationRange( bool set );
  void 		setObjectTracking( bool set );
  void 		startRegistration( bool refine=false );
  void 		calculateRegistion();
  void 		finishRegistration();
  void 		resetRegistration( bool all=false );
  void 		loadRegistration( bool all=false );
  void 		saveRegistration( bool all=false, uint64_t timestamp=0 );
  void 		setCharacteristic( double char1, double char2=0.0, const char *devType=NULL );
  void 		setUseOutEnv( bool useOutEnv );

  void 		scanEnv();
  void 		resetEnv( bool all=false );
  void 		loadEnv( bool all=false );
  void 		saveEnv( bool all=false, uint64_t timestamp=0 );
  void 		useEnv ( bool use );
  
  void		update();

  bool 		parseArg( int &i, const char *argv[], int &argc );
  void 		printArgHelp() const;
  void 		copyArgs( LidarDevice *argDevice );

  void		activateGroup( const char *groupName );
  bool		isActive( const char *groupName ) const;

  LidarDeviceList  &activeDevices();
  LidarDeviceList   runningDevices( bool onlyValidDevices=true ) const;
  LidarDeviceList   allDevices() const;
  LidarDeviceList   devicesInGroup( const char *groupName ) const;
  LidarDeviceList   remoteDevices () const;

  static bool 	    isSimulationMode();
  static void 	    setReadCheckPoint( const char *checkPoint );
};


/***************************************************************************
*** 
*** Lidar
***
****************************************************************************/

class Lidar
{
public:
  static bool	initialize();
  static void	exit();
  
  static void   setSignalHandlers();
  static void (*exitHook)       ();
  static void (*error)		( const char *format, ... );
  static void (*warning)	( const char *format, ... );
  static void (*log)  		( const char *format, ... );
  static void (*info)  		( const char *format, ... );
  static void (*notification)  	( const char *tags, const char *format, ... );

  static void setErrorFileName     ( const char *fileName );
  static void setLogFileName       ( const char *fileName );
  static void setNotificationScript( const char *scriptFileName );
};

#endif

