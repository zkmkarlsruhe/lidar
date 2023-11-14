// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef _LDLIDAR_DRIVER_H_
#define _LDLIDAR_DRIVER_H_

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

namespace ldlidar 
{
  class LDLidarDriver;
};

#include "scanData.h"

/***************************************************************************
*** 
*** DECLARATIONS
***
****************************************************************************/

/***************************************************************************
*** 
*** LDLidarDriver
***
****************************************************************************/

class LDLidarDriver
{
public:
  ldlidar::LDLidarDriver *node;

public:
  bool  usePWM;
  int   offPWM;
  int   onPWM;
  int   period;
  int	pwmChip;
  int	pwmChannel;

  std::string	deviceName;
  std::string	model;

public:
  LDLidarDriver( bool usePWM=false, int pwmChip=0, int pwmChannel=0, int offPWM=240, int onPWM=8400, int period=24000 );
  ~LDLidarDriver();

  std::string   sdkVersion();

  bool 		isOpen() const;

  bool 		connect( const char *deviceName, const char *model=NULL );
  void 		disconnect();
  void		setMotorSpeed( float speed );
  bool		isMotorSpeed( float speed );

  double        getSpeed();

  bool		grabScanData( ScanData &data, int timeout=1000 );

  static void	setVerbose( int level );
};



#endif

