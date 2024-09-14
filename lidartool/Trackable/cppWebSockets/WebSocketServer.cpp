/** --------------------------------------------------------------------------
 *  WebSocketServer.cpp
 *
 *  Base class that WebSocket implementations must inherit from.  Handles the
 *  client connections and calls the child class callbacks for connection
 *  events like onConnect, onMessage, and onDisconnect.
 *
 *  Author    : Jason Kruse <jason@jasonkruse.com> or @mnisjk
 *  Copyright : 2014
 *  License   : BSD (see LICENSE)
 *  --------------------------------------------------------------------------
 **/

#include <stdlib.h>
#include <string>
#include <cstring>
#include <sys/time.h>
#include <fcntl.h>
#include <mutex>
#include "libwebsockets.h"
#include "Util.h"
#include "WebSocketServer.h"

using namespace std;

// 0 for unlimited
#define MAX_BUFFER_SIZE 0

// Nasty hack because certain callbacks are statically defined

static int callback_main(   struct lws *wsi,
                            enum lws_callback_reasons reason,
                            void *user,
                            void *in,
                            size_t len )
{
    int result = 0;
  
    int fd;
    switch( reason ) {
        case LWS_CALLBACK_ESTABLISHED:
        {
	  char name[200], rip[200];
	  WebSocketServer *self = static_cast<WebSocketServer*>(lws_context_user(lws_get_context(wsi)));
	  self->mutex.lock();
	  fd = lws_get_socket_fd( wsi );
	  lws_get_peer_addresses( wsi, fd, name, 200, rip, 200 );
	  self->onConnectWrapper( fd, rip );
	  self->mutex.unlock();
	  lws_callback_on_writable( wsi );
	  break;
	}
        case LWS_CALLBACK_SERVER_WRITEABLE:
        {
	    WebSocketServer *self = static_cast<WebSocketServer*>(lws_context_user(lws_get_context(wsi)));
	    self->mutex.lock();
	    self->mutex.unlock();
	  
	    fd = lws_get_socket_fd( wsi );
    	    
	    if ( fd <= 0 )
            { result = -1;
	      break;
	    }
	    
	    bool success = true;

	    if ( self->connections.count(fd) && !self->connections[fd]->writeBuffer.empty() )
            {
	      std::vector<uint8_t> &message = self->connections[fd]->writeBuffer.front();
	      int msgLen = message.size();
	      const int packSize = self->packSize;
	
	      int bufSize = (msgLen > packSize ? packSize : msgLen);

	      unsigned char *buf = &self->writeBuffer[0];

	      int size;
	      int charsSent = self->connections[fd]->charsSent;

	      size = msgLen-charsSent;
	      if ( size > packSize )
		size = packSize;

	      memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, &message[charsSent], size);
	      lws_write_protocol n = (lws_write_protocol) lws_write_ws_flags( self->_binary?LWS_WRITE_BINARY:LWS_WRITE_TEXT, charsSent == 0, charsSent + size == msgLen );

	      int sent = lws_write(wsi,&buf[LWS_SEND_BUFFER_PRE_PADDING],size,n);

	      if ( sent > 0 )
		charsSent += sent;
	      else if ( sent == 0 )
		success = false;
	      else
   	      { charsSent = 0;
		success   = false;
		self->onError( fd, std::string( "Error writing to socket" ) );
		result = -1;
	      }

	      if ( charsSent == msgLen )
              { charsSent = 0;
		self->connections[fd]->writeBuffer.pop_front();
	      }

	      self->connections[fd]->charsSent = charsSent;
	    }

	    if ( success )
	      lws_callback_on_writable( wsi );
            break;
	}
        case LWS_CALLBACK_RECEIVE:
        {
	    WebSocketServer *self = static_cast<WebSocketServer*>(lws_context_user(lws_get_context(wsi)));
	    self->mutex.lock();
	    self->mutex.unlock();
	    int fd = lws_get_socket_fd( wsi );
	    
	    bool isFinal = lws_is_final_fragment(wsi);
	    std::vector<uint8_t> &readBuffer( self->connections[fd]->readBuffer );
	    
	    if ( isFinal && readBuffer.empty() )
	      self->onMessage( fd, std::string( (const char *)in, len ) );
	    else
            { int size = readBuffer.size();
	      readBuffer.resize( size + len );
	      memcpy( &readBuffer[size], static_cast<const uint8_t *>(in), len );

	      if ( isFinal )
   	      { len = readBuffer.size();
		self->onMessage( fd, std::string( (const char *)&readBuffer[0], len) );
		readBuffer.resize( 0 );
	      }
	    }

           break;
	}
        case LWS_CALLBACK_CLOSED:
        {
	    WebSocketServer *self = static_cast<WebSocketServer*>(lws_context_user(lws_get_context(wsi)));
            self->onDisconnectWrapper( lws_get_socket_fd( wsi ) );
	    result = -1;
            break;
	}
        default:
            break;
    }
    return result;
}

static struct lws_protocols protocols[] = {
  {
    "trackable",
    callback_main,
    0, // user data struct not used
    MAX_BUFFER_SIZE,
  },
  { NULL, NULL, 0, 0 } // terminator
};

WebSocketServer::WebSocketServer( int port, const string certPath, const string& keyPath, bool binary )
{
    this->_binary   = binary;
    this->_port     = port;
    this->_certPath = certPath;
    this->_keyPath  = keyPath;
    lws_set_log_level( 0, lwsl_emit_syslog ); // We'll do our own logging, thank you.
    struct lws_context_creation_info info;
    memset( &info, 0, sizeof info );
    info.port = this->_port;
    info.iface = NULL;
    info.protocols = protocols;
    info.user = (void*)this; /* HERE */

#ifndef LWS_NO_EXTENSIONS
    info.extensions = lws_get_internal_extensions( );
#endif

    if( !this->_certPath.empty( ) && !this->_keyPath.empty( ) )
    {
        Util::log( "Using SSL certPath=" + this->_certPath + ". keyPath=" + this->_keyPath + "." );
        info.ssl_cert_filepath        = this->_certPath.c_str( );
        info.ssl_private_key_filepath = this->_keyPath.c_str( );
    }
    else
    {
        Util::log( "Not using SSL" );
        info.ssl_cert_filepath        = NULL;
        info.ssl_private_key_filepath = NULL;
    }
    info.gid = -1;
    info.uid = -1;
    info.options = 0;

    // keep alive
    info.ka_time = 60; // 60 seconds until connection is suspicious
    info.ka_probes = 10; // 10 probes after ^ time
    info.ka_interval = 10; // 10s interval for sending probes
    this->_context = lws_create_context( &info );
    if( !this->_context )
      fprintf( stderr, "libwebsocket init failed" );
    Util::log( "Server started on port " + Util::toString( this->_port ) );

    // Some of the libwebsocket stuff is define statically outside the class. This
    // allows us to call instance variables from the outside.  Unfortunately this
    // means some attributes must be public that otherwise would be private.
}

WebSocketServer::~WebSocketServer()
{
    // Free up some memory
    for( map<int,Connection*>::const_iterator it = this->connections.begin( ); it != this->connections.end( ); ++it )
    {
        Connection* c = it->second;
        this->connections.erase( it->first );
        delete c;
    }
}

void WebSocketServer::onConnectWrapper( int socketID, const char *remoteIP )
{
    Connection* c = new Connection;
    c->createTime = time( 0 );
    c->charsSent  = 0;
    c->keyValueMap["remoteIP"] = remoteIP;
    
    this->connections[ socketID ] = c;
    this->onConnect( socketID );
}

void WebSocketServer::onDisconnectWrapper( int socketID )
{
    this->onDisconnect( socketID );
    this->_removeConnection( socketID );
}

void WebSocketServer::onErrorWrapper( int socketID, const string& message )
{
    Util::log( "Error: " + message + " on socketID '" + Util::toString( socketID ) + "'" );
    this->onError( socketID, message );
    this->_removeConnection( socketID );
}

void WebSocketServer::send( int socketID, const char *data, int length )
{
    // Push this onto the buffer. It will be written out when the socket is writable.
  this->connections[socketID]->writeBuffer.push_back( std::vector<uint8_t>(data, &data[length]) );
}

void WebSocketServer::broadcast( const char *data, int length )
{
  for( map<int,Connection*>::const_iterator it = this->connections.begin( ); it != this->connections.end(); ++it )
    this->send( it->first, data, length );

  _deleteRemovedConnections();
}

void WebSocketServer::setValue( int socketID, const string& name, const string& value )
{
    this->connections[socketID]->keyValueMap[name] = value;
}

string WebSocketServer::getValue( int socketID, const string& name )
{
    return this->connections[socketID]->keyValueMap[name];
}

int WebSocketServer::getNumberOfConnections( )
{
  _deleteRemovedConnections();

   return this->connections.size();
}

void WebSocketServer::wait( uint64_t timeout )
{
  _deleteRemovedConnections();

  if( lws_service( this->_context, timeout ) < 0 )
    fprintf( stderr, "WebSocketServer::wait(): Error polling for socket activity." );
}

void WebSocketServer::_removeConnection( int socketID )
{
  connectionsToRemove.emplace( socketID );
}

void WebSocketServer::_deleteRemovedConnections()
{
  for ( auto socketID: this->connectionsToRemove )
  {
    if ( this->connections.find( socketID ) != this->connections.end() )
    { Connection* c = this->connections[ socketID ];
      this->connections.erase( socketID );
      delete c;
    }
  }

  this->connectionsToRemove.clear();
}
 
