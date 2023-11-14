// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef _LSLIDAR_DRIVER_H_
#define _LSLIDAR_DRIVER_H_

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
*** LSLidarDriver
***
****************************************************************************/

class LSLidarDriver
{
public:
  enum Model
  {
        Undefined = 0,
        M10 	  = 1,
	N10 	  = 2,
  };
  
protected:

  int  	waitReadable( int millis );
  int  	read(char *buffer, int length, int timeout=30);

  void 	flushinput();

  int 	close();

  bool  readPacketM10( int count, int timeout=30 );
  bool  parseDataN10 ( unsigned char *buf, unsigned int len, ScanData &scanDataReady );

  int32_t fd;
  char   *packet_bytes;
  double  last_degree;
  int     scanIndex;
  int	  totalBytes;

  ScanData scanData, scanDataReady;
  

  bool 		connectM10( );
  bool 		connectN10();

  bool		grabScanDataM10( ScanData &data, int timeout=1000 );
  bool		grabScanDataN10( ScanData &data, int timeout=1000 );

public:
  Model	  model;

  LSLidarDriver();
  ~LSLidarDriver();

  bool 		isOpen() const;

  int 		setOpt( int nBits, uint8_t nEvent, int nStop );
  
  bool 		connect( const char *deviceName, bool tryOpen=false );
  void 		disconnect();

  bool		grabScanData( ScanData &data, int timeout=1000 );

  static void	setVerbose( int level );
};



#endif

