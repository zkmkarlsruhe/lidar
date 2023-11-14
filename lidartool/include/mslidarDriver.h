// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef _MSLIDAR_DRIVER_H_
#define _MSLIDAR_DRIVER_H_

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

namespace ordlidar 
{
  class OrdlidarDriver;

}



/***************************************************************************
*** 
*** MSLidarDriver
***
****************************************************************************/

class MSLidarDriver
{
public:
  ordlidar::OrdlidarDriver *lidar;

protected:

public:
  MSLidarDriver();
  ~MSLidarDriver();

  bool 		isOpen() const;

  bool 		connect( const char *deviceName );
  void 		disconnect();
  bool		setRotationSpeed( float speed );
  float		getRotationSpeed();
  bool		startMotor();
  bool		stopMotor();

  bool		grabScanData( ScanData &data, int timeout=1000 );

  static void	setVerbose( int level );
};



#endif

