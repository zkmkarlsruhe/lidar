// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef TRACKABLE_INFLUXDB_OBSERVER_H
#define TRACKABLE_INFLUXDB_OBSERVER_H

#include <mutex>
#include <thread>
#include <atomic>

#include <chrono>
#include <unistd.h>

#include <string.h>
#include <math.h>

#include "webAPI.h"

/***************************************************************************
*** 
*** TrackableInfluxDBObserver
***
****************************************************************************/

class TrackableInfluxDBObserver : public TrackableObserver
{
public:
  WebAPI 	   *webAPI;
  std::string	    apiURL;
  std::string	    url;
  std::string	    protocol;
  std::string	    host;
  std::string	    bucket;
  std::string	    token;
  std::string	    auth;
  std::string	    orgID;
  std::string	    org;
  std::string	    tags;
  int		    apiVersion;
  int		    port;
  int		    batchSize;
  int		    batchSec;
  uint64_t	    lastWrittenTime;
  
  TrackableInfluxDBObserver()
  : TrackableObserver(),
    webAPI( NULL ),
    apiVersion( 1 ),
    protocol( "http" ),
    host( "localhost" ),
    port( 8086 ),
    lastWrittenTime( 0 )
  {
    type 	   = InfluxDB;
    continuous	   = true;
    fullFrame      = true;
    isJson         = false;
    isThreaded     = false;
    name           = "influxdb";
    batchSize	   = 5000;
    batchSec	   = 5;
    maxFPS	   = 5;

    obsvFilter.parseFilter( "x,y,size,uuid" );
  }

  ~TrackableInfluxDBObserver()
  {
    if ( webAPI != NULL )
    { delete webAPI;
      webAPI = NULL;
    }
  }

  void setParam( KeyValueMap &descr )
  {
    TrackableObserver::setParam( descr );
    
    descr.get( "bucket",   bucket );
    descr.get( "tags",     tags );
    descr.get( "api",      apiVersion );
    descr.get( "url",      url );
    descr.get( "protocol", protocol );
    descr.get( "host",     host );
    descr.get( "port",     port );
    descr.get( "auth",     auth );
    descr.get( "token",    token  );
    descr.get( "org",      org  );
    descr.get( "orgID",    orgID  );
    descr.get( "batch",    batchSize );
    descr.get( "batchSec", batchSec );
  }

  void startThread()
  { 
  }
  
  bool createWebAPI()
  {
    if ( !url.empty() )
      apiURL = url;
    else
    {
      apiURL = protocol;
      apiURL += "://";
      apiURL += host;
      apiURL += ":";
      apiURL += std::to_string( port );
    }
    
    if ( apiVersion == 2 )
      apiURL += "/api/v2";
      
    apiURL += "/write?bucket=";
    apiURL += bucket;
    apiURL += "&precision=ms";
    
    if ( !org.empty() )
    { apiURL += "&org=";
      apiURL += org;
    }
    else if ( !orgID.empty() )
    { apiURL += "&orgID=";
      apiURL += orgID;
    }

    setFileName( apiURL.c_str() );

    webAPI = new WebAPI();

    if ( isThreaded )
      webAPI->setThreaded( true );

    if ( !auth.empty() || !token.empty() )
    { std::string header( "Authorization: " );
      if ( !auth.empty() )
	header += auth;
      else if ( !token.empty() )
      { header += " Token ";
	header += token;
      }
      webAPI->addHeader( header.c_str() );
    }

    return true;  
  }
    
  void addValue( Filter::ObsvFilterFlag flag, const char *filter, std::string value, const ObsvObject &object, std::string &message, bool &first )
  {
    if ( !obsvFilter.filterEnabled( flag ) )
      return;
    
    if ( !first )
      message += ",";
    else
      first = false;
    
    message += obsvFilter.kmc( filter );
    message += "=";
    message += value;
  }
  
  void addObject( const ObsvObjects &objects, const ObsvObject &object, const char *region=NULL )
  {
    if ( isThreaded && thread == NULL )
      startThread();

    std::string message( "track" );
  
    if ( !tags.empty() )
    { message += ",";
      message += tags;
    }

    if ( obsvFilter.filterEnabled( Filter::OBSV_UUID ) )
    { message += ",";
      message += obsvFilter.kmc(Filter::ObsvUUID);
      message += "=";
      message += ((ObsvObject*)&object)->uuid.str();
    }

    if ( obsvFilter.filterEnabled( Filter::OBSV_ID ) )
    { message += ",id=";
      message += object.id;
    }

    if ( region != NULL && region[0] != '\0' )
    { message += ",";
      message += obsvFilter.kmc(Filter::ObsvRegion);
      message += "=";
      message += region;
    }

    message += " ";

    bool first = true;
    addValue( Filter::OBSV_X,    Filter::ObsvX,    std::to_string(object.x-objects.centerX),    object, message, first );
    addValue( Filter::OBSV_Y,    Filter::ObsvY,    std::to_string(object.y-objects.centerY),    object, message, first );
    addValue( Filter::OBSV_Z,    Filter::ObsvZ,    std::to_string(object.z-objects.centerZ),    object, message, first );
    addValue( Filter::OBSV_SIZE, Filter::ObsvSize, std::to_string(object.size), object, message, first );

    message += " ";
    message += std::to_string( object.timestamp );

    messages.push_back( message );
  }

  void write( std::vector<std::string> &messages, uint64_t timestamp=0 )
  {
    if ( webAPI == NULL )
      return;
    
    if ( messages.size() == 0 )
      return;

    if ( !webAPI->isReady() )
    { 
      int dropCount = 0;
      
      while ( messages.size() > 10000 )
      { messages.erase( messages.begin() );
	dropCount += 1;
      }
      
      error( "TrackableInfluxDBObserver(%s,%s) not ready dropping %d messages\n", name.c_str(), apiURL.c_str(), dropCount );

      return;
    }

    int totalSize = 0;
    
    for ( int i = 0; i < messages.size(); ++i )
      totalSize += messages[i].length();

    if ( totalSize > 0 )
    {
      char batch[totalSize+messages.size()];
      int p = 0;
      
      for ( int i = 0; i < messages.size(); ++i )
      {
	int length = messages[i].length();
	memcpy( &batch[p], messages[i].c_str(), length );
	p += length;

	if ( i == messages.size()-1 )
	  batch[p++] = '\0';
	else
	  batch[p++] = '\n';
      }

      if ( verbose )
	info( "TrackableInfluxDBObserver(%s,%s): %s\n", name.c_str(), apiURL.c_str(), batch );

      if ( !test )
	webAPI->post( batch, sizeof(batch)-1, apiURL.c_str() );
    }

    messages.clear();
  }
  
  bool observe( const ObsvObjects &other, bool force=false )
  { 
    if ( webAPI != NULL && webAPI->hasReturnData() )
    {
      if ( verbose )
      {
	std::vector<uint8_t> &returnData( webAPI->returnData() );
	
	if ( webAPI->returnData().size() > 0 )
        { std::string result( webAPI->returnDataStr() );
	  info( "TrackableInfluxDBObserver(%s,%s) returned: %s\n", name.c_str(), apiURL.c_str(), result.c_str() );
	}
      }
      
      webAPI->clearReturnData();
    }
    
    if ( maxFPS <= 0.0 )
      maxFPS = 1;
    else if ( maxFPS > 10.0 )
      maxFPS = 10.0;

    if ( !TrackableObserver::observe( other, force ) )
      return false;
    
    if ( !reporting )
      return false;
    
    if ( webAPI == NULL )
      createWebAPI();

    bool reportRegions = (obsvFilter.filterEnabled( Filter::OBSV_REGIONS ) || obsvFilter.filterEnabled( Filter::OBSV_REGION ));

    if ( reportRegions )
    { for ( int i = rects.numRects()-1; i >= 0; --i )
      { ObsvObjects &objects( rects.rect(i).objects );
	for ( auto &iter: objects )
        { const ObsvObject &object( iter.second );
	  addObject( objects, object, objects.region.c_str() );
	}
      }
    }
    
    if ( !reportRegions || !rects.rect(0).objects.region.empty() )
    { 
      const ObsvObjects &objects( rects.rect(0).objects );
      for ( auto &iter: other )
      { const ObsvObject &object( iter.second );
	if ( useLatent || !object.isLatent() )
	  addObject( objects, object );
      }
    }
    
    if ( messages.size() >= batchSize || other.timestamp - lastWrittenTime > batchSec*1000 )
    { write( messages, other.timestamp );
      lastWrittenTime = other.timestamp;
    }
    
    return true;
  }

  bool start( uint64_t timestamp=0 )
  {
    if ( !TrackableObserver::start(timestamp) )
      return false;

    if ( webAPI == NULL )
      createWebAPI();
    
    return true;
  }
  
  bool stop( uint64_t timestamp=0 )
  { 
    if ( !TrackableObserver::stop(timestamp) )
      return false;

    write( messages );

    return true;
  }

  
};


#endif // TRACKABLE_OBSERVER_H

