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


#include "trackableHUB.h"
#include "TrackBase.h"
#include "libwebsockets.h"

/***************************************************************************
*** 
*** DEFINES
***
****************************************************************************/

// 0 for unlimited
#define MAX_BUFFER_SIZE 0

/***************************************************************************
*** 
*** GLOBALS
***
****************************************************************************/

static int g_Verbose = 0;

/***************************************************************************
*** 
*** TrackableHUB
***
****************************************************************************/

void
TrackableHUB::setVerbose( int level )
{
  g_Verbose = level;
}

    
static bool
consume( unsigned char *&in, size_t &len, unsigned char *out, int numBytes )
{
  memcpy( out, in, numBytes );
  len -= numBytes;
  in += numBytes;

  return true;
}

bool
TrackableHUB::decodeFrame( PackedTrackable::BinaryFrame &frame )
{
  static uint64_t	frame_id = 0;
      
  ObsvObjects objects;

  objects.frame_id  = ++frame_id;

  if ( !PackedPlayer::decodeFrame( objects, frame ) )
    return false;	

  if ( g_Verbose )
    printf( "got %ld trackables\n", objects.size() );


//  setObjects( objects );

  return true;
}



bool
TrackableHUB::decodePacket( unsigned char *in, size_t len )
{
  PackedTrackable::Header &header( *(PackedTrackable::Header*)in );
  if ( header.zero != 0 )
    return false;

  switch ( header.flags & PackedTrackable::TypeBits )
  {
    case PackedTrackable::StartHeader:
    {
 //printf( "start / stop\n" );
 //     g_Track.m_Stage->observer->start( header.timestamp );
      observe( header );
      break;
    }

    case PackedTrackable::StopHeader:
    { //printf( "stop\n" );
 //     g_Track.m_Stage->observer->stop( header.timestamp );
      observe( header );
      break;
    }

    case PackedTrackable::FrameHeader:
    {
      PackedTrackable::BinaryFrame frame;

      if ( !consume( in, len, (unsigned char *)&frame.header, sizeof(frame.header) ) )
	return false;

      if ( !consume( in, len, (unsigned char *)&frame.uuid, sizeof(frame.uuid) ) )
	return false;

      PackedTrackable::Binary binary;
      for ( int i = 0; i < frame.header.size; ++i )
      { if ( !consume( in, len, (unsigned char *)&binary, sizeof(binary) ) )
	  return false;
	frame.push_back( binary );
      }
  
      observe( frame );
      
//      decodeFrame( frame );
      
      break;
    }

    default:
    {
      printf( "unknown header type\n" );
      return false;
      break;
    }
  }

  return true;
}


/***************************************************************************
*** 
*** websocket
***
****************************************************************************/

static struct lws *web_socket = NULL;
struct lws_context *context   = NULL;

static int callback_client( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
  switch( reason )
  {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
    {
      if ( g_Verbose )
	printf("[Main Service] Connect with server success.\n");
      lws_callback_on_writable( wsi );
      break;
    }
      
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    {
      TrackGlobal::error("[Main Service] Connect with server error: %s", in ? (char *)in : "(null)");
      web_socket = NULL;
      break;
    }

    case LWS_CALLBACK_CLIENT_CLOSED:
    {
      if ( g_Verbose )
	printf("[Main Service] LWS_CALLBACK_CLOSED\n");
      web_socket = NULL;
      break;
    }

    case LWS_CALLBACK_CLIENT_RECEIVE:
    {
//      printf("[Main Service] LWS_CALLBACK_CLIENT_RECEIVE: %ld\n", len );
      TrackableHUB::instance()->decodePacket( (unsigned char *)in, len );

	  /* Handle incomming messages here. */
      break;
    }
      
    case LWS_CALLBACK_CLIENT_WRITEABLE:
    {
      unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 512 + LWS_SEND_BUFFER_POST_PADDING];
      unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];

      printf("[Main Service] LWS_CALLBACK_CLIENT_WRITEABLE\n");
//			size_t n = sprintf( (char *)p, "%u", rand() );
//			lws_write( wsi, p, n, LWS_WRITE_TEXT );
      break;
    }

    default:
//      printf("[Main Service] catch reason: %d\n", reason );
      
      break;
  }

  return 0;
}

static struct lws_protocols client_protocols[] = {
  {
    "trackable",
    callback_client,
    0, // user data struct not used
    MAX_BUFFER_SIZE,
  },
  { NULL, NULL, 0, 0 } // terminator
};


static void
ws_init()
{
  struct lws_context_creation_info info;
  memset( &info, 0, sizeof(info) );

  info.port = CONTEXT_PORT_NO_LISTEN;
  info.iface = NULL;
  info.protocols = client_protocols;

  info.user = (void*)NULL; /* HERE */

#ifndef LWS_NO_EXTENSIONS
  info.extensions = lws_get_internal_extensions();
#endif

  info.ssl_cert_filepath        = NULL;
  info.ssl_private_key_filepath = NULL;

  info.gid = -1;
  info.uid = -1;
  info.options = 0;

    // keep alive
  info.ka_time = 60; // 60 seconds until connection is suspicious
  info.ka_probes = 10; // 10 probes after ^ time
  info.ka_interval = 10; // 10s interval for sending probes
//    this->_context = lws_create_context( &info );

  context = lws_create_context( &info );
}

static bool
ws_connect( const char *hostName, int port )
{
  struct lws_client_connect_info ccinfo;
  memset( &ccinfo, 0, sizeof(ccinfo) );

  ccinfo.context = context;
  ccinfo.address = hostName;
  ccinfo.port = port;
  ccinfo.path = "/";
//  ccinfo.host = ccinfo.address;
  ccinfo.host = lws_canonical_hostname( context );
  ccinfo.origin = ccinfo.address;
  ccinfo.protocol = client_protocols[0].name;
  ccinfo.pwsi = &web_socket;

  web_socket = lws_client_connect_via_info(&ccinfo);

  return web_socket != NULL;
}

bool
TrackableHUB::update()
{
  uint64_t now = getmsec();
  
  if ( web_socket == NULL && now - lastConnectionTime > 1000 )
  {
    if ( !ws_connect( m_Host.c_str(), m_Port ) )
      fprintf( stderr, "ERROR: connecting to %s:%d\n", m_Host.c_str(), m_Port );  

    lastConnectionTime = now;
  }

  if ( web_socket != NULL )
    lws_service( context, 250 );
  else
    return false;

  return true;
}


/***************************************************************************
***
*** TrackableHUB
***
****************************************************************************/


TrackableHUB::TrackableHUB()
  : m_Host( "localhost" ),
    m_Port( 5000 ),
    lastConnectionTime( 0 ),
    m_IsConnected( false ),
    m_DiscardTime( 0 )
{
}

void
TrackableHUB::setEndpoint( const char *host, int port )
{
  m_Host = host;
  m_Port = port;
}


TrackableHUB *
TrackableHUB::instance()
{
  static TrackableHUB *instance = NULL;
  if ( instance == NULL )
  {
    instance = new TrackableHUB();

    ws_init();
  }

  return instance;
}

