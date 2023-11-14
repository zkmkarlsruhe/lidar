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

#include "src/CYdLidar.h"
#include <core/serial/common.h>
#include <map>
#include <core/math/angles.h>
#include <numeric>
#include <algorithm>
#if YDLIDARVERSION >= 110
#include "src/YDlidarDriver.h"
#else
#include "src/ydlidar_driver.h"
#endif
#include <math.h>
#include <functional>
#include <core/common/DriverInterface.h>
#include <core/common/ydlidar_help.h>
#include "src/ETLidarDriver.h"

using namespace ydlidar;

#include "ydlidarDriver.h"

#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>
#include <chrono>

#include <iostream>

#include <termios.h>
#include <sys/ioctl.h>

/***************************************************************************
*** 
*** DEFINES
***
****************************************************************************/

class Lidar
{
public:
  static bool	initialize();

  static void (*error)( const char *format, ... );
  static void (*log)  ( const char *format, ... );
};

/***************************************************************************
*** 
*** GLOBALS
***
****************************************************************************/

static int g_Verbose = false;

static int g_Baudrates[] = {
  115200,
  230400,
  512000,
  153600,
  128000

};

static const int g_DefaultFrequency = 10;

static struct YDLidarDeviceSpec	g_DeviceSpec[] = {

//LIDAR	     Model   Baudrate  SampleRate(K)   	Range(m)	Frequency(HZ)	Intensity(bit)	SingleChannel	LidarType	DeviceType	  SUPPORTED    TESTED

  { "F4",	1,	115200,	{4},4,		0.12,12,	5,12,5,		false,		false, 		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "S4",	4,	115200,	{4},4,		0.10,8.0,	5,12,5,		false,		false, 		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "S4B",	11,	153600,	{4},4,		0.10,8.0,	5,12,5,		true,		false, 		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "S2",	12,	115200,	{3},3,		0.10,8.0,	4,8,4,		false,		true, 		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false 	},
  { "G4",	5,	230400,	{4,8,9},9, 	0.1,16,		5,12,9,		false,		false, 		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  true 	},
  { "X4",	6,	128000,	{5},5,		0.12,10,	5,12,6,		false,		false , 	TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "X2",	6,	115200,	{3},3,		0.10,8.0,	4,8,4,		false,		true, 		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "X2L",	6,	115200,	{3},3,		0.10,8.0,	4,8,4,		false,		true, 		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false 	},
  { "G4PRO",	7,	230400,	{4,8,9},9,	0.1,16,		5,12,9,		false,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false 	},
  { "F4PRO",	8,	230400,	{4,6},6,	0.12,12,	5,12,8,		false,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "R2",	9,	230400,	{5},5,		0.12,16,	5,12,7,		false,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false 	},
  { "G6",	13,	512000,	{8,16,18},18,	0.1,25,		5,12,12,	false,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false 	},
  { "G2A",	14,	230400,	{5},5,		0.12,12,	5,12,7,		false,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false 	},
  { "G2",	15,	230400,	{5},5,		0.28,16,	5,12,7,		true,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false 	},
  
  { "G2C",	16,	115200,	{4},4,		0.1,12,		5,12,5,		false,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "G4B",	17,	512000,	{10},10,	0.12,16,	5,12,10,	true,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "G4C",	18,	115200,	{4},4,		0.1,12,		5,12,5,		false,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "G1",	19,	230400,	{9},9,		0.28,16,	5,12,9,		false,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},

  { "TX8", 	100,	115200,	{4},4,		0.1,8,		4,8,5,		false,		true,		TYPE_TOF,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "TX20", 	100,	115200,	{4},4,		0.1,20,		4,8,5,		false,		true,		TYPE_TOF,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "TG15",	100,	512000,	{10,18,20},20,	0.05,15,	3,16,10,	false,		false,		TYPE_TOF,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "TG30", 	101,	512000,	{10,18,20},20,	0.05,30,	3,16,10,	false,		false,		TYPE_TOF,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "TG50", 	102,	512000,	{10,18,20},20,	0.05,50,	3,16,10,	false,		false,		TYPE_TOF,	YDLIDAR_TYPE_SERIAL, true,  false	},
  { "T5", 	200,	8000,	{20},20,	0.05,15,	10,35,20,	true,		false,		TYPE_TOF_NET,	YDLIDAR_TYPE_TCP,    false, false	},
  { "T15", 	200,	8000,	{20},20,	0.05,15,	10,35,20,	true,		false,		TYPE_TOF_NET,	YDLIDAR_TYPE_TCP,    false, false	},
  { "T30", 	200,	8000,	{20},20,	0.05,30,	10,35,20,	true,		false,		TYPE_TOF_NET,	YDLIDAR_TYPE_TCP,    false, false	},
  
  { "TMINI", 	150,	230400,	{4},4,		0.05,12,	4,12,4,		true,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  true	},
  
};

// from https://github.com/YDLIDAR/YDLidar-SDK

/*
F4	1	115200	4	0.12~12	5~12	false	false	4.8~5.2
S4	4	115200	4	0.10~8.0	5~12 (PWM)	false	false	4.8~5.2
S4B	4/11	153600	4	0.10~8.0	5~12(PWM)	true(8)	false	4.8~5.2
S2	4/12	115200	3	0.10~8.0	4~8(PWM)	false	true	4.8~5.2
G4	5	230400	9/8/4	0.28/0.26/0.1~16	5~12	false	false	4.8~5.2
X4	6	128000	5	0.12~10	5~12(PWM)	false	false	4.8~5.2
X2/X2L	6	115200	3	0.10~8.0	4~8(PWM)	false	true	4.8~5.2
G4PRO	7	230400	9/8/4	0.28/0.26/0.1~16	5~12	false	false	4.8~5.2
F4PRO	8	230400	4/6	0.12~12	5~12	false	false	4.8~5.2
R2	9	230400	5	0.12~16	5~12	false	false	4.8~5.2
G6	13	512000	18/16/8	0.28/0.26/0.1~25	5~12	false	false	4.8~5.2
G2A	14	230400	5	0.12~12	5~12	false	false	4.8~5.2
G2	15	230400	5	0.28~16	5~12	true(8)	false	4.8~5.2
G2C	16	115200	4	0.1~12	5~12	false	false	4.8~5.2
G4B	17	512000	10	0.12~16	5~12	true(10)	false	4.8~5.2
G4C	18	115200	4	0.1~12	5~12	false	false	4.8~5.2
G1	19	230400	9	0.28~16	5~12	false	false	4.8~5.2
TX8 　	100	115200	4	0.1~8	4~8(PWM)	false	true	4.8~5.2
TX20 　	100	115200	4	0.1~20	4~8(PWM)	false	true	4.8~5.2
TG15 　	100	512000	20/18/10	0.05~30	3~16	false	false	4.8~5.2
TG30 　	101	512000	20/18/10	0.05~30	3~16	false	false	4.8~5.2
TG50 　	102	512000	20/18/10	0.05~50	3~16	false	false	4.8~5.2
T15 　	200	8000	20	0.05~15	10-35	true	false	4.8~5.2
T30 　	200	8000	20	0.05~30	10-35	true	false	4.8~5.2
*/

static struct YDLidarDeviceSpec	g_TryDeviceSpec[] = {

//LIDAR	Model	Baudrate  SampleRate(K)		Range(m)	Frequency(HZ)	Intenstiy(bit)	SingleChannel	LidarType	DeviceType	  SUPPORTED    TESTED

  { "TMINI", 	150,	230400,	{4},4,		0.05,12,	4,12,4,		true,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  true	},

  { "F4",	1,	115200,	{4},4,		0.12,12,	5,12,5,		false,		false, 		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},

  { "G4",	5,	230400,	{4,8,9},4, 	0.1,16,		5,12,9,		false,		false, 		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  true 	},

  { "X4",	6,	128000,	{5},5,		0.12,10,	5,12,6,		false,		false , 	TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},

  { "R2",	9,	230400,	{5},5,		0.12,16,	5,12,7,		false,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false 	},

  { "G1",	19,	230400,	{9},9,		0.28,16,	5,12,9,		false,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},

  { "G6",	13,	512000,	{8,16,18},18,	0.1,25,		5,12,12,	false,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false 	},

  { "G4B",	17,	512000,	{10},10,	0.12,16,	5,12,10,	true,		false,		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},

  { "S4B",	11,	153600,	{4},4,		0.10,8.0,	5,12,5,		true,		false, 		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false	},

  { "S2",	12,	115200,	{3},3,		0.10,8.0,	4,8,4,		false,		true, 		TYPE_TRIANGLE,	YDLIDAR_TYPE_SERIAL, true,  false 	},

  { "TX8", 	100,	115200,	{4},4,		0.1,8,		4,8,5,		false,		true,		TYPE_TOF,	YDLIDAR_TYPE_SERIAL, true,  false	},

  { "TG15",	100,	512000,	{10,18,20},20,	0.05,30,	3,16,10,	false,		false,		TYPE_TOF,	YDLIDAR_TYPE_SERIAL, true,  false	},

  { "T5", 	200,	8000,	{20},20,	0.05,15,	10,35,20,	true,		false,		TYPE_TOF_NET,	YDLIDAR_TYPE_TCP,    false, false	},
};

struct YDLidarDeviceSpec *
YDLidarDriver::getSpec( const char *modelName )
{
  if ( modelName == NULL )
    return NULL;

  std::string string( modelName );
  std::for_each(string.begin(), string.end(), [](char & c) { c = ::tolower(c); });
  
  for ( int i = sizeof(g_DeviceSpec)/sizeof(g_DeviceSpec[0])-1; i >= 0; --i )
  {
    std::string model( g_DeviceSpec[i].model );
    std::for_each(model.begin(), model.end(), [](char & c) { c = ::tolower(c); });
    
    if ( model == string )
      return &g_DeviceSpec[i];
  }
  
  return NULL;
}

  
struct YDLidarDeviceSpec *
YDLidarDriver::getSpec( int modelId )
{
  for ( int i = sizeof(g_DeviceSpec)/sizeof(g_DeviceSpec[0])-1; i >= 0; --i )
    if ( modelId == g_DeviceSpec[i].modelId )
      return &g_DeviceSpec[i];

  return NULL;
}

/***************************************************************************
*** 
*** YDLidarParam
***
****************************************************************************/

YDLidarParam::YDLidarParam( const char *modelName )
  :
  lidarType 	( TYPE_TRIANGLE ),
  deviceType	( YDLIDAR_TYPE_SERIAL ),
  baudrate  	( 230400 ),
  sampleRate	( 9 ),
  abnormalCheckCount( 4 ),
  fixedResolution( false ),
  reversion	( false ),
  inverted	( false ),
  autoReconnect	( true ),	    
  isSingleChannel( false ),
  intensity	( false ),
  supportMotorDtrCtrl( true ),
  supportHeartBeat( false ),
  maxAngle	(  180.0f ),
  minAngle	( -180.0f ),
  maxRange	( 64.f ),
  minRange	( 0.05f ),
  frequency	( 7 )
{
  setSpec( YDLidarDriver::getSpec( modelName ) );
}
  
void
YDLidarParam::setSpec( YDLidarDeviceSpec *spec )
{
  if ( spec == NULL )
    return;
  
  baudrate   	   = spec->baudrate;
  sampleRate 	   = spec->defaultSampleRate;
  isSingleChannel  = spec->singleChannel;
  intensity 	   = spec->intensity;
  maxRange 	   = spec->maxRange;
  minRange 	   = spec->minRange;
  frequency 	   = spec->defaultFrequency;

  if ( g_DefaultFrequency > 0 )
  { if ( g_DefaultFrequency > spec->maxFrequency )
      frequency = spec->minFrequency;
    else if ( g_DefaultFrequency < spec->minFrequency )
      frequency = spec->minFrequency;
    else
      frequency = g_DefaultFrequency;
  }

  lidarType        = spec->lidarType;
  deviceType       = spec->deviceType;
}

bool
YDLidarParam::isSerial() const
{
  return deviceType == YDLIDAR_TYPE_SERIAL;
}




/***************************************************************************
*** 
*** YDLidarDriver
***
****************************************************************************/


YDLidarDriver::YDLidarDriver()
  : param(),
    laser( NULL ),
    trying( false )
{
}

YDLidarDriver::~YDLidarDriver()
{
  disconnect();
}

bool
YDLidarDriver::isOpen() const
{ return false;
}  

bool
YDLidarDriver::pingDeviceInfo( const char *deviceName, int &model, int &firmware_version, int &hardware_version, uint8_t serialnum[16] )
{
  bool success = false;
  
  int size = sizeof(g_Baudrates) / sizeof(g_Baudrates[0]);

  int defaultBaudrate = param.baudrate;

  for ( int i = -1; !success && i < size; ++i )
  {
    if ( i >= 0 )
    { if ( g_Baudrates[i] == defaultBaudrate )
	++i;
      if ( i >= size )
	break;

      param.baudrate = g_Baudrates[i];
    }
    
    laser = new CYdLidar();
    laser->verbose = g_Verbose;
    laser->trying  = trying;

	/// ignore array
    std::string ignore_array;
    ignore_array.clear();
    laser->setlidaropt(LidarPropIgnoreArray, ignore_array.c_str(), ignore_array.size());

    std::string	 port( deviceName );
    laser->setlidaropt( LidarPropSerialPort, port.c_str(), port.size());
    
    laser->setlidaropt( LidarPropSerialBaudrate, &param.baudrate, sizeof(int) );
    int optval = YDLIDAR_TYPE_SERIAL;
    laser->setlidaropt(LidarPropDeviceType, &optval, sizeof(int));
 
    bool value = false;
    laser->setlidaropt(LidarPropAutoReconnect, 	  &value, sizeof(bool));
    laser->setlidaropt(LidarPropSingleChannel, 	  &value, sizeof(bool));

    success = laser->pingDeviceInfo( model, firmware_version, hardware_version, serialnum );

    delete laser;
    laser = NULL;
  }

  return success;
}



bool
YDLidarDriver::connect( const char *deviceName )
{
  laser = new CYdLidar();
  laser->verbose = g_Verbose;
  laser->trying  = trying;

  /// lidar port
  std::string	 port( deviceName );

  CYdLidar &laser( *this->laser );

  laser.setlidaropt( LidarPropSerialPort, port.c_str(), port.size());

  /// ignore array
  std::string ignore_array;
  ignore_array.clear();
  laser.setlidaropt(LidarPropIgnoreArray, ignore_array.c_str(), ignore_array.size());

  //////////////////////int property/////////////////

  laser.setlidaropt(LidarPropSerialBaudrate, 	 &param.baudrate, sizeof(int));
  laser.setlidaropt(LidarPropLidarType, 	 &param.lidarType, sizeof(int));
  laser.setlidaropt(LidarPropDeviceType, 	 &param.deviceType, sizeof(int));
  laser.setlidaropt(LidarPropSampleRate, 	 &param.sampleRate, sizeof(int));
  laser.setlidaropt(LidarPropAbnormalCheckCount, &param.abnormalCheckCount, sizeof(int));

  //////////////////////bool property/////////////////

  laser.setlidaropt(LidarPropFixedResolution, 	  &param.fixedResolution, sizeof(bool));
  laser.setlidaropt(LidarPropReversion, 	  &param.reversion, sizeof(bool));
  laser.setlidaropt(LidarPropInverted, 		  &param.inverted, sizeof(bool));
  laser.setlidaropt(LidarPropAutoReconnect, 	  &param.autoReconnect, sizeof(bool));
  laser.setlidaropt(LidarPropSingleChannel, 	  &param.isSingleChannel, sizeof(bool));
  laser.setlidaropt(LidarPropIntenstiy, 	  &param.intensity, sizeof(bool));

  laser.setlidaropt(LidarPropSupportMotorDtrCtrl, &param.supportMotorDtrCtrl, sizeof(bool));
  laser.setlidaropt(LidarPropSupportHeartBeat, 	  &param.supportHeartBeat, sizeof(bool));

  //////////////////////float property/////////////////

  laser.setlidaropt(LidarPropMaxAngle, 	    &param.maxAngle,  sizeof(float));
  laser.setlidaropt(LidarPropMinAngle, 	    &param.minAngle,  sizeof(float));
  laser.setlidaropt(LidarPropMaxRange, 	    &param.maxRange,  sizeof(float));
  laser.setlidaropt(LidarPropMinRange, 	    &param.minRange,  sizeof(float));
  laser.setlidaropt(LidarPropScanFrequency, &param.frequency, sizeof(float));

#if YDLIDARVERSION >= 110
  laser.enableGlassNoise(false);
  laser.enableSunNoise(false);
#endif

  bool success = laser.initialize();

  if ( !success )
    disconnect();

  return success;
}

std::string
YDLidarDriver::getSDKVersion() const
{ return laser->lidarPtr->getSDKVersion(); }

std::string
YDLidarDriver::getSerialNumber() const
{ return laser->m_SerialNumber; }

int
YDLidarDriver::getModel() const
{ return laser->lidar_model; }

int
YDLidarDriver::getFirmwareVersion() const
{ return (laser->Major<<8) | laser->Minjor; }

int
YDLidarDriver::getHardwareVersion() const
{ return laser->m_LidarVersion.hardware; }

void
YDLidarDriver::disconnect()
{
  if ( laser != NULL )
  { laser->disconnecting();
    delete laser;
    laser = NULL;
  }
}

void
YDLidarDriver::startMotor()
{
  if ( laser != NULL )
    laser->turnOn();
}

void
YDLidarDriver::stopMotor()
{
  if ( laser != NULL )
    laser->turnOff();
}

static bool
compareAngle(const ScanPoint& p1, const ScanPoint& p2)
{ return p1.angle < p2.angle; 
}

bool
YDLidarDriver::grabScanData( ScanData &data, int timeout )
{
  if ( laser == NULL )
    return false;
  
  LaserScan laserScan;

  if ( !laser->doProcessSimple(laserScan) ) 
  { Lidar::error( "YDLidarDriver::grabScanData(): Failed to get Lidar Data" );
    return false;
  }

  data.resize( laserScan.points.size() );
  
  for ( int i = ((int)laserScan.points.size())-1; i >= 0; --i )
  {
    LaserPoint &scanData( laserScan.points[i] );
    ScanPoint  &sample   ( data[i] );
   
    sample.distance = scanData.range;
    sample.angle    = scanData.angle * 180.0 / M_PI;
    if ( sample.angle < 0 )
      sample.angle += 360.0;
    if ( sample.distance == 0 )
      sample.quality  = 0;
    else
    {
      sample.quality  = scanData.intensity;
      if ( sample.quality >= 10 ) // hack, intensities seem to be constant
	sample.quality = 127;
    }
//    printf("sample(%d): theta: %03.2f Dist: %08.2f Q: %d \n", i, sample.angle, sample.distance, (int)sample.quality );
  }

  std::sort( data.begin(), data.end(), compareAngle );

  return true;
}


void
YDLidarDriver::setVerbose( int level )
{
  g_Verbose = level;
  
  if ( laser != NULL )
    laser->verbose = level;
}

