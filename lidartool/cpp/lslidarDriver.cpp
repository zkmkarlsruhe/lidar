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
#include "lslidarDriver.h"

#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>
#include <chrono>
#include <math.h>

#include <iostream>

#include <termios.h>
#include <sys/ioctl.h>

class Lidar
{
public:
  static bool	initialize();

  static void (*error)( const char *format, ... );
  static void (*log)  ( const char *format, ... );
};

/***************************************************************************
*** 
*** DEFINES
***
****************************************************************************/

#define MAX_ACK_BUF_LEN 2304000
#define	POINT_PER_PACK  16

/***************************************************************************
*** 
*** GLOBALS
***
****************************************************************************/

static int g_Verbose = false;

/***************************************************************************
*** 
*** Helper
***
****************************************************************************/

/***************************************************************************
*** 
*** LSLidarDriver
***
****************************************************************************/


LSLidarDriver::LSLidarDriver()
  : fd(-1),
    totalBytes( 0 ),
    scanData(),
    model( Undefined )
{
  packet_bytes = new char[MAX_ACK_BUF_LEN];
}

LSLidarDriver::~LSLidarDriver()
{
  disconnect();

  delete[] packet_bytes;
}

bool
LSLidarDriver::isOpen() const
{ return fd != -1;
}  

void
LSLidarDriver::disconnect()
{ 
  if ( !isOpen() )
    return;

  ::close(fd);
  fd = -1;
}

void
LSLidarDriver::flushinput()
{
  if ( !isOpen() )
    return;

  tcflush(fd, TCIFLUSH);
}


int
LSLidarDriver::waitReadable(int millis)
{
  int serial = fd;
  
  fd_set fdset;
  struct timeval tv;
  int rc = 0;
  
  while (millis > 0)
  {
    if (millis < 5000)
    {
      tv.tv_usec = millis % 1000 * 1000;
      tv.tv_sec  = millis / 1000;

      millis = 0;
    }
    else
    {
      tv.tv_usec = 0;
      tv.tv_sec  = 5;

      millis -= 5000;
    }

    FD_ZERO(&fdset);
    FD_SET(serial, &fdset);
    
    rc = select(serial + 1, &fdset, NULL, NULL, &tv);
    if (rc > 0)
    {
      rc = (FD_ISSET(serial, &fdset)) ? 1 : -1;
      break;
    }
    else if (rc < 0)
    {
      rc = -1;
      break;
    }
  }

  return rc;
}


int
LSLidarDriver::read(char *buffer, int length, int timeout)
{
  memset(buffer, 0, length);

  int totalBytesRead = 0;
  int rc;
  int unlink = 0;
  char* pb = buffer;

  if (timeout > 0)
  {
    rc = waitReadable(timeout);
    if (rc <= 0)
    {
      return (rc == 0) ? 0 : -1;
    }

    int	retry = 3;
    while (length > 0)
    {
      rc = ::read(fd, pb, (size_t)length);

      if (rc > 0)
      {		
        length -= rc;
        pb += rc;
        totalBytesRead += rc;

        if (length == 0)
        {
          break;
        }
      }
      else if (rc < 0)
      {
	Lidar::error("LSLidarDriver::read(): error");
        retry--;
        if (retry <= 0)
        {
          break;
        }
      }
	 
      unlink++;
      rc = waitReadable(20);
      if(unlink > 10)
	return -1;
	  
      if (rc <= 0)
      {
        break;
      }
    }
  }
  else
  {
    rc = ::read(fd, pb, (size_t)length);

    if (rc > 0)
    {
      totalBytesRead += rc;
    }
    else if ((rc < 0) && (errno != EINTR) && (errno != EAGAIN))
    {
      Lidar::error("LSLidarDriver::read() error");
      return -1;
    }
  }

  return totalBytesRead;
}

bool
LSLidarDriver::readPacketM10( int count, int timeout )
{
  bool success = true;
  
  for (int i = 0; i < count; i++)
  {
    int k = packet_bytes[i];
    k < 0 ? k += 256 : k;
    int y = packet_bytes[i + 1];
    y < 0 ? y += 256 : y;
    
    int k_1 = packet_bytes[i + 2];
    k_1 < 0 ? k_1 += 256 : k_1;
    int y_1 = packet_bytes[i + 3];
    y_1 < 0 ? y_1 += 256 : y_1;
		
    if (k == 0xA5 && y == 0x5A)
    {
      if(i != 0)
      {
	memcpy(packet_bytes, packet_bytes + 92 - i, 92 - i);
	int numRead = read(packet_bytes + 92 - i, i, timeout);
	if ( numRead != i )
	  return false;
      }
			
      int s = packet_bytes[i + 2];
      s < 0 ? s += 256 : s;
      int z = packet_bytes[i + 3];
      z < 0 ? z += 256 : z;
			
      float degree = (s * 256 + z) / 100.f;

      if ( degree >= 360 )
	degree -= 360;
      
      s = packet_bytes[i + 4];
      s < 0 ? s += 256 : s;
      z = packet_bytes[i + 5];
      z < 0 ? z += 256 : z;
      
//      Difop_data->MotorSpeed = float(2500000.0 / (s * 256 + z));
			
      int invalidValue = 0;
      for (size_t num = 2; num < 86; num+=2)
      {
	int s = packet_bytes[i + num + 4];
	s < 0 ? s += 256 : s;
	int z = packet_bytes[i + num + 5];
	z < 0 ? z += 256 : z;
	
	if ((s * 256 + z) != 0xFFFF)
        {
	  scanData[scanIndex].distance = double(s * 256 + (z)) / 1000.f;
	  
	  if ( scanData[scanIndex].distance > 0.1 )
	    scanData[scanIndex].quality  = 100;
	  scanIndex++;
	}
	else
	  invalidValue++;
      }

      invalidValue = 42 - invalidValue;

      for (size_t i = 0; i < invalidValue; i++)
      {
	if ((degree + (15.0 / invalidValue * i)) > 360)
	  scanData[scanIndex-invalidValue+i].angle = degree + (15.0 / invalidValue * i) - 360;
	else
	  scanData[scanIndex-invalidValue+i].angle = degree + (15.0 / invalidValue * i);
      }
			
      if (degree < last_degree)
      {
	scanIndex = 0;
			
	scanDataReady.resize(scanData.size());
	scanDataReady.assign(scanData.begin(), scanData.end());

	for(int k=0; k<scanData.size(); k++)
        { scanData[k].distance  = 0;
	  scanData[k].angle     = 0;
	  scanData[k].quality   = 0;
	}

	success = true;
      }
      last_degree = degree;
    }	
    else if (k == 0xA5 && y == 0xFF && k_1 == 00 && y_1 == 0x5A)
    {
	  /*lsm10_v2::difopPtr Difop_data = lsm10_v2::difopPtr(
						new lsm10_v2::difop());
 
			//温度
			int s = packet_bytes[i + 12];
			s < 0 ? s += 256 : s;
			int z = packet_bytes[i + 13];
			z < 0 ? z += 256 : z;
			Difop_data->Temperature = (float(s * 256 + z) / 4096 * 330 - 50);

			//高压
			s = packet_bytes[i + 14];
			s < 0 ? s += 256 : s;
			z = packet_bytes[i + 15];
			z < 0 ? z += 256 : z;
			Difop_data->HighPressure = (float(s * 256 + z) / 4096 * 3.3 * 101);

			//转速
			s = packet_bytes[i + 16];
			s < 0 ? s += 256 : s;
			z = packet_bytes[i + 17];
			z < 0 ? z += 256 : z;
			Difop_data->MotorSpeed = (float(1000000 / (s * 256 + z) / 24));
			
			device_pub.publish(Difop_data);*/
    }
  }

  return success;
}


bool
LSLidarDriver::grabScanDataM10( ScanData &data, int timeout )
{
  if ( waitReadable(timeout) <= 0 )
    return false;
  
  timeout = 200;
  
  int count = read( packet_bytes, 92, 200 );
  if ( count <= 0 ) 
    return false;


  if ( !readPacketM10( count, 200 ) || scanDataReady.size() == 0 )
   return false;
  
  data.assign(scanDataReady.begin(), scanDataReady.end());
  scanDataReady.resize( 0 );

  return true;
}


bool
LSLidarDriver::parseDataN10( unsigned char *buf, unsigned int len, ScanData &scanDataReady)
{
  bool success = false;
  
  unsigned int i;
  double lidar_angle;
  double degree;

//  printf( "%0x %0x\n", buf[0], buf[1] );

  if(buf[0] != 0XA5 || buf[1] != 0X5A) 
    return false;
	
  uint8_t crc = 0;

  for (uint32_t i = 0; i < 57; i++)
  {
    crc = (crc + buf[i]) & 0xff;
  }

  if (crc != buf[57]) return false;
  double start_angle = buf[5] *256+buf[6];
  double end_angle   = buf[55]*256+buf[56];
	
  double diff = (double) fmod(double (end_angle - start_angle + 36000),36000)/100;

      //printf("diff = %d\n",diff);	
  float step = diff / (POINT_PER_PACK - 1);
  float start = (double)start_angle / 100.0;
  float end = (double)fmod(end_angle , 36000)/100;;	

  ScanPoint data;	
  for (int i = 0; i < 16; i++)
  {
    data.distance = buf[7+i*3]*256+buf[8+i*3];
    data.distance /= 1000.0;
    data.angle = start + i * step;
    
    if (data.angle >= 360.0)
    {
      data.angle -= 360.0;
    }
    data.quality = buf[9+i*3];
	
//    printf( "data: %g %g %d\n", data.angle, data.distance, data.quality );

    if (scanIndex > 0 )
    {
      if( data.angle < last_degree )
      {
	scanDataReady.resize(scanData.size());
	scanDataReady.assign(scanData.begin(), scanData.end());

	scanIndex = 0;
	scanData.resize( scanIndex );

	success = true;
      }
    }

    scanData.resize( scanIndex+1 );
    scanData[scanIndex] = data;
    
    last_degree = data.angle ;

    scanIndex += 1;
  }

  return success;
}


bool
LSLidarDriver::grabScanDataN10( ScanData &data, int timeout )
{
  if ( waitReadable(timeout) <= 0 )
    return false;
  
  uint64_t startMSec = getmsec();
  int count;
  
  while ( getmsec()-startMSec <= timeout && waitReadable(1)>0 && (count=::read( fd, &packet_bytes[totalBytes], MAX_ACK_BUF_LEN-totalBytes )) > 0 )
  {
    totalBytes = totalBytes + count;
				
    if ( totalBytes >= 47 )
    { bool success = parseDataN10( (unsigned char *)packet_bytes, totalBytes, data );
      totalBytes = 0;
      if ( success )
	return true;
    }
  }
  
  return false;
}


bool
LSLidarDriver::grabScanData( ScanData &data, int timeout )
{
  if ( waitReadable(timeout) <= 0 )
    return false;
  
  if ( model == N10 )
    return grabScanDataN10( data, timeout );
  
  return grabScanDataM10( data, timeout );
}


bool
LSLidarDriver::connectM10()
{
  struct termios newtio, oldtio;

  if (tcgetattr(fd, &oldtio) != 0)
  { perror("LSLidarDriver::connect(): serial get error");
    disconnect();
    return false;
  }

  bzero(&newtio, sizeof(newtio));

  newtio.c_cflag |= (CLOCAL | CREAD | CS8);

  newtio.c_cflag &= ~PARENB;

  int baudrate = B460800;
//  int baudrate = B230400;

  cfsetispeed(&newtio, baudrate);
  cfsetospeed(&newtio, baudrate);

  newtio.c_cflag &= ~CSTOPB;

  newtio.c_cc[VTIME] = 0;
  newtio.c_cc[VMIN]  = 0;

  tcflush(fd, TCIFLUSH);

  if ((tcsetattr(fd, TCSANOW, &newtio)) != 0)
  { perror("LSLidarDriver::connect(): serial set error");
    disconnect();
    return false;
  }

  scanData.resize( 360 * 42 / 15 );

  return true;
}


bool
LSLidarDriver::connectN10()
{
  struct termios newtio, oldtio;

  if (tcgetattr(fd, &oldtio) != 0)
  { perror("LSLidarDriver::connect(): serial get error");
    disconnect();
    return false;
  }

  bzero(&newtio, sizeof(newtio));

//  int baudrate = B460800;
  int baudrate = B230400;

  cfsetispeed(&newtio, baudrate);		 
  cfsetospeed(&newtio, baudrate);

  newtio.c_cflag |= (tcflag_t)(CLOCAL | CREAD | CS8 | CRTSCTS);
  newtio.c_cflag &= (tcflag_t) ~(CSTOPB | PARENB | PARODD);
  newtio.c_lflag &= (tcflag_t) ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL |
				  ISIG | IEXTEN);  
  newtio.c_oflag &= (tcflag_t) ~(OPOST);
  newtio.c_iflag &= (tcflag_t) ~(IXON | IXOFF | INLCR | IGNCR | ICRNL | IGNBRK);

  newtio.c_cc[VTIME] = 0;	
  newtio.c_cc[VMIN]  = 0;	
	
  tcflush(fd, TCIFLUSH);	

  if (tcsetattr(fd, TCSANOW, &newtio) != 0)
  { perror("LSLidarDriver::connect(): serial set error");
    disconnect();
    return false;
  }

  return true;
}

bool
LSLidarDriver::connect( const char *deviceName, bool tryOpen )
{
  if ( model == Undefined || tryOpen )
  {
    ScanData laserScan;
    uint64_t startMSec;

    int timeout = 300;

    Model modelOrg = model;

    if ( modelOrg == Undefined )
      model = M10;

    if ( model == M10 )
    {
      startMSec = getmsec();
      if ( connect( deviceName ) )
      { 
	while ( getmsec()-startMSec < timeout )
          if ( grabScanData( laserScan, timeout/10 ) )
	    return true;
      }
      disconnect();
    }
    
    if ( modelOrg == Undefined )
      model = N10;

    if ( model == N10 )
    {
      startMSec = getmsec();
      if ( connect( deviceName ) )
      { 
	while ( getmsec()-startMSec < timeout )
	  if ( grabScanData( laserScan, timeout/10 ) )
	    return true;
      }
      disconnect();
    }
    
    model = modelOrg;
      
    return false;
  }

  last_degree = 0.0;
  scanIndex   = 0;

  int flags = (O_RDWR | O_NOCTTY | O_NDELAY);

  fd = open(deviceName, flags);
  if (-1 == fd) {
    return false;
  }

  if ( model == N10 )
    return connectN10();

  return connectM10();
}


void
LSLidarDriver::setVerbose( int level )
{
  g_Verbose = level;
}

