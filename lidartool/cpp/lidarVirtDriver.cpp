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
#include "keyValueMap.h"
#include "lidarKit.h"
#include "lidarVirtDriver.h"

using namespace rp::standalone::rplidar;

/***************************************************************************
*** 
*** DEFINES
***
****************************************************************************/

#define MSG_BASE      	   0x1254125412540000
#define MSG_SCAN_DATA      (MSG_BASE|1)
#define MSG_ENV_DATA       (MSG_BASE|2)
#define MSG_CMD		   (MSG_BASE|3)

/***************************************************************************
*** 
*** GLOBALS
***
****************************************************************************/

static const int nodesPerPacket = 128;

static int g_Verbose = false;

/***************************************************************************
*** 
*** Helper
***
****************************************************************************/

/***************************************************************************
*** 
*** LidarVirtualHeader
***
****************************************************************************/

class LidarVirtualHeader
{
public:
  _u64  type;

  LidarVirtualHeader( _u64 type )
    : type ( type )
    {}
};
  

/***************************************************************************
*** 
*** LidarCmdHeader
***
****************************************************************************/

class LidarCmdHeader : public LidarVirtualHeader
{
public:
  _u16 cmdSize;
  
  LidarCmdHeader()
    : LidarVirtualHeader( MSG_CMD ),
      cmdSize( 0 )
    {}
  
  LidarCmdHeader( const char *cmd )
    : LidarVirtualHeader( MSG_CMD ),
      cmdSize( strlen( cmd )+1 )
    {}
  
};
  
/***************************************************************************
*** 
*** LidarScanHeader
***
****************************************************************************/

class LidarScanHeader : public LidarVirtualHeader
{
public:
  _u64 seqNr;
  _u8  packetId;
  _u8  nodesPerPacket;
  _u16 totalNodes;

  LidarScanHeader( _u64 type=MSG_SCAN_DATA )
    : LidarVirtualHeader( type )
    {}
  
  LidarScanHeader( _u64 type, _u64 seqNr, int packetId, int nodesPerPacket, int totalNodes )
    : LidarVirtualHeader( type ),
      seqNr	    ( seqNr ),
      packetId      ( packetId ),
      nodesPerPacket( nodesPerPacket ),
      totalNodes    ( totalNodes )
    {}
  
};
  
/***************************************************************************
*** 
*** LidarCmdMsg
***
****************************************************************************/

class LidarCmdMsg
{
public:
  std::vector<unsigned char> buffer;

  LidarCmdMsg( const char *cmd )
  {
    LidarCmdHeader header( cmd );
  
    buffer.resize( sizeof(header) + header.cmdSize );
    
    memcpy( (void*)&buffer[0], 		    (void*)&header, sizeof(header) );
    memcpy( (void*)&buffer[sizeof(header)], (void*)cmd, header.cmdSize );
  }
  
};
  
/***************************************************************************
*** 
*** LidarScanMsg
***
****************************************************************************/

class LidarScanMsg
{
public:
  std::vector<unsigned char> buffer;

  LidarScanMsg( _u64 type, _u64 seqNr, int packetId, int nodesPerPacket, int totalNodes, LidarRawSampleBuffer &nodes )
  {
    LidarScanHeader header( type, seqNr, packetId, nodesPerPacket, totalNodes );

//    printf( "send %d %d %d\n", header.packetId, header.nodesPerPacket, header.totalNodes );

    int nodesInPacket = nodesPerPacket;

    if ( (packetId+1)*nodesPerPacket > totalNodes )
      nodesInPacket = totalNodes % nodesPerPacket;

    buffer.resize( sizeof(header) + nodesInPacket * sizeof(nodes[0]) );
    
    memcpy( (void*)&buffer[0], (void*)&header, sizeof(header) );

    if ( nodesInPacket > 0 )
      memcpy( (void*)&buffer[sizeof(header)], (void*)&nodes[packetId*nodesPerPacket], nodesInPacket * sizeof(nodes[0]) );
  }
  
};

/***************************************************************************
*** 
*** LidarScanData
***
****************************************************************************/

bool
LidarScanData::complete() const
{
  if ( packetsReceived.size() == 0 )
    return false;
    
  for ( int i = packetsReceived.size()-1; i >= 0; --i )
    if ( !packetsReceived[i] )
      return false;
    
  return true;
}

void
LidarScanData::addData( unsigned char *data, int size )
{
  LidarScanHeader &header( *(LidarScanHeader*)data );

  int numPackets = header.totalNodes / header.nodesPerPacket;
  if ( numPackets == 0 || header.totalNodes % header.nodesPerPacket > 0 )
    numPackets += 1;

  if ( header.packetId >= numPackets )
    return;

  if ( packetsReceived.size() == 0 )
  { packetsReceived 	= std::vector<bool>( numPackets, false );
    seqNr		= header.seqNr;
    nodes.resize( header.totalNodes );
  }
    
  if ( header.packetId >= packetsReceived.size() )
    Lidar::error( "addData: header.packetId: %d packetsReceived.size(): %ld", header.packetId, packetsReceived.size() );

  packetsReceived[header.packetId] = true;
       
  int nodesInPacket = header.nodesPerPacket;
    
  if ( (header.packetId+1)*header.nodesPerPacket > header.totalNodes )
    nodesInPacket = header.totalNodes % header.nodesPerPacket;

  if ( nodesInPacket > 0 )
  {
    if ( size != sizeof(header) + nodesInPacket * sizeof(LidarRawSample) )
      Lidar::error( "addData: received size: %d != calculated %ld", size, sizeof(header) + nodesInPacket * sizeof(LidarRawSample) );

    int nodeIndex = header.packetId*header.nodesPerPacket;
    
    if ( nodeIndex + nodesInPacket > nodes.size() )
      Lidar::error( "addData: received size: %d > nodes size %ld", nodeIndex + nodesInPacket, nodes.size() );
      
    memcpy( (void*)&nodes[header.packetId*header.nodesPerPacket], (void*)&data[sizeof(header)], nodesInPacket * sizeof(LidarRawSample) );
  }
  
//    printf( "got(%d) %d/%d %d %d\n", header.seqNr, header.packetId, numPackets, header.nodesPerPacket, header.totalNodes );
}

/***************************************************************************
*** 
*** LidarScanDataList
***
****************************************************************************/

LidarScanData &
LidarScanDataList::getScanData( _u64 seqNr )
{
  for ( int i = ((int)size())-1; i >= 0; --i )
    if ( (*this)[i].seqNr == seqNr )
      return (*this)[i];

  if ( size() > 16 )
    erase( begin() );

  for ( int i = ((int)size())-1; i >= 0; --i )
  {
    if ( (*this)[i].seqNr < seqNr )
    {
      emplace( begin()+i+1, seqNr );
      
      return (*this)[i+1];
    }
  }

  emplace( begin(), seqNr );

  return (*this)[0];
}


bool
LidarScanDataList::grabScanData( LidarRawSampleBuffer &nodes, bool grabLatest )
{
  bool success = false;
  
  for ( int i = 0; i < size(); ++i )
  {
    LidarScanData &scanData( (*this)[i] );
    
    if ( scanData.complete() )
    {
      //      printf( "complete: %d  %ld %ld\n", scanData.seqNr < currentSeqNr, scanData.seqNr, currentSeqNr );

      if ( scanData.seqNr < currentSeqNr && (seqFailureCount += 1) < 15 )
      { erase( begin()+i );
	i -= 1;
      }
      else
      {
	seqFailureCount = 0;

	nodes        = scanData.nodes;
	currentSeqNr = scanData.seqNr;
	success = true;
      
	erase( begin()+i );

	if ( !grabLatest )
	  return true;
	
	i -= 1;
      }
    }
  }

  return success;
}

/***************************************************************************
*** 
*** LidarVirtualDriver
***
****************************************************************************/

LidarVirtualDriver::LidarVirtualDriver( bool isInDevice )
  : udpSocket(),
    port( 0 ),
    remotePort( 0 ),
    cmdQueue(),
    scanDataList(),
    motorState( -1 ),
    powerUpState( -1 ),
    isInDevice( isInDevice ),
    isOpen    ( false ),
    deviceStatusSent( false ),
    seqNr( 0 )
{
}
  
void
LidarVirtualDriver::setVerbose( int level )
{
  g_Verbose = level;
}

bool
LidarVirtualDriver::connect( std::string hostname, int port )
{
  this->port = port;
  return udpSocket.connectTo( hostname, port );
}

std::string
LidarVirtualDriver::getHostname() const
{
  return udpSocket.local_addr.getHostname();
}

std::string
LidarVirtualDriver::getRemoteHostname() const
{
  return udpSocket.remote_addr.getHostname();
}

int
LidarVirtualDriver::getRemotePort() const
{
  return udpSocket.remote_addr.getPort();
}

bool
LidarVirtualDriver::send( const char *msg, int length )
{
  if ( udpSocket.remote_addr.empty() )
    return false;
  
  return udpSocket.sendPacket( msg, length );
}

bool
LidarVirtualDriver::sendConnect()
{	
  lastRemoteAddr = udpSocket.remote_addr;
  deviceStatusSent = false;
  
  return sendCmd( "connect" );
}

bool
LidarVirtualDriver::sendConnectAcknowledge()
{	
  lastRemoteAddr = udpSocket.remote_addr;

  deviceStatusSent = false;

  return false;
  
  return sendCmd( "connectAcknowledge" );
}

bool
LidarVirtualDriver::sendCmd( const char *cmd )
{	
  if ( g_Verbose > 0 )
    Lidar::info( "try send cmd: '%s'", cmd );

  if ( udpSocket.remote_addr.empty() )
    return false;
  
  LidarCmdMsg msg( cmd );

  if ( g_Verbose > 0 )
    Lidar::info( "send cmd: '%s'", cmd );

  return send( (const char *)&msg.buffer[0], msg.buffer.size() );
}

bool
LidarVirtualDriver::sendMotorState()
{
  return false;

  if ( motorState < 0 || !isInDevice )
    return false;
  
  return sendCmd( motorState ? "motorOn" : "motorOff" );
}

bool
LidarVirtualDriver::sendPowerUpState()
{
  if ( powerUpState < 0 || isInDevice )
    return false;
  
  bool result = sendCmd( powerUpState ? "startPowerUp" : "finishPowerUp" );

  powerUpState = -1;
  
  return result;
}

bool
LidarVirtualDriver::sendStatus()
{
  return sendMotorState();
}

bool
LidarVirtualDriver::sendDeviceType( const char *deviceType, const char *sensorIN, bool sensorPowerSupported )
{
  if ( isInDevice )
    return false;
  
  std::string cmd( "deviceType=" );
  cmd += deviceType;
  cmd += " sensorIN=";
  cmd += sensorIN;
  cmd += " sensorPowerSupported=";
  cmd += (sensorPowerSupported?"true":"false");
  deviceStatusSent = true;
  
  return sendCmd( cmd.c_str() );
}

bool
LidarVirtualDriver::sendScanData( LidarRawSampleBuffer &nodes, bool isEnv )
{
  if ( udpSocket.remote_addr.empty() )
    return false;
  
  seqNr += 1;
  
  int count = nodes.size();

  int numPackets = count / nodesPerPacket;
  if ( count == 0 || (count % nodesPerPacket) != 0 )
    numPackets += 1;
  
  bool success = true;
  
  for ( int i = 0; i < numPackets; ++i )
  {
    LidarScanMsg msg( isEnv ? MSG_ENV_DATA : MSG_SCAN_DATA, seqNr, i, nodesPerPacket, count, nodes );
    success = (send( (const char *)&msg.buffer[0], msg.buffer.size() ) && success);
  }

  return success;
}

bool
LidarVirtualDriver::sendUseOutEnv( bool useOutEnv )
{
  return sendCmd( useOutEnv ? "outEnvOn" : "outEnvOff" );
}

bool
LidarVirtualDriver::sendEnvData( LidarRawSampleBuffer &nodes )
{
  return sendScanData( nodes, true );
}

bool
LidarVirtualDriver::sendScanData( LidarRawSampleBuffer &nodes )
{
  return sendScanData( nodes, false );
}

void
LidarVirtualDriver::update( int timeout_ms )
{
  if ( !udpSocket.isOk() && !getHostname().empty() && port != 0 )
  { if ( !connect( getHostname(), port ) )
      return;
  }
  
  bool success = udpSocket.receiveNextPacket( timeout_ms );

  uint64_t currentTime = getmsec();

  while ( success )
  {
    lastRecvTime = currentTime;

    _u64 type = ((_u64*)udpSocket.packetData())[0];


    if ( type == MSG_CMD )
    {
      LidarCmdHeader header;
      
      const char *cmd = &((char*)udpSocket.packetData())[sizeof(LidarCmdHeader)];
      
      if ( strcmp(cmd,"connect") == 0 )
      { if ( isInDevice )
	  sendMotorState();
	else
	  sendPowerUpState();
	
	scanDataList.currentSeqNr = 0;
	sendConnectAcknowledge();
      }

      cmdQueue.push( cmd );
      
      if ( g_Verbose > 0 )
	Lidar::info( "got cmd '%s'", cmd );
    }
    else if ( type == MSG_SCAN_DATA || type == MSG_ENV_DATA )
    {
      if ( udpSocket.packetSize() < sizeof(LidarScanHeader) )
	 Lidar::error( "udpSocket.packetSize() %ld < %ld", udpSocket.packetSize(), sizeof(LidarScanHeader) );

      LidarScanHeader header;
      memcpy( (void*)&header, (void*)udpSocket.packetData(), sizeof(header) );

      if ( header.nodesPerPacket != nodesPerPacket )
	Lidar::error( "header.nodesPerPacket %d != %d", header.nodesPerPacket, nodesPerPacket );

      if ( header.nodesPerPacket > 0 && header.totalNodes < 500000 ) // plausible data test
      {
	if ( header.totalNodes > 8000 )
	  Lidar::error( "Warning: header.totalNodes: %d", header.totalNodes );

	int numPackets = header.totalNodes / header.nodesPerPacket;
    
	if ( header.totalNodes == 0 || header.totalNodes % header.nodesPerPacket > 0 )
	  numPackets += 1;
      
	LidarScanData &scanData = (type == MSG_SCAN_DATA ? scanDataList.getScanData(header.seqNr) : envDataList.getScanData(header.seqNr) );
	
	if ( &scanData == NULL )
	  Lidar::error( "scanData == NULL" );

	scanData.addData( (unsigned char *)udpSocket.packetData(), udpSocket.packetSize() );
	
	if ( g_Verbose > 0 && scanData.complete() )
	  Lidar::info( "recv scanData: %d", scanData.seqNr );
      }
    }

    if ( lastRemoteAddr != udpSocket.remote_addr && !udpSocket.remote_addr.empty() )
      sendConnect();
    
    success = udpSocket.receiveNextPacket( 0 );
  }

/*
  if ( isInDevice )
  { uint64_t statusMilliSec = currentTime - statusTime;
    if ( statusMilliSec > 800 ) // send status every 800 msec
    { sendStatus();
      statusTime = currentTime;
    }
  }
*/

  if ( !isOpen && !getHostname().empty() )
  {
    uint64_t recvMilliSec = currentTime - lastRecvTime;
    if ( recvMilliSec > 1000 ) // send connect if no messages are received for 1sec
    { uint64_t connectMilliSec = currentTime - connectTime;
      if ( connectMilliSec > 1000 ) // send connect every sec
      { sendConnect();
	connectTime = currentTime;
      }
    }
  }
}

bool
LidarVirtualDriver::grabEnvData( LidarRawSampleBuffer &nodes )
{
  return envDataList.grabScanData( nodes, true );
}

bool
LidarVirtualDriver::grabScanData( LidarRawSampleBuffer &nodes, int timeout, bool grabLatest )
{
  update( timeout );

  return scanDataList.grabScanData( nodes, grabLatest );
}

std::string
LidarVirtualDriver::getNextCmd()
{
  std::string result;

  if ( cmdQueue.empty() )
    return result;
  
  result = cmdQueue.front();

  cmdQueue.pop();
  
  return result;
}

bool
LidarVirtualDriver::setMotorState( bool state )
{
  motorState = state;
  
  return sendMotorState();
}

bool
LidarVirtualDriver::setUSBPower( bool on )
{
  powerUpState = on;
  
  return sendPowerUpState();
}

