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

#include "ldlidar_driver.h"

#include "helper.h"
#include "ldlidarDriver.h"

#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include <iostream>
#include <fstream>

#include <termios.h>
#include <sys/ioctl.h>

#include <fcntl.h>

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

static int g_Verbose = false;
static const int minNoiseIntensity = 0;
static const int minDistance       = 200;
static const int maxNoiseDistance  = 1000;

class Lidar
{
public:
  static bool	initialize();

  static void (*error)( const char *format, ... );
  static void (*log)  ( const char *format, ... );
  static void (*info) ( const char *format, ... );
};

/***************************************************************************
*** 
*** Helper
***
****************************************************************************/

static std::string
pwmRead( int chip, int channel, const char *name )
{
  std::string result;

  int pwm, ret, length;
  char path[128];

  if ( channel < 0 )
    snprintf( path, sizeof(path), "/sys/class/pwm/pwmchip%d/%s", chip, name);
  else
    snprintf( path, sizeof(path), "/sys/class/pwm/pwmchip%d/pwm%d/%s", chip, channel, name);
    
  length = strlen( path );

  if ( length >= sizeof(path) )
  { Lidar::error( "path %s too long", name);
    return result;
  }

  std::ifstream stream( path );
  if ( stream.is_open() )
    stream >> result;
  
  return result;
}

static int 
pwmWrite( int chip, int channel, const char *name, const char *value, const char *deviceName )
{
  int pwm, ret, length;
  char path[128];

  if ( channel < 0 )
    snprintf( path, sizeof(path), "/sys/class/pwm/pwmchip%d/%s", chip, name);
  else
    snprintf( path, sizeof(path), "/sys/class/pwm/pwmchip%d/pwm%d/%s", chip, channel, name);
    
  length = strlen( path );

  if ( length >= sizeof(path) )
  { Lidar::error( "path %s too long", name);
    return -1;
  }

  if ( g_Verbose )
    Lidar::info( "LidarDevice(%s) pwmWrite %s -> %s", deviceName, value, path );
  
  pwm = open( path, O_WRONLY );
  if ( pwm < 0 )
  { 
    Lidar::error( "%s%s", path, strerror(errno) );
//    return -1;
  }

  ret = write( pwm, value, strlen(value) );

  if ( ret < 0 )
  { 
    Lidar::error( "%s%s", value, strerror(errno) );
    close( pwm );
    return -2;
  }

  close( pwm );

  return 0;
}

static int 
pwmWrite( int chip, const char *name, const char *value, const char *deviceName )
{
  return pwmWrite( chip, -1, name, value, deviceName );
}

/***************************************************************************
*** 
*** LDLidarDriver
***
****************************************************************************/

static uint64_t GetSystemTimeStamp(void) {
  std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> tp = 
    std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now());
  auto tmp = std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch());
  return ((uint64_t)tmp.count());
}


LDLidarDriver::LDLidarDriver( bool usePWM, int pwmChip, int pwmChannel, int offPWM, int onPWM, int period )
  : node( NULL ),
    usePWM( usePWM ),
    offPWM( offPWM ),
    onPWM ( onPWM ),
    period( period ),
    pwmChip   ( pwmChip ),
    pwmChannel( pwmChannel ),
    model( "ld06" )
{
}

LDLidarDriver::~LDLidarDriver()
{
  disconnect();
}

std::string
LDLidarDriver::sdkVersion()
{
  ldlidar::LDLidarDriver *node = new ldlidar::LDLidarDriver();
  
  std::string version( node->GetLidarSdkVersionNumber() );
  delete node;

  return version;
}


bool
LDLidarDriver::isOpen() const
{
  return node != NULL;
}  

bool
LDLidarDriver::connect( const char *DeviceName, const char *Model )
{
  deviceName = DeviceName;
  model      = Model;
  
  ldlidar::LDType type_name;
  uint32_t serial_baudrate = 0;

  if (model == "ld19") {
    serial_baudrate = 230400;
    type_name = ldlidar::LDType::LD_19;
  } else if (model == "st06" || model == "stl06p") {
    serial_baudrate = 230400;
    type_name = ldlidar::LDType::STL_06P;
  } else if (model == "st27" || model == "stl27l") {
    serial_baudrate = 921600;
    type_name = ldlidar::LDType::STL_27L;
  } else if (model == "st26" || model == "stl26") {
    serial_baudrate = 230400;
    type_name = ldlidar::LDType::STL_26;
  } else {
    serial_baudrate = 230400;
    type_name = ldlidar::LDType::LD_06; 
  }

  node = new ldlidar::LDLidarDriver();
  node->RegisterGetTimestampFunctional(std::bind(&GetSystemTimeStamp));
  node->EnableFilterAlgorithnmProcess( true );

  bool success = node->Start(type_name, deviceName, serial_baudrate, ldlidar::COMM_SERIAL_MODE);
  if ( !success )
    disconnect();
  
  return success;
}


bool
LDLidarDriver::isMotorSpeed( float speed )
{
  if ( !usePWM )
    return true;

  int pwm, ret;
  char path[128];
  snprintf( path, sizeof(path), "/sys/class/pwm/pwmchip%d/pwm%d", pwmChip, pwmChannel );

  if ( !fileExists( path ) )
    return false;
  
  int value = offPWM * (1.0-speed) + onPWM * speed;

  std::string period    ( std::to_string( this->period ) );
  std::string duty_cycle( std::to_string( value  ) );
  std::string one       ( "1" );

  if ( pwmRead( pwmChip, pwmChannel, "period" ) != period )
    return false;
  
  if ( pwmRead( pwmChip, pwmChannel, "duty_cycle" ) != duty_cycle )
    return false;
  
  if ( pwmRead( pwmChip, pwmChannel, "enable" ) != one )
    return false;

  return true;
}

void
LDLidarDriver::setMotorSpeed( float speed )
{
  if ( !usePWM )
    return;

  int pwm, ret;
  char path[128];
  snprintf( path, sizeof(path), "/sys/class/pwm/pwmchip%d/pwm%d", pwmChip, pwmChannel );

  if ( !fileExists( path ) )
  { std::string chanString( std::to_string(pwmChannel) );
    ret = pwmWrite( pwmChip, "export", chanString.c_str(), deviceName.c_str() );
    if (ret) return;
  }
  
  int value = offPWM * (1.0-speed) + onPWM * speed;

  std::string polarity  ( "normal" );
  std::string period    ( std::to_string( this->period ) );
  std::string duty_cycle( std::to_string( value  ) );
  std::string one       ( "1" );

  if ( pwmRead( pwmChip, pwmChannel, "polarity" ) != polarity )
  { ret = pwmWrite( pwmChip, pwmChannel, "polarity", polarity.c_str(), deviceName.c_str() );
//    if (ret) return;
  }
  
  if ( pwmRead( pwmChip, pwmChannel, "period" ) != period )
  { ret = pwmWrite( pwmChip, pwmChannel, "period", period.c_str(), deviceName.c_str() );
//    if (ret) return;
  }
  
  if ( pwmRead( pwmChip, pwmChannel, "duty_cycle" ) != duty_cycle )
  { ret = pwmWrite( pwmChip, pwmChannel, "duty_cycle", duty_cycle.c_str(), deviceName.c_str() );
//    if (ret) return;
  }
  
  if ( pwmRead( pwmChip, pwmChannel, "enable" ) != one )
  {
    ret = pwmWrite( pwmChip, pwmChannel, "enable", one.c_str(), deviceName.c_str() );
    ret = pwmWrite( pwmChip, pwmChannel, "polarity", polarity.c_str(), deviceName.c_str() );
  }
}


double
LDLidarDriver::getSpeed()
{
  if ( node == NULL )
    return 0.0;

  double lidar_spin_freq;
  if ( node->GetLidarSpinFreq(lidar_spin_freq) )
    return lidar_spin_freq;
  
  return 0.0;
}


void
LDLidarDriver::disconnect()
{ 
  if ( node == NULL )
    return;

  setMotorSpeed( 0.0 );
  
  node->Stop();
  delete node;
  node = NULL;
}


bool
LDLidarDriver::grabScanData( ScanData &data, int timeout )
{
  ldlidar::Points2D laserScan;

  double lidar_spin_freq;

  ldlidar::LidarStatus status = node->GetLaserScanData( laserScan, timeout );
  
  if ( status != ldlidar::LidarStatus::NORMAL )
    return false;
  
      /* filter single samples / noise */

  int size = laserScan.size();

  ldlidar::Points2D   filteredScan;
  filteredScan.resize( size );
  

  ldlidar::PointData *lastScanData = &laserScan[size-1];
  ldlidar::PointData *scanData     = &laserScan[0];
  ldlidar::PointData *nextScanData;

  for ( int i = 0; i < size; ++i )
  {
    filteredScan[i] = *scanData;

    nextScanData = &laserScan[(i+1)%size];

    if ( scanData->intensity > minNoiseIntensity )
    {
      if ( scanData->distance < minDistance )
	filteredScan[i].intensity = 0;
      else if ( lastScanData->intensity <= minNoiseIntensity && nextScanData->intensity <= minNoiseIntensity )
	filteredScan[i].intensity = 0;
      else if ( (lastScanData->intensity > minNoiseIntensity && abs(lastScanData->distance-scanData->distance) < maxNoiseDistance) ||
		(nextScanData->intensity > minNoiseIntensity && abs(nextScanData->distance-scanData->distance) < maxNoiseDistance) )
	;
      else
	filteredScan[i].intensity = 0;
    }
    
    lastScanData = scanData;
    scanData     = nextScanData;
  }

  data.resize( size );

  for ( int i = size-1; i >= 0; --i )
  {
//    ldlidar::PointData &scanData( laserScan[i] );
    ldlidar::PointData &scanData( filteredScan[i] );
    ScanPoint &sample   ( data[i] );
   
    sample.distance = scanData.distance / 1000.0;
    sample.angle    = scanData.angle;
    sample.quality  = scanData.intensity;
  }

  return true;
}

void
LDLidarDriver::setVerbose( int level )
{
  g_Verbose = level;
}

