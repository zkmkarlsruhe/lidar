// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef TRACKABLE_OSC_OBSERVER_H
#define TRACKABLE_OSC_OBSERVER_H

#include <mutex>
#include <thread>
#include <atomic>

#include <chrono>
#include <unistd.h>

#include <string.h>
#include <math.h>

#include <lo/lo_cpp.h>

/***************************************************************************
*** 
*** TrackableOSCObserver
***
****************************************************************************/

class TrackableOSCObserver : public TrackableObserver
{
public:
  lo::Address 	loa;
  
  std::string	version;
  bool          msgEmpty;

  lo::ServerThread *m_ServerThread;

  int		hasLock;

  void lock()
  { 
    if ( !hasLock )
      mutex.lock();
    hasLock += 1;
  }

  void unlock()
  {
    hasLock -= 1;
    if ( !hasLock )
      mutex.unlock();
  }


  TrackableOSCObserver( const char *url )
  : TrackableObserver(),
    m_ServerThread( NULL ),
    loa( url ),
    hasLock( 0 )
  {
    type 	   = OSC;
    continuous	   = true;
    fullFrame	   = false;
    isJson         = false;
    isThreaded     = false;
    name           = "osc";
    
//    if ( verbose )
//      printf( "TrackableOSCObserver: using port %d\n", port );
    
//    setFileName( url );
    setClientURL( url );

    obsvFilter.parseFilter( "frame,frame_id,frame_end,object,move,x,y,size,id" );
  }

  ~TrackableOSCObserver()
  {
    if ( m_ServerThread != NULL )
      delete m_ServerThread;
  }

  std::string cleanupURL( const char *URL )
  {
    std::string url( URL );
    
    if ( !startsWith( url, "osc" ) )
    {
      if ( startsWith( url, "udp" ) || startsWith( url, "tcp" ) )
	url = "osc." + url;
      else
 	url = "osc.udp://" + url;
    }
    
    return url;
  }

  void setClientURL( const char *URL )
  {
    std::string url( cleanupURL(URL) );
    
    setFileName( url.c_str() );

    if ( verbose )
      info( "TrackableOSCObserver(%s) set client url: %s", name.c_str(), url.c_str() );
    
    loa = lo::Address( url );
  }

  bool startServer( int port )
  {
    if ( m_ServerThread != NULL )
      return true;
    
    lo::ServerThread *serverThread = new lo::ServerThread( port );
    if ( !serverThread->is_valid() )
    { error( "TrackableOSCObserver(%s): Error starting server on port: %d !!!", name.c_str(), port );
      delete serverThread;
      return false;
    }

//    serverThread->set_callbacks([serverThread](){printf("Thread init: %p.\n",serverThread); return 0;},
//				[](){printf("Thread cleanup.\n");});

    serverThread->add_method(NULL, NULL,
                   [this](const char *p, lo_message msg)->int
                   {
		     std::string path( p );
		     
		     lo_address a = lo_message_get_source( msg );

		     const char *host = lo_address_get_hostname(a);
		     const char *port = lo_address_get_port(a);

		     if ( verbose )
		       info( "TrackableOSCObserver(%s): got msg %s from %s:%d", name.c_str(), path.c_str(), host, std::atoi(port) );

		     std::string resetPath( "/" );
		     resetPath += obsvFilter.kmc( Filter::ObsvReset );

		     if ( path == resetPath )
		       reset();
		     
		     return 0;
		   });
 
    if ( verbose )
      info( "TrackableOSCObserver(%s) starting server on port: %d", name.c_str(), port );
    
    m_ServerThread = serverThread;

    m_ServerThread->start();

    return true;
  }

  void setParam( KeyValueMap &descr )
  {
    TrackableObserver::setParam( descr );
    
    if ( descr.get( "version", version ) && !version.empty() && version.front() != '/' )
      version = std::string("/") + version;
    
    int serverPort;
    if ( descr.get( "serverPort", serverPort ) )
      startServer( serverPort );
  }
  
  inline void addName( lo::Message &msg, const std::string &name )
  { 
    if ( !name.empty() )
    { msg.add( "s", name.c_str() );
      msgEmpty = false;
    }
  }

  inline void add( lo::Message &msg, const std::string &name, uint64_t value )
  { 
    if ( std::isdigit(name[0]) )
    { if ( value != atoi(name.c_str()) )
	return;
    }
    else
      addName( msg, name );
    msg.add( (int64_t)value );
    msgEmpty = false;
  }

  inline void add( lo::Message &msg, const std::string &name, int64_t value )
  { 
    if ( std::isdigit(name[0]) )
    { if ( value != atoi(name.c_str()) )
	return;
    }
    else
      addName( msg, name );
    msg.add( value );
    msgEmpty = false;
  }

  inline void add( lo::Message &msg, const std::string &name, int32_t value )
  { if ( std::isdigit(name[0]) )
    { if ( value != atoi(name.c_str()) )
	return;
    }
    else
      addName( msg, name );
    msg.add( value );
    msgEmpty = false;
  }

  inline void add( lo::Message &msg, const std::string &name, float value )
  { addName( msg, name );
    msg.add( value );
    msgEmpty = false;
  }

  inline void add( lo::Message &msg, const std::string &name, const std::string &value )
  { addName( msg, name );
    msg.add( "s", value.c_str() );
    msgEmpty = false;
  }


  inline void addSchemeValue( std::string &string, lo::Message &msg, bool &hasUpdate, bool &hasStatic,bool&hasDynamic, uint64_t timestamp, ObsvObjects *objects=NULL, ObsvObject *object=NULL )
  { 
    ObsvValue value( getObsvValue( string.c_str(), hasUpdate, hasStatic, hasDynamic, timestamp, objects, object ) );
    if ( std::holds_alternative<float>(value) )
      msg.add( std::get<float>(value) );
    else if ( std::holds_alternative<int32_t>(value) )
      msg.add( std::get<int32_t>(value) );
    else if ( std::holds_alternative<int64_t>(value) )
      msg.add( std::get<int64_t>(value) );
    else if ( std::holds_alternative<std::string>(value) )
      msg.add( "s", std::get<std::string>(value).c_str() );
  }

  inline void addStamp( lo::Message &msg )
  {
    if ( obsvFilter.filterEnabled( Filter::FRAME_ID ) )
       add( msg, obsvFilter.km(Filter::FrameId), frame_id );

    if ( obsvFilter.filterEnabled( Filter::TIMESTAMP ) )
      add( msg, obsvFilter.km(Filter::Timestamp), timestamp );
  }
  
  void addObject( ObsvObjects &objects, ObsvObject &object, lo::Message &msg )
  {
    bool enterEnabled = obsvFilter.filterEnabled( Filter::OBSV_ENTER );
    bool moveEnabled  = obsvFilter.filterEnabled( Filter::OBSV_MOVE  );
    bool leaveEnabled = obsvFilter.filterEnabled( Filter::OBSV_LEAVE );

    if ( !(enterEnabled || moveEnabled || leaveEnabled) && (obsvFilter.filterEnabled( Filter::OBSV_OBJECTS ) || obsvFilter.filterEnabled( Filter::OBSV_OBJECT )) )
      moveEnabled = true;
    
    bool reportMove  = ((object.status == ObsvObject::Move)  &&  moveEnabled  && (continuous || object.d >= reportDistance));
    bool reportEnter = ((object.status == ObsvObject::Enter) &&  enterEnabled);
    bool reportLeave = ((object.status == ObsvObject::Leave) && (leaveEnabled || obsvFilter.filterEnabled( Filter::OBSV_LIFESPAN )));

    if ( !(reportEnter || reportMove || reportLeave) )
      return;

    object.moveDone();

    if ( !fullFrame )
    {
      if ( obsvFilter.filterEnabled( Filter::FRAME_ID ) )
	add( msg, obsvFilter.km(Filter::FrameId), objects.frame_id );

      if ( obsvFilter.filterEnabled( Filter::TIMESTAMP ) )
	add( msg, obsvFilter.km(Filter::Timestamp), objects.timestamp );

      if ( obsvFilter.filterEnabled( Filter::OBSV_REGION ) && !objects.region.empty() )
	add( msg, obsvFilter.kmc(Filter::ObsvRegion), objects.region );
    }
    
    if ( obsvFilter.filterEnabled( Filter::OBSV_ID ) )
      add( msg, obsvFilter.kmc(Filter::ObsvId), (int32_t)object.id );

    if ( obsvFilter.filterEnabled( Filter::OBSV_UUID ) )
      add( msg, obsvFilter.kmc(Filter::ObsvUUID), object.uuid.str() );

    if ( obsvFilter.filterEnabled( Filter::OBSV_POSITION ) )
    {
      msgEmpty = false;
      addName( msg, obsvFilter.kmc(Filter::ObsvPosition) ); 
      msg.add( object.x - objects.centerX );
      msg.add( object.y - objects.centerY );
      if ( !std::isnan(object.z)  )
	msg.add( object.z - objects.centerZ );
    }
    else
    {
      if ( obsvFilter.filterEnabled( Filter::OBSV_X ) )
	add( msg, obsvFilter.kmc(Filter::ObsvX), object.x - objects.centerX  );

      if ( obsvFilter.filterEnabled( Filter::OBSV_Y ) )
	add( msg, obsvFilter.kmc(Filter::ObsvY), object.y - objects.centerY  );

      if ( obsvFilter.filterEnabled( Filter::OBSV_Z ) && !std::isnan(object.z)  )
	add( msg, obsvFilter.kmc(Filter::ObsvZ), object.z - objects.centerZ  );
    }
    
    if ( obsvFilter.filterEnabled( Filter::OBSV_SIZE ) )
      add( msg, obsvFilter.kmc(Filter::ObsvSize), object.size );

    if ( obsvFilter.filterEnabled( Filter::OBSV_REGION ) && !objects.region.empty() )
      add( msg, obsvFilter.kmc(Filter::ObsvRegion), objects.region );

    if ( obsvFilter.filterEnabled( Filter::OBSV_TYPE ) )
    { if ( reportEnter )
	add( msg, obsvFilter.kmc(Filter::ObsvType), obsvFilter.kmc(Filter::ObsvEnter) );
      if ( reportMove  )
	add( msg, obsvFilter.kmc(Filter::ObsvType), obsvFilter.kmc(Filter::ObsvMove) );
      if ( reportLeave )
	add( msg, obsvFilter.kmc(Filter::ObsvType), obsvFilter.kmc(Filter::ObsvLeave) );
    }

    if ( (object.status & ObsvObject::Leave) && obsvFilter.filterEnabled( Filter::OBSV_LIFESPAN ) )
      add( msg, obsvFilter.kmc(Filter::ObsvLifeSpan), object.timestamp - object.timestamp_enter );
  }
  
  void send( const std::string &prefix, lo::Message &msg )
  { 
    loa.send( version+obsvFilter.kmprefix("/",prefix), msg );
  }
  
  void addSchemeComponent( ObsvObjects *objects, ObsvObject *object, std::string &component, lo::Message &msg, bool &hasUpdate, bool &hasStatic, bool &hasDynamic, uint64_t timestamp )
  {
    if ( component.front() == '<' && component.back() == '>' )
    {
      std::string key( component.substr(1,component.length()-2) );
      addSchemeValue( key, msg, hasUpdate, hasStatic, hasDynamic, timestamp, objects, object );
      return;
    }

    std::string result( schemeComponentAsString( component, hasUpdate, hasStatic, hasDynamic, timestamp, objects, object ) );
    msg.add( "s", result.c_str() );
  }

  void reportScheme( std::vector<SchemeMessage> &scheme, uint64_t timestamp, ObsvObjects *objects=NULL, ObsvObject *object=NULL )
  {
    for ( int i = 0; i < scheme.size(); ++i )
    {
      if ( schemeCondition( scheme[i], timestamp, objects, object ) )
      {
	lo::Message msg;
	bool hasUpdate  = false;
	bool hasStatic  = false;
	bool hasDynamic = false;

	std::vector<std::string> &components( scheme[i].components );
      	  
	std::string adressPattern( schemeComponentAsString( components[0], hasUpdate, hasStatic, hasDynamic, timestamp, objects, object ) );

	for ( int c = 1; c < components.size(); ++c )
	  addSchemeComponent( objects, object, components[c], msg, hasUpdate, hasStatic, hasDynamic, timestamp );

	if ( hasUpdate || (hasStatic&&!hasDynamic) || scheme[i].forceUpdate )
	  loa.send( adressPattern, msg );
      }
    }
  }

  void report()
  {
    if ( hasScheme )
    { 
      reportSchemes();
      return;
    }

    bool reportObjects = hasReportObjects();
      
    if ( fullFrame )
    {
      bool hasMsg = false;
      lo::Message msg;
      msgEmpty = true;
    
      for ( int i = rects.numRects()-1; i >= 0; --i )
      {
	ObsvObjects &objects( rects.rect(i).objects );
	bool msgEmptyBak = msgEmpty;
	msgEmpty = true;

	if ( obsvFilter.filterEnabled( Filter::OBSV_COUNT ) )
        { if ( continuous || objects.lastCount != (int)objects.validCount )
	    add( msg, obsvFilter.kmc(Filter::ObsvCount), (int)objects.validCount );
	}
	if ( obsvFilter.filterEnabled( Filter::OBSV_SWITCH ) )
        { if ( continuous || ((bool)objects.lastCount) != (bool)objects.validCount )
	    add( msg, obsvFilter.kmc(Filter::ObsvSwitch), (int)(bool)objects.validCount );
	}
	if ( obsvFilter.filterEnabled( Filter::OBSV_ALIVE ) )
        { if ( objects.alive )
	    add( msg, obsvFilter.kmc(Filter::ObsvAlive), objects.alive );
	}

	if ( reportObjects )
        { for ( auto &iter: objects )
          { ObsvObject &object( iter.second );
	    addObject( objects, object, msg );
	  }
	}

	bool objectsMsgEmpty = msgEmpty;
	msgEmpty = msgEmptyBak;
	
	if ( !objectsMsgEmpty )
	  hasMsg = true;

	if ( !objectsMsgEmpty && obsvFilter.filterEnabled( Filter::OBSV_REGION ) && !objects.region.empty() )
	  add( msg, obsvFilter.kmc(Filter::ObsvRegion), objects.region );
      }
      
      if ( hasMsg )
	send( Filter::Frame, msg );
    }
    else
    {
      for ( int i = rects.numRects()-1; i >= 0; --i )
      {
	ObsvObjects &objects( rects.rect(i).objects );
	
	lo::Message msg;
	msgEmpty = true;
    
	if ( obsvFilter.filterEnabled( Filter::FRAME ) )
        { addStamp( msg );
	  msgEmpty = false;
	}

	if ( obsvFilter.filterEnabled( Filter::OBSV_COUNT ) )
	{ if ( continuous || objects.lastCount != (int)objects.size() )
	    add( msg, obsvFilter.kmc(Filter::ObsvCount), (int)objects.size() );
	}
	if ( obsvFilter.filterEnabled( Filter::OBSV_SWITCH ) )
        { if ( continuous || ((bool)objects.lastCount) != (bool)objects.size() )
	    add( msg, obsvFilter.kmc(Filter::ObsvSwitch), (int)(bool)objects.size() );
	}

	if ( !msgEmpty && obsvFilter.filterEnabled( Filter::OBSV_REGION ) && !objects.region.empty() )
	  add( msg, obsvFilter.kmc(Filter::ObsvRegion), objects.region );
    
	if ( !msgEmpty )
	  send( Filter::Frame, msg );

	if ( reportObjects )
        { for ( auto &iter: objects )
          { ObsvObject &object( iter.second );
	    lo::Message msg;
	    msgEmpty = true;

	    addObject( objects, object, msg );
      
	    if ( !msgEmpty )
	      send( Filter::ObsvObject, msg );
	  }
	}
      }
    }
    
    if ( obsvFilter.filterEnabled( Filter::FRAME_END ) )
    { lo::Message msg;
      msgEmpty = true;
      addStamp( msg );
      //      if ( !msgEmpty )
	send( Filter::FrameEnd, msg );
    }
  }
  
  bool observe( const ObsvObjects &other, bool force=false )
  {
    lock();
    bool result = TrackableObserver::observe( other, force );
    unlock();

    return result;
  }
  
  bool start( uint64_t timestamp=0 )
  {
    lock();
    bool result = TrackableObserver::start( timestamp );
    unlock();

    return result;
  }
  
  bool stop( uint64_t timestamp=0 )
  {
    lock();
    bool result = TrackableObserver::stop( timestamp );
    unlock();

    return result;
  }
  
  void	reset( uint64_t timestamp=0 )
  {
    lock();
    TrackableObserver::reset( timestamp );
    unlock();
  }
  
};


#endif // TRACKABLE_OSC_OBSERVER_H

