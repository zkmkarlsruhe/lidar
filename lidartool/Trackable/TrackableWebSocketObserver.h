// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef TRACKABLE_WEBSOCKET_OBSERVER_H
#define TRACKABLE_WEBSOCKET_OBSERVER_H

#include <mutex>
#include <thread>
#include <atomic>

#include <chrono>
#include <unistd.h>

#include <string.h>
#include <math.h>

#include <cppWebSockets/WebSocketServer.h>

/***************************************************************************
*** 
*** WebSocketObserver
***
****************************************************************************/

class WebSocketObserver : public TrackableObserver, public WebSocketServer
{
public:

  WebSocketObserver( int port=5000 )
  : TrackableObserver(),
    WebSocketServer( port )
  {
    continuous	   = true;
    fullFrame      = true;
    port           = port;
    isThreaded     = true;
  }

  ~WebSocketObserver()
  {
  }

  int numConnections()
  {
    lock();
    int count = getNumberOfConnections();
    unlock();
    return count;
  }
  
  void onConnect( int socketID )
  {
    if ( verbose )
      info( "WebSocketObserver(%s) New connection from %s", name.c_str(), getValue( socketID, "remoteIP" ).c_str() );
  }
  
  void onDisconnect( int socketID )
  {
    if ( verbose )
      info( "WebSocketObserver(%s) Disconnected %s", name.c_str(), getValue( socketID, "remoteIP" ).c_str() );
  }
  
  void onError( int socketID, const string &message )
  {
    error( "WebSocketObserver(%s,%s) Error: %s", name.c_str(), getValue( socketID, "remoteIP" ).c_str(), message.c_str() );
  }

  void write( std::vector<std::string> &msgs, uint64_t timestamp=0 )
  {
    if ( numConnections() == 0 )
      return;

    for ( int i = 0; i < msgs.size(); ++i )
    {
      std::string &message( msgs[i] );

      if ( verbose )
	info( "WebSocketObserver(%s) send: %s", name.c_str(), message.c_str() );

      broadcast( message.c_str(), message.length() );
    }
  }
  
};


/***************************************************************************
*** 
*** TrackableWebSocketObserver
***
****************************************************************************/

class TrackableWebSocketObserver : public WebSocketObserver
{
  void threadFunction()
  {
    lock();
    int timeout = (messages.size() == 0);
    if ( connections.size() == 0 )
      timeout = 10;
    unlock();
    
    TrackableObserver::threadFunction();

    wait( timeout );
  }
  
public:
  TrackableWebSocketObserver( int port=5000 )
  : WebSocketObserver( port )
  {
    type 	   = WebSocket;
    continuous	   = true;
    fullFrame      = true;
    isJson         = true;
    isThreaded     = true;
    name           = "websocket";

    if ( verbose )
      info( "TrackableWebSocketObserver: opening WEBSOCKET on port %d", port );
    
    obsvFilter.parseFilter( "timestamp=ts,action=running,start=true,stop=false,frame,frame_id,objects,type,enter,move,leave,x,y,z,size,id,lifespan,count" );
  }

  ~TrackableWebSocketObserver()
  {
  }

  void onMessage( int socketID, const string &data )
  {
    if ( verbose )
      info( "TrackableWebSocketObserver(%s,%s) onMessage: %s", name.c_str(), getValue( socketID, "remoteIP" ).c_str(), data.c_str() );

    std::string fullFramePrefix( "fullFrame:" );
    std::string reportDistancePrefix( "reportDistance:" );
    std::string streamDataPrefix( "streamData:" );
    std::string continuousPrefix( "continuous:" );
    std::string reset( obsvFilter.kmc(Filter::ObsvReset) );
    std::string filterPrefix( "filter:" );
    
#ifdef JSON_TOOL_H
    if ( data[0] == '{' )
    {
      rapidjson::StringStream s(data.c_str());
      rapidjson::Document json;
      json.ParseStream( s );

      if ( json.IsObject() )
      {
 	rapidjson::fromJson( json, "continuous",     continuous );
	rapidjson::fromJson( json, "fullFrame",      fullFrame );
	rapidjson::fromJson( json, "streamData",     reporting );
	rapidjson::fromJson( json, "reportDistance", reportDistance );

	bool reset;
 	if ( rapidjson::fromJson( json, obsvFilter.kmc(Filter::ObsvReset), reset ) )
	  this->reset();
	
	std::string filter;
	if ( rapidjson::fromJson( json, "filter", filter ) )
	  obsvFilter.parseFilter( filter.c_str() );
      }
    }
    else
#endif
    if ( startsWith( data, filterPrefix ) )
    { std::string filter( &data.c_str()[filterPrefix.length()] ); 
      trim( filter, "\"" );
      trim( filter, "'" );
      if ( verbose )
	info( "got filter: %s", filter.c_str() );
      obsvFilter.parseFilter( filter.c_str() );
    }
    else if ( startsWith( data, continuousPrefix ) )
    { getBool( &data.c_str()[continuousPrefix.length()], continuous );
      if ( verbose )
	info( "got continuous: %d", continuous );
    }
    else if ( data == reset )
    { if ( verbose )
	printf( "got reset\n" );
      rects.reset();
    }
    else if ( startsWith( data, continuousPrefix ) )
    { getBool( &data.c_str()[continuousPrefix.length()], continuous );
      if ( verbose )
	info( "got continuous: %d", continuous );
    }
    else if ( startsWith( data, streamDataPrefix ) )
    { getBool( &data.c_str()[streamDataPrefix.length()], reporting );
      if ( verbose )
	printf( "got streamData: %d", reporting );
    }
    else if ( startsWith( data, fullFramePrefix ) )
    { getBool( &data.c_str()[fullFramePrefix.length()], fullFrame );
      if ( verbose )
	info( "got fullFrame: %d", fullFrame );
    }
    else if ( startsWith( data, reportDistancePrefix ) )
    { ::getValue( &data.c_str()[reportDistancePrefix.length()], reportDistance );
      if ( verbose )
	info( "got reportDistance: %g", reportDistance );
    }
  }
  
};


/***************************************************************************
*** 
*** TrackablePackedWebSocketObserver
***
****************************************************************************/

class TrackablePackedWebSocketObserver : public WebSocketObserver, PackedTrackable::Stream
{
  std::vector<std::vector<uint8_t>> msgs;
  std::vector<uint8_t> msg;

  void threadFunction()
  {
    lock();

    int timeout = (msgs.size() == 0);
    if ( connections.size() == 0 )
      timeout = 10;
 
    if ( msgs.size() > 0 )
    { 
      for ( int i = 0; i < msgs.size(); ++i )
	broadcast( (const char *)&msgs[i][0], msgs[i].size() );
      
      msgs.resize( 0 );
    }

    unlock();

    wait( timeout );
  }
  
public:
  
  TrackablePackedWebSocketObserver( int port=5000 )
  : WebSocketObserver( port )
  {
    _binary        = true;
    type 	   = PackedWebSocket;
    continuous	   = true;
    fullFrame      = true;
    isJson         = false;
    isThreaded     = true;
    name           = "packedwebsocket";

//    if ( verbose )
      printf( "TrackablePackedWebSocketObserver: opening WEBSOCKET on port %d\n", port );
   }

  ~TrackablePackedWebSocketObserver()
  {
  }
  
  bool flushMsg()
  { 
    if ( verbose )
      printf( "TrackablePackedWebSocketObserver(%s) send: %ld bytes\n", name.c_str(), msg.size() );

    lock();
    
    msgs.push_back( msg );
    msg.resize( 0 );
    
    unlock();

    return true;
  }

  virtual int read( unsigned char *buffer, int size )
  { return false;
  }

  virtual bool write( const unsigned char *buffer, int size )
  { 
    int oldSize = msg.size();
    
    msg.resize( oldSize + size );
    memcpy( &msg[oldSize], buffer, size );

    return true;
  }

  bool start( uint64_t timestamp=0 )
  {
    msg.resize( 0 );
    
    if ( timestamp == 0 )
      timestamp = getmsec();
    
    if ( !TrackableObserver::start(timestamp) )
      return false;

    if ( !reporting || numConnections() == 0 )
      return true;
    
    PackedTrackable::Header header( timestamp, PackedTrackable::StartHeader );
    put( header );

    return flushMsg();
  }
  
  bool stop( uint64_t timestamp=0 )
  { 
    if ( timestamp == 0 )
      timestamp = getmsec();
    
    if ( !TrackableObserver::stop(timestamp) )
      return false;

    if ( !reporting || numConnections() == 0 )
      return true;

    PackedTrackable::Header header( timestamp, PackedTrackable::StopHeader );
    put( header );

    return flushMsg();
  }

  bool observe( const ObsvObjects &other, bool force )
  { 
    if ( maxFPS <= 0.0 )
      maxFPS = 15;
    else if ( maxFPS > 60.0 )
      maxFPS = 60.0;

    if ( !TrackableObserver::observe( other, force ) )
      return false;
    
    if ( !reporting || numConnections() == 0 )
      return true;

    PackedTrackable::BinaryFrame frame( other.timestamp, other.uuid );

    if ( smoothing <= 0.0 )
    {
      for ( auto &iter: other )
      { const ObsvObject &object( iter.second );
      
	if ( useLatent || !object.isLatent() )
	  frame.add( object.id, object.x, object.y, object.size, object.flags ); 
      }
    }
    else
    {
      ObsvRect &rect( rects.rect(0) );
      ObsvObjects &objects( rect.objects );

      for ( auto &iter: objects )
	iter.second.status = ObsvObject::Invalid;

      for ( auto &iter: other )
      { const ObsvObject &object( iter.second );

	if ( useLatent || !object.isLatent() )
	{
	  ObsvObject *obj = objects.get( object.id );
	
	  if ( obj == NULL )
          { auto pair( objects.emplace(object.id,object) );
	    obj = &pair.first->second;
	    obj->objects = &objects;
	    obj->status  = ObsvObject::Enter;
	    obj->track( object );
	    obj->moveDone();
	    obj->update();
	  }
          else
	  { obj->track( object, smoothing );
	    obj->status = ObsvObject::Move;
	    obj->edge   = ObsvRect::Edge::EdgeNone;
	  }

	  obj->flags = object.flags;

	  frame.add( obj->id, obj->x, obj->y, obj->size, obj->flags ); 
	}
      }

      for ( auto iter = objects.begin(); iter != objects.end(); )
      { 
	if ( iter->second.status == ObsvObject::Invalid )
	  iter = objects.erase(iter);
	else 
	  ++iter;
      }
    }

    put( frame );
    
    return flushMsg();
  }

  void onMessage( int socketID, const string &data )
  {
    if ( verbose )
      printf( "TrackableWebSocketObserver(%s,%s) onMessage: %s\n", name.c_str(), getValue( socketID, "remoteIP" ).c_str(), data.c_str() );

  }
  
  void onConnect( int socketID )
  {
   
    if ( verbose )
      printf( "TrackableWebSocketObserver(%s) New connection from %s\n", name.c_str(), getValue( socketID, "remoteIP" ).c_str() );

    if ( isStarted > 0 )
    {
      PackedTrackable::Header header( timestamp, PackedTrackable::StartHeader ); 
      send( socketID, (const char *)&header, sizeof(header) );
    }
    else if ( isStarted = 0 )
    {
      PackedTrackable::Header header( timestamp, PackedTrackable::StopHeader ); 
      send( socketID, (const char *)&header, sizeof(header) );
    }
  }

};


#endif // TRACKABLE_WEBSOCKET_OBSERVER_H

