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

#include "helper.h"
#include "mslidarDriver.h"

#include "core/serial/serial.h"
#include "ord_lidar_driver.h"
using namespace ordlidar;


#include <errno.h>
#include <memory.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <fstream>

#include <termios.h>

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

/***************************************************************************
*** 
*** MSLidarDriver
***
****************************************************************************/


MSLidarDriver::MSLidarDriver()
  : lidar( NULL )
{
}

MSLidarDriver::~MSLidarDriver()
{
  if ( lidar != NULL )
  { disconnect();
    delete lidar;
  }
}

bool
MSLidarDriver::isOpen() const
{ 
  return lidar != NULL;
}  

bool
MSLidarDriver::connect( const char *deviceName )
{
  if ( isOpen() )
    return true;
  
  OrdlidarDriver *lidar = new OrdlidarDriver( ORADAR_TYPE_SERIAL, ORADAR_MS200 );
  
  lidar->SetSerialPort( deviceName, 230400 );

  if ( !lidar->Connect() )
  { delete lidar;
    return false;
  }
  
  if ( !lidar->Ping() )
  { delete lidar;
    return false;
  }

  this->lidar = lidar;

  return true;
}


float
MSLidarDriver::getRotationSpeed()
{
  if ( !isOpen() )
    return 0;
  
  return lidar->GetRotationSpeed();
}

bool
MSLidarDriver::setRotationSpeed( float speed )
{
  if ( !isOpen() )
    return false;
  
  return lidar->SetRotationSpeed( speed );
}

bool
MSLidarDriver::startMotor()
{
  if ( lidar == NULL )
    return false;

  return lidar->Activate();
}

bool
MSLidarDriver::stopMotor()
{
  if ( lidar == NULL )
    return false;

  return lidar->Deactive();
}

void
MSLidarDriver::disconnect()
{ 
  if ( lidar == NULL )
    return;

  lidar->Disconnect();
  
  delete lidar;
  lidar = NULL;
}

bool
MSLidarDriver::grabScanData( ScanData &data, int timeout )
{
  if ( !isOpen() )
    return false;

  full_scan_data_st scan_data;
  
  if ( !lidar->GrabFullScanBlocking( scan_data, timeout ) )
    return false;

  data.resize( scan_data.vailtidy_point_num );

  for ( int i = ((int)scan_data.vailtidy_point_num)-1; i >= 0; --i )
  {
    point_data_t &scanData( scan_data.data[i] );
    ScanPoint    &sample( data[i] );
   
    sample.distance  = scanData.distance / 1000.0;
    sample.angle     = scanData.angle;
    sample.quality   = scanData.intensity / 2;

//    if ( sample.quality >= 127 )
//      printf( "%g %d\n", sample.angle, sample.quality );
    
  }

  return true;
}


void
MSLidarDriver::setVerbose( int level )
{
  g_Verbose = level;
}

