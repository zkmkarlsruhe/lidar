// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef _YDLIDAR_DRIVER_H_
#define _YDLIDAR_DRIVER_H_

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#include "scanData.h"

/***************************************************************************
*** 
*** DECLARATIONS
***
****************************************************************************/

/***************************************************************************
*** 
*** YDLidarParam
***
****************************************************************************/

struct YDLidarDeviceSpec {
  std::string 		model;
  int			modelId;
  int 			baudrate;
  std::vector<int>	sampleRates;
  int 			defaultSampleRate;
  float 		minRange;
  float 		maxRange;
  float	 		minFrequency;
  float	 		maxFrequency;
  float	 		defaultFrequency;
  bool 			intensity;
  bool 			singleChannel;
  int 			lidarType;
  int 			deviceType;
  bool			supported;
  bool			tested;
  
};

 
class YDLidarParam
{
public:

  int		 lidarType;
  int		 deviceType;
  int 		 baudrate;
  int		 sampleRate;
  int		 abnormalCheckCount;

  bool		 fixedResolution;
  bool		 reversion;
  bool		 inverted;
  bool		 autoReconnect;
  bool		 isSingleChannel;
  bool		 intensity;
  bool		 supportMotorDtrCtrl;
  bool		 supportHeartBeat;

  float		 maxAngle;
  float		 minAngle;
  float		 maxRange;
  float		 minRange;
  float 	 frequency;

  YDLidarParam( const char *modelName=NULL );
  
  void setSpec( YDLidarDeviceSpec *spec );
  
  bool isSerial() const;
  
};

/***************************************************************************
*** 
*** YDlidarDriver
***
****************************************************************************/

class CYdLidar;

class YDLidarDriver
{
public:

  
  YDLidarParam	param;
  CYdLidar     *laser;
  bool		trying;
  
public:
  YDLidarDriver();
  ~YDLidarDriver();

  bool 		isOpen() const;

  std::string	getSDKVersion() const;
  std::string	getSerialNumber () const;
  int		getModel () const;
  int		getFirmwareVersion() const;
  int		getHardwareVersion() const;

  bool 		pingDeviceInfo( const char *deviceName, int &model, int &firmware_version, int &hardware_version, uint8_t serialnum[16] );
  bool 		connect( const char *deviceName );
  void 		disconnect();

  void		startMotor();
  void		stopMotor();

  bool		grabScanData( ScanData &data, int timeout=1000 );

  void		setVerbose( int level );

  static struct YDLidarDeviceSpec *getSpec( const char *modelName );
  static struct YDLidarDeviceSpec *getSpec( int modelId );
  
};



#endif

