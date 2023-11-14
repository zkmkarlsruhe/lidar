// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef TRACKABLE_MQTT_OBSERVER_H
#define TRACKABLE_MQTT_OBSERVER_H

#include <mutex>
#include <thread>
#include <atomic>

#include <chrono>
#include <unistd.h>

#include <string.h>
#include <math.h>

#include <mosquitto.h>

/***************************************************************************
*** 
*** TrackableMQTTObserver
***
****************************************************************************/


class TrackableMQTTObserver : public TrackableObserver
{
public:
  struct mosquitto *mosq;
  std::string	    topic;
  std::string	    caFile;
  std::string	    caPath;
  std::string	    certFile;
  std::string	    keyFile;
  std::string	    keyPasswd;
  std::atomic<bool> isConnected;
  
  static int pw_callback( char *buf, int size, int rwflag, void *userdata )
  { TrackableMQTTObserver *observer = static_cast<TrackableMQTTObserver*>(userdata);
    strncpy( buf, observer->keyPasswd.c_str(), size-1 );
    return strlen( buf );
  }

  TrackableMQTTObserver( const char *url )
  : TrackableObserver(),
    mosq	     ( NULL ),
    topic	     ( "v1/devices/me/telemetry" ),
    isConnected	     ( false )
  {
    type 	   = MQTT;
    continuous	   = false;
    fullFrame      = false;
    isJson         = true;
    isThreaded     = true;
    name           = "mqtt";

    setURL( url );

    obsvFilter.parseFilter( "timestamp=ts,action=running,start=true,stop=false,type,enter,leave,id,lifespan,count" );
  }

  ~TrackableMQTTObserver()
  {
    if ( mosq != NULL )
    { mosquitto_lib_cleanup();
      mosquitto_destroy(mosq);
      mosq = NULL;
    }
  }

  virtual void setParam( KeyValueMap &descr )
  {
    TrackableObserver::setParam( descr );
    
    descr.get( "caFile",    caFile );
    descr.get( "caPath",    caPath );
    descr.get( "certFile",  certFile );
    descr.get( "keyFile",   keyFile );
    descr.get( "keyPasswd", keyPasswd  );
  }
  
  static void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
  {
	/* Print out the connection result. mosquitto_connack_string() produces an
	 * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
	 * clients is mosquitto_reason_string().
	 */
    TrackableMQTTObserver *observer = (TrackableMQTTObserver *) obj;
    
//    printf("TrackableMQTTObserver: on_connect: %s\n", mosquitto_connack_string(reason_code));

    observer->lock();

    if(reason_code != 0){
	  /* If the connection fails for any reason, we don't want to keep on
	   * retrying in this example, so disconnect. Without this, the client
	   * will attempt to reconnect. */
      error("TrackableMQTTObserver(%s): Error: on_connect: %s", observer->name.c_str(), mosquitto_connack_string(reason_code));

      mosquitto_disconnect(mosq);
      observer->mosq = NULL;
      observer->isConnected = false;
    }
    else
      observer->isConnected  = true;

    observer->unlock();

	/* You may wish to set a flag here to indicate to your application that the
	 * client is now connected. */
  }

  void setURL( const char *url )
  {
    setFileName( url );

//    connect( url );
  }

  void disconnect()
  {
    lock();
    bool isDisconnected = (mosq == NULL);
    unlock();
      
    if ( isDisconnected )
      return;
    
    flush();
    
    isConnected  = false;

    lock();

    mosquitto_disconnect(mosq);
    mosq = NULL;

    unlock();
  }
  
  bool connect( const char *url )
  {
    lock();

    if ( mosq != NULL )
    { unlock();
      return false;
    }

    std::string hostname;
    std::string username;
    int port = 1883;

    const char *s  = url;
    const char *at = strstr(s, "@");
    if ( at != 0 )
    { 
      std::string prefix;
      prefix.assign(s, at);
      
      const char *col = strstr(prefix.c_str(), ":");
      if ( col != 0 )
      { username.assign(prefix.c_str(), col);
	topic = &col[1];
      }
      else
	username = prefix;

      s = at+1;
    }
    
    const char *col = strstr(s, ":");
    if ( col != 0 )
    { hostname.assign(s, col);
      s = col+1;
      port = atoi(s);
    }
    else
      hostname = s;
    
    mosq = mosquitto_new(NULL, true, this);
    if(mosq == NULL)
    { unlock();
      error( "TrackableMQTTObserver(%s): Error: Out of memory", name.c_str() );
      return false;
    }

      /* Configure callbacks. This should be done before connecting ideally. */
    mosquitto_connect_callback_set(mosq, on_connect);

//    printf( "connect: %s : %s @ %s : %d\n", username.c_str(), topic.c_str(), hostname.c_str(), port );

    if ( !username.empty() )
      mosquitto_username_pw_set(mosq, username.c_str(), NULL );

    if ( (!caFile.empty() || !caPath.empty()) && (certFile.empty() == keyFile.empty()) )
      mosquitto_tls_set( mosq, caFile.c_str(), caPath.c_str(), certFile.c_str(), keyFile.c_str(), pw_callback );

      /* Connect to test.mosquitto.org on port port, with a keepalive of 10 seconds.
       * This call makes the socket connection only, it does not complete the MQTT
       * CONNECT/CONNACK flow, you should use mosquitto_loop_start() or
       * mosquitto_loop_forever() for processing net traffic. */

    int rc = mosquitto_connect(mosq, hostname.c_str(), port, 10);
    if(rc != MOSQ_ERR_SUCCESS){
      mosquitto_destroy(mosq);
      mosq = NULL;
      unlock();
      error( "TrackableMQTTObserver(%s): connect(%s,%d): Error: %s", name.c_str(), hostname.c_str(), port, mosquitto_strerror(rc));
      return false;
    }

      /* Run the network loop in a background thread, this call returns quickly. */
    rc = mosquitto_loop_start(mosq);
    if(rc != MOSQ_ERR_SUCCESS){
      mosquitto_destroy(mosq);
      mosq = NULL;
      unlock();
      error( "TrackableMQTTObserver(%s): connect(%s,%d): Error: %s", name.c_str(), hostname.c_str(), port, mosquitto_strerror(rc));
      return false;
    }

    unlock();

    return true;
  }

  bool start( uint64_t timestamp=0 )
  {
    if ( !isConnected && mosq == NULL )
      connect( logFileTemplate.c_str() );

    if ( !TrackableObserver::start(timestamp) )
      return false;

    return true;
  }
  
  bool stop( uint64_t timestamp=0 )
  { 
    if ( !TrackableObserver::stop(timestamp) )
      return false;

    disconnect();

    return true;
  }

  void write( std::vector<std::string> &messages, uint64_t timestamp=0 )
  { 
    if ( timestamp == 0 )
      timestamp = getmsec();

    if ( !isConnected )
    {
      if ( mosq == NULL )
	return;
      
      uint64_t start_time = getmsec();
      uint64_t now = start_time;
      
      while ( !isConnected && mosq != NULL && now - start_time < 2000 ) // try to connect for 2 sec
      { usleep( 10000 );
	now = getmsec();
      }
      
      if ( !isConnected )
	return;
    }
    
    for ( int i = 0; i < messages.size(); ++i )
    {
/*
      std::string message( "{\"" );

      message += obsvFilter.kmc(Filter::ObsvMQTTTele);
      message += "\":";
      message += messages[i];
      message += "}";
*/
      std::string &message( messages[i] );

      if ( verbose )
	info( "TrackableMQTTObserver(%s) publish: %s", name.c_str(), message.c_str() );
      
      int rc = mosquitto_publish(mosq, NULL, topic.c_str(), message.length(), message.c_str(), 0, false);
      if( rc != MOSQ_ERR_SUCCESS )
	error( "TrackableMQTTObserver(%s): Error publishing: %s", name.c_str(), mosquitto_strerror(rc));
    }
  }
  
};


#endif // TRACKABLE_OBSERVER_H

