// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef _LIDAR_VIRT_H_
#define _LIDAR_VIRT_H_

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#include "UDP.hh"

#include "rplidar.h"
#include "sl_lidar.h"

#include <queue>

/***************************************************************************
*** 
*** DECLARATIONS
***
****************************************************************************/

/***************************************************************************
*** 
*** LidarUrl
***
****************************************************************************/

inline  const char *LidarProto = "virtual:";

struct LidarUrl {

  std::string 	hostname;
  int         	port;
  int 		err;

  LidarUrl()
    : hostname(),
      port( 0 ),
      err ( 0 )
  {}

  LidarUrl(const char *url)
    : hostname(),
      port( 0 ),
      err ( 0 )
  { init(url); }

  bool isOk()  const { return err == 0 && port != 0; }
  bool isErr() const { return err != 0; }
  bool init(const char *url) {
    err = 0;
    const char *s = url;
    const char *prot = strstr(s, LidarProto);

    if (prot != 0)
      s += strlen(LidarProto);
    
    const char *col = strstr(s, ":");
    if (col != 0)
    { 
      const char *slash = strstr(s, "//");
      if ( slash != NULL )
	s += 2;
      hostname.assign(s, col);
      s = col+1;
    }
    
    port = atoi(s);
    if (port == 0)
    { err = 3; return false; }
    
    return true;
  }

  static void	printHelp( const char *prefix=" ", const char *postfix="\n", bool output=false )
  {
    if ( output )
      printf( "%s[hostname|IP:]port%s", prefix, postfix );
    else
      printf( "%svirtual[:hostname|IP]:port%s", prefix, postfix );
  }
  
};

/***************************************************************************
*** 
*** LidarRawSample
***
****************************************************************************/

class LidarRawSample
{
public:
  uint16_t	angle_z_q14;
  uint32_t   	dist_mm_q2;
  int8_t    	quality;

};

/***************************************************************************
*** 
*** LidarRawSampleBuffer
***
****************************************************************************/

class LidarRawSampleBuffer : public std::vector<LidarRawSample> 
{
public:
};

/***************************************************************************
*** 
*** LidarScanData
***
****************************************************************************/

class LidarScanData
{
public:
  _u64  seqNr;
  LidarRawSampleBuffer  nodes;
  std::vector<bool>	packetsReceived;

  LidarScanData( _u64 seqNr=0 )
  : seqNr( seqNr ),
    nodes(),
    packetsReceived()
  {}

  bool	complete() const;

  void	addData( unsigned char *data, int size );
};


/***************************************************************************
*** 
*** LidarScanDataList
***
****************************************************************************/

class LidarScanDataList : public std::vector<LidarScanData> 
{
public:
  _u64  currentSeqNr;
  int   seqFailureCount;
  LidarScanDataList()
  : currentSeqNr( 0 )
  {}

  bool grabScanData( LidarRawSampleBuffer &nodes, bool grabLatest );

  LidarScanData &getScanData( _u64 seqNr );
  
};


/***************************************************************************
*** 
*** LidarVirtualUdpSocket
***
****************************************************************************/

class LidarVirtualUdpSocket : public UdpSocket
{
public:
    SockAddr	lastRemoteAddr;
    bool		send( const char *msg, int length )
    { if ( remote_addr.empty() )
	return false;
      return sendPacket( msg, length );
    }
    
    
};

/***************************************************************************
*** 
*** LidarVirtualDriver
***
****************************************************************************/

class LidarVirtualDriver
{
public:
  UdpSocket			udpSocket;
  int				port;
  std::string			remoteHostname;
  int				remotePort;
  
  std::queue<std::string> 	cmdQueue;
  
  LidarScanDataList	 scanDataList;
  LidarScanDataList	 envDataList;

  int			 motorState;
  int			 powerUpState;
  bool 			 isInDevice;
  SockAddr 		 lastRemoteAddr; 
  bool			 deviceStatusSent;
  bool			 isOpen;
  
  _u64			 seqNr;
  uint64_t 		 lastRecvTime;
  uint64_t 		 connectTime;
  uint64_t 		 statusTime;

  int  angIndex( int angIndex, int numSamples ) const
  { return (angIndex+numSamples) % numSamples; }

  int angIndexByAngle( float angle, int numSamples ) const
  { return angIndex( round( angle / (2*M_PI) * (numSamples-1) ), numSamples ); }

  bool		sendScanData( LidarRawSampleBuffer &nodes, bool isEnv );
      
public:
  LidarVirtualDriver( bool isInDevice );
  
  bool 		connect( std::string hostname, int port );
  bool 		send   ( const char *msg, int length );
  bool 		sendConnect();
  bool 		sendConnectAcknowledge();
  bool 		sendCmd( const char *cmd );
  bool		sendScanData ( LidarRawSampleBuffer &nodes );
  bool		sendEnvData  ( LidarRawSampleBuffer &nodes );
  bool		sendUseOutEnv( bool useOutEnv );
  bool 		sendMotorState();
  bool 		sendPowerUpState();
  bool 		sendStatus();
  bool 		sendDeviceType( const char *deviceType, const char *sensorIN, bool sensorPowerSupported );

  bool          setUSBPower( bool on );
  bool 		setMotorState( bool state );

  void		update( int timeout_ms=0 );
  bool		grabScanData( LidarRawSampleBuffer &nodes, int timeout=1000, bool grabLatest=true );
  bool		grabEnvData ( LidarRawSampleBuffer &nodes );

  std::string   getNextCmd();
  std::string   getRemoteHostname() const;
  std::string   getHostname() const;
  int           getRemotePort() const;
  
  static void	setVerbose( int level );
};

#endif

