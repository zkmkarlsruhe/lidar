// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef TRACKABLE_OBSERVER_H
#define TRACKABLE_OBSERVER_H

#include "helper.h"

#include <mutex>
#include <thread>
#include <atomic>

#include <chrono>
#include <unistd.h>
#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

#include <string.h>
#include <math.h>
#include <functional>
#include <filesystem>
#include <variant>

#include "UUID.h"

#include "filterTool.h"

#include "PackedTrackable.h"

#include <signal.h>

#include <arpa/inet.h> // htons, inet_addr
#include <netinet/in.h> // sockaddr_in
#include <sys/types.h> // uint16_t
#include <sys/socket.h> // socket, sendto
#include <netdb.h>
#include <sys/time.h>

/***************************************************************************
*** 
*** ObsvFilter
***
****************************************************************************/

namespace Filter 
{

  enum ObsvFilterFlag : uint64_t   {
    OBSV_FRAME        = FRAME,
    OBSV_FRAME_ID     = FRAME_ID,
    OBSV_FRAME_END    = FRAME_END,
    OBSV_TIMESTAMP    = TIMESTAMP,
    OBSV_ID	      = ID,
    OBSV_OBJECTS      = OBJECTS,
    OBSV_OBJECT       = OBJECT,
    OBSV_POSITION     = POSITION,
    OBSV_X	      = POS_X,
    OBSV_Y	      = POS_Y,
    OBSV_Z	      = POS_Z,
    OBSV_SIZE	      = SIZE,
    OBSV_TYPE	      = (1ULL<<13),
    OBSV_ENTER        = (1ULL<<14),
    OBSV_MOVE         = (1ULL<<15),
    OBSV_LEAVE        = (1ULL<<16),
    OBSV_ENTEREDGE    = (1ULL<<17),
    OBSV_LEAVEEDGE    = (1ULL<<18),
    OBSV_ENTERCOUNT   = (1ULL<<19),
    OBSV_LEAVECOUNT   = (1ULL<<20),
    OBSV_GATECOUNT    = (1ULL<<21),
    OBSV_LIFESPAN     = (1ULL<<22),
    OBSV_AVGLIFESPAN  = (1ULL<<23),
    OBSV_START	      = (1ULL<<24),
    OBSV_STOP	      = (1ULL<<25),
    OBSV_ACTION       = (1ULL<<26),
    OBSV_COUNT        = (1ULL<<27),
    OBSV_SWITCH	      = (1ULL<<28),
    OBSV_SWITCH_DURATION  = (1ULL<<29),
    OBSV_ALIVE        = (1ULL<<30),
    OBSV_OPERATIONAL  = (1ULL<<31),
    OBSV_RESET        = (1ULL<<32),
    OBSV_REGIONS      = (1ULL<<33),
    OBSV_REGION       = (1ULL<<34),
    OBSV_UUID         = (1ULL<<35),
    OBSV_REGIONX      = (1ULL<<36),
    OBSV_REGIONY      = (1ULL<<37),
    OBSV_REGIONWIDTH  = (1ULL<<38),
    OBSV_REGIONHEIGHT = (1ULL<<39),
    OBSV_RUNMODE      = (1ULL<<40),
    OBSV_STATISTICS   = (1ULL<<41)
};

static inline const char *ObsvFrame       = Frame;
static inline const char *ObsvFrameId     = FrameId;
static inline const char *ObsvFrameEnd    = FrameEnd;
static const char *ObsvTimestamp    = Timestamp;
static const char *ObsvObjects 	    = Objects;
static const char *ObsvObject 	    = Object;
static const char *ObsvId 	    = Id;
static const char *ObsvPosition     = Position;
static const char *ObsvX 	    = PosX;
static const char *ObsvY 	    = PosY;
static const char *ObsvZ 	    = PosZ;
static const char *ObsvSize 	    = Size;
static const char *ObsvType 	    = "type";
static const char *ObsvEnter	    = "enter";
static const char *ObsvMove	    = "move";
static const char *ObsvLeave	    = "leave";
static const char *ObsvEnterEdge    = "enteredge";
static const char *ObsvLeaveEdge    = "leaveedge";
static const char *ObsvEnterCount   = "gateentercount";
static const char *ObsvLeaveCount   = "gateleavecount";
static const char *ObsvGateCount    = "gatecount";
static const char *ObsvLifeSpan	    = "lifespan";
static const char *ObsvAvgLifeSpan  = "avglifespan";
static const char *ObsvStart	    = "start";
static const char *ObsvStop	    = "stop";
static const char *ObsvAction	    = "action";
static const char *ObsvCount	    = "count";
static const char *ObsvSwitch	    = "switch";
static const char *ObsvSwitchDuration   = "switchduration";
static const char *ObsvAlive	    = "alive";
static const char *ObsvOperational  = "operational";
static const char *ObsvReset        = "reset";
static const char *ObsvRegions	    = "regions";
static const char *ObsvRegion	    = "region";
static const char *ObsvUUID	    = "uuid";
static const char *ObsvRegionX      = "region_x";
static const char *ObsvRegionY      = "region_y";
static const char *ObsvRegionWidth  = "region_width";
static const char *ObsvRegionHeight = "region_height";
static const char *ObsvRunMode      = "runmode";
static const char *ObsvStatistics   = "statistics";

class ObsvFilter : public Filter
{
public:
  ObsvFilter()
    : Filter()
  {
    object_id = "%id";

    addFilter( OBSV_FRAME,  	  ObsvFrame );
    addFilter( OBSV_FRAME_ID,  	  ObsvFrameId );
    addFilter( OBSV_FRAME_END,    ObsvFrameEnd  );
    addFilter( OBSV_TIMESTAMP,    ObsvTimestamp );
    addFilter( OBSV_ID,   	  ObsvId  );
    addFilter( OBSV_OBJECTS,   	  ObsvObjects );
    addFilter( OBSV_OBJECT,   	  ObsvObject );
    addFilter( OBSV_TYPE,   	  ObsvType  );
    addFilter( OBSV_ENTER, 	  ObsvEnter );
    addFilter( OBSV_MOVE, 	  ObsvMove );
    addFilter( OBSV_LEAVE, 	  ObsvLeave );
    addFilter( OBSV_ENTEREDGE,    ObsvEnterEdge );
    addFilter( OBSV_LEAVEEDGE, 	  ObsvLeaveEdge );
    addFilter( OBSV_ENTERCOUNT,   ObsvEnterCount );
    addFilter( OBSV_LEAVECOUNT,   ObsvLeaveCount );
    addFilter( OBSV_GATECOUNT,    ObsvGateCount );
    addFilter( OBSV_POSITION, 	  ObsvPosition );
    addFilter( OBSV_X, 		  ObsvX );
    addFilter( OBSV_Y, 		  ObsvY );
    addFilter( OBSV_Z, 		  ObsvZ );

    addFilter( OBSV_SIZE,    	  ObsvSize );
    addFilter( OBSV_LIFESPAN,	  ObsvLifeSpan );
    addFilter( OBSV_AVGLIFESPAN,  ObsvAvgLifeSpan );
    addFilter( OBSV_START,	  ObsvStart );
    addFilter( OBSV_STOP,    	  ObsvStop );
    addFilter( OBSV_ACTION,    	  ObsvAction );
    addFilter( OBSV_COUNT,    	  ObsvCount );
    addFilter( OBSV_SWITCH,    	  ObsvSwitch );
    addFilter( OBSV_SWITCH_DURATION,  ObsvSwitchDuration );
    addFilter( OBSV_ALIVE,    	  ObsvAlive );
    addFilter( OBSV_OPERATIONAL,  ObsvOperational );
    addFilter( OBSV_RESET,    	  ObsvReset );

    addFilter( OBSV_REGIONS,   	  ObsvRegions );
    addFilter( OBSV_REGION,    	  ObsvRegion );
    addFilter( OBSV_REGIONX,      ObsvRegionX );
    addFilter( OBSV_REGIONY,      ObsvRegionY );
    addFilter( OBSV_REGIONWIDTH,  ObsvRegionWidth );
    addFilter( OBSV_REGIONHEIGHT, ObsvRegionHeight );
    addFilter( OBSV_RUNMODE,      ObsvRunMode );
    addFilter( OBSV_STATISTICS,   ObsvStatistics );
    addFilter( OBSV_UUID,    	  ObsvUUID );

    initialized = false;
  }

};

} // namespace Filter 



/***************************************************************************
*** 
*** ObsvUserData
***
****************************************************************************/

class ObsvUserData
{
public:

  virtual ~ObsvUserData() {}

};

/***************************************************************************
*** 
*** ObsvObject
***
****************************************************************************/

class ObsvObjects;

class ObsvObject
{
public:

  enum  ObsvStatus
  { Enter    =  (1<<1),
    Leave    =  (1<<2),
    Move     =  (1<<3),
    Invalid  =      0
  };

  enum  Flags
  { Touched  =  (1<<0),
    Private  =  (1<<1),
    Portal   =  (1<<2),
    Green    =  (1<<3),
    Latent   =  (1<<4),
    Immobile =  (1<<5),
    Default  =      0
  };

  uint64_t	timestamp, timestamp0;
  uint64_t	timestamp_enter;
  uint64_t	timestamp_touched;
  uint64_t	timestamp_private;
  uint64_t 	timestamp_immobile;
  uint32_t	id;
  int		status;
  int		edge;
  uint16_t	flags;
  
  float 	x, y, z, lx, ly, lz, x0, y0, z0;
  float		d;
  float 	size, size0;
  UUID 		uuid;
  
  float		immobilePos[3];

  ObsvObjects   *objects;
  ObsvUserData  *userData;
  
  ObsvObject( uint64_t timestamp=0, int id=0, float x=0.0f, float y=0.0f, float z=NAN, float size=0.5f, int flags=Touched, UUID *uuid=NULL )
  : timestamp ( timestamp ),
    timestamp_private ( 0 ),
    timestamp_immobile( 0 ),
    x   ( x ),
    y   ( y ),
    z   ( z ),
    lx  ( NAN ),
    ly  ( NAN ),
    lz  ( NAN ),
    size( size ),
    flags( flags ),
    objects( NULL ),
    userData( NULL )
    {
      if ( uuid != NULL )
	this->uuid = *uuid;

      if ( flags&Touched )
	timestamp_touched = timestamp;
    }

  ~ObsvObject()
  {
    if ( userData != NULL )
      delete userData;
  }
  
  inline bool		isTouched() const
  { return flags & Touched;
  }
    
  inline void		setTouched( bool set )
  { if ( set == (flags & Touched) )
      return;
    if ( set )
      flags |=  Touched;
    else
      flags &= ~Touched;
  }
  inline bool		isPrivate() const
  { return flags & Private;
  }
    
  inline void		setPrivate( bool set )
  { if ( set == (flags & Private) )
      return;
    if ( set )
      flags |=  Private;
    else
      flags &= ~Private;
  }

  inline void touchPrivate( bool set, uint64_t timestamp, uint64_t timeout )
  { 
    if ( set )
    { if ( timestamp_private == 0 )
	timestamp_private = timestamp;
      else if ( timestamp - timestamp_private > timeout )
	setPrivate( true );
    }
    else
      timestamp_private = 0;
  }
  
  inline bool		isImmobile() const
  { return flags & Immobile;
  }

  inline void		setImmobile( bool set )
  { if ( set == (flags & Immobile) )
      return;
    if ( set )
      flags |=  Immobile;
    else
      flags &= ~Immobile;
  }

  inline void checkImmobile( uint64_t timestamp, uint64_t timeout, float maxDist )
  { 
    double d0 = immobilePos[0] - x;
    double d1 = immobilePos[1] - y;
    double distance = sqrt( d0*d0 + d1*d1 );
    if ( distance > maxDist )
    { immobilePos[0] = x;
      immobilePos[1] = y;
      immobilePos[2] = z;
      timestamp_immobile = timestamp;
      setImmobile( false );
    }
    else
    {
      if ( timestamp_immobile == 0 )
	timestamp_immobile = timestamp;
      else if ( timestamp - timestamp_immobile > timeout )
	setImmobile( true );
    }
  }
  
  inline bool		isLatent() const
  { return flags & Latent;
  }
    
  inline void		setLatent( bool set )
  { if ( set == (flags & Latent) )
      return;
    if ( set )
      flags |=  Latent;
    else
      flags &= ~Latent;
  }

  const char *edgeAsString() const
  { switch ( edge )
    { case 1: return "left"; break;
      case 2: return "right"; break;
      case 3: return "top"; break;
      case 4: return "bottom"; break;
      default: break;
    }

    return "none";
  }

  void	track( const ObsvObject &other, float smoothing=0.0f )
  {
    timestamp 	= other.timestamp;

    if ( smoothing > 0 && !isnan(x) && !isnan(y) && !isnan(size) )
    {
      float oms = 1.0 - smoothing;
      x		= smoothing * x + oms * other.x;
      y		= smoothing * y + oms * other.y;
      if ( !isnan(z) && !isnan(other.z) ) z = smoothing * z + oms * other.z;
      size	= smoothing * size + oms * other.size;
    }
    else
    { x		= other.x;
      y		= other.y;
      z		= other.z;
      size	= other.size;
    }
    
    lx 		= other.lx;
    ly 		= other.ly;
    lz 		= other.lz;
  }

  float	distanceMoved() const
  {
    float dx = x - x0;
    float dy = y - y0;
    dx *= dx;
    dy *= dy;

    if ( isnan(z) || isnan(z0) )
      return sqrt( dx+dy );
    
    float dz = z - z0;
    dz *= dz;

    return sqrt( dx+dy+dz );
  }
  
  void	moveDone()
  {
    timestamp0 	= timestamp;
    size0 	= size;
    x0 		= x;
    y0 		= y;
    z0 		= z;
    d		= 0.0;
  }
  
  void	update()
  {
    lx 		= x;
    ly 		= y;
    lz 		= z;
  }
  
};



/***************************************************************************
*** 
*** ObsvCustom
***
****************************************************************************/

class ObsvCustom
{
public:

  virtual ~ObsvCustom(){};
  
};



/***************************************************************************
*** 
*** ObsvObjects
***
****************************************************************************/

class ObsvRect;

class ObsvObjects : public std::map<int,ObsvObject>
{
public:
  uint64_t 	timestamp;
  uint64_t 	alive_timestamp;
  uint64_t 	switch_timestamp;
  uint64_t 	frame_id;
  int		lastCount,   validCount;
  int		enterCount,  lastEnterCount;
  int		leaveCount,  lastLeaveCount;
  int		gateCount,   lastGateCount;
  int		lastAvgLifespan, avgLifespan, lifespanCount;
  uint64_t 	lifespanSum;
  uint64_t 	switchDurationSum;
  float         centerX, centerY, centerZ;
  float         scaleX,  scaleY,  scaleZ;
  float		operational;
  int		alive;
  UUID		uuid;
  ObsvRect     *rect;
  ObsvCustom   *custom;
  ObsvUserData *userData;
  
  std::string   region;
  
  ObsvObjects()
  : timestamp        (  0 ),
    alive_timestamp  (  0 ),
    switch_timestamp (  0 ),
    lastCount        ( -1 ),
    enterCount       (  0 ),
    leaveCount       (  0 ),
    gateCount        (  0 ),
    lastEnterCount   ( -1 ),
    lastLeaveCount   ( -1 ),
    lastGateCount    ( -1 ),
    lastAvgLifespan  ( -1 ),
    avgLifespan      (  0 ),
    lifespanSum      (  0 ),
    lifespanCount    (  0 ),
    switchDurationSum(  0 ),
    centerX          (  0 ),
    centerY          (  0 ),
    centerZ          (  0 ),
    scaleX           (  1 ),
    scaleY           (  1 ),
    scaleZ           (  1 ),
    operational	     (  1 ),
    alive	     (  1 ),
    custom           (  NULL ),
    userData         (  NULL )
    {}
  
  ~ObsvObjects()
  {
    if ( custom != NULL )
      delete custom;
    if ( userData != NULL )
      delete userData;
  }
  
  ObsvObject *get( int id )
  { ObsvObjects::iterator iter( find(id) );
    if ( iter == end() )
      return NULL;
    return &iter->second;
  }

  const ObsvObject *get( int id ) const
  { ObsvObjects::const_iterator iter( find(id) );
    if ( iter == end() )
      return NULL;
    return &iter->second;
  }

  void	update()
  { for ( auto &iter: *this )
      iter.second.update();
  }

  void clear()
  { validCount       = 0;
    std::map<int,ObsvObject>::clear();
  }
  
};


/***************************************************************************
*** 
*** ObsvRect
***
****************************************************************************/

class ObsvRect
{
public:

  enum Edge
  {
    EdgeNone      = 0,
    EdgeLeft      = 1,
    EdgeRight     = 2,
    EdgeTop       = 3,
    EdgeBottom    = 4
  };

  enum Shape
  {
    ShapeRect     = 0,
    ShapeEllipse  = 1
  };


  std::string name;
  float x, y, width, height;
  bool  invert;
  Edge  edge;
  Shape shape;
  
  ObsvObjects objects;

  ObsvRect()
  : invert( false ),
    edge  ( EdgeNone ),
    shape ( ShapeRect )
  {
  }

  void set( float x=-3.0, float y=-3.0, float width=6.0, float height=6.0, ObsvRect::Edge edge=ObsvRect::Edge::EdgeNone, ObsvRect::Shape shape=ObsvRect::Shape::ShapeRect )
  { this->x      = x;
    this->y      = y;
    this->width  = width;
    this->height = height;
    this->edge   = edge;
    this->shape  = shape;
  }

  void set( const char *name, float x=-3.0, float y=-3.0, float width=6.0, float height=10.0, ObsvRect::Edge edge=ObsvRect::Edge::EdgeNone, ObsvRect::Shape shape=ObsvRect::Shape::ShapeRect )
  { this->objects.region = name;
    this->name   = name;
    this->x      = x;
    this->y      = y;
    this->width  = width;
    this->height = height;
    this->edge   = edge;
    this->shape  = shape;
  }

  bool contains( float x, float y, float size=0.0f ) const
  {
    if ( shape == ShapeRect )
      return x+size >= this->x &&
             x-size <= this->x+this->width &&
             y+size >= this->y &&
	     y-size <= this->y+this->height;
    
    
    x -= this->x + 0.5 * this->width;
    y -= this->y + 0.5 * this->height;    
    y *= this->width / this->height;
    
    return sqrt( x*x + y*y ) <= 0.5 * this->width;
  }

  Edge edgeCrossed( const ObsvObject &obj, ObsvObject::ObsvStatus status ) const
  {
    float x, y;
    
    if ( status == ObsvObject::Leave || isnan(obj.lx) )
    { x = obj.x;
      y = obj.y;
    }
    else
    {
      x = obj.lx;
      y = obj.ly;
    }

    if ( isnan(x) || isnan(y) )
      return EdgeNone;

    float absx = fabs( x );
    float absy = fabs( y );
    
    if ( absx > absy )
    { if ( x < 0 )
	return EdgeLeft;
      return EdgeRight;
    }
    
    if ( y < 0 )
      return EdgeBottom;

    return EdgeTop;
  }


};
 

/***************************************************************************
*** 
*** ObsvRects
***
****************************************************************************/

class ObsvRects : public std::vector<ObsvRect>
{
public:
  ObsvRect	default_rect;
  
  inline void unite( const char *name )
  { default_rect.name = name;
    default_rect.objects.region = name;
  }

  inline int numRects() const
  { return default_rect.name.empty() && size() > 0 ? size() : 1;
  }

  inline const ObsvRect &rect( int i=0 ) const
  { 
    if ( default_rect.name.empty() && size() > 0 )
      return (*this)[i];

    return default_rect;
  }

  inline ObsvRect &rect( int i=0 )
  { 
    if ( default_rect.name.empty() && size() > 0 )
      return (*this)[i];

    return default_rect;
  }

  ObsvRect *get( const char *name )
  {
    if ( default_rect.name.empty() && default_rect.name == name )
      return &default_rect;
    
    for ( int i = 0; i < this->size(); ++i )
      if ( (*this)[i].name == name )
	return &((*this)[i] );
    
    return NULL;
  }

  ObsvRect *set( const char *name, float x=-3.0, float y=-3.0, float width=6.0, float height=6.0, ObsvRect::Edge edge=ObsvRect::Edge::EdgeNone, ObsvRect::Shape shape=ObsvRect::Shape::ShapeRect )
  {
    ObsvRect *rect = get( name );
    if ( rect == NULL )
    { ObsvRect r;
      r.set( name, x, y, width, height, edge, shape );
      push_back( r );
      rect = &back();
    }
    else
      rect->set( x, y, width, height, edge, shape );
    
    return rect;
  }

  ObsvRect *set( float x=-3.0, float y=-3.0, float width=106.0, float height=6.0, ObsvRect::Edge edge=ObsvRect::Edge::EdgeNone, ObsvRect::Shape shape=ObsvRect::Shape::ShapeRect )
  { default_rect.set( x, y, width, height, edge, shape );
    return &default_rect;
  }

  bool contains( int rectIndex, float x, float y, float size )
  {
    if ( this->size() > 0 )
    { 
      size *= 0.5;

      if ( default_rect.name.empty() )
      { ObsvRect &rect( (*this)[rectIndex] );
	bool contains = rect.contains( x, y, size );
	if ( rect.invert )
	  return !contains;
	return contains;
      }
      
	  // unite
      for ( int i = 0; i < this->size(); ++i )
	if ( (*this)[i].contains( x, y, size ) )
	  return !(*this)[i].invert;
      
      return false;
    }
    
    return true;
  }

  ObsvRect::Edge edgeCrossed( int rectIndex, const ObsvObject &obj, ObsvObject::ObsvStatus status ) const
  {
    if ( !default_rect.name.empty() && this->size() != 1 )
      return ObsvRect::EdgeNone;

    const ObsvRect &rect( this->rect(rectIndex) );
    ObsvRect::Edge edge = rect.edgeCrossed( obj, status );

    if ( rect.edge == ObsvRect::Edge::EdgeNone )
      return edge;

    if ( edge != rect.edge )
      return ObsvRect::Edge::EdgeNone;

    return edge;
  }

  int	countEdge( int rectIndex, ObsvRect::Edge edge )
  {
    if ( edge == ObsvRect::EdgeNone || !default_rect.name.empty() && this->size() != 1 )
      return 0;
    
    const ObsvRect &rect( this->rect(rectIndex) );
    if ( edge == rect.edge )
      return 1;
    
    return 0;
  }
  

  void	reset()
  { 
    for ( int i = numRects()-1; i >= 0; --i )
    {
      ObsvObjects &objects( rect(i).objects );
      objects.clear();
      objects.lastCount         = -1;
      objects.enterCount        =  0;
      objects.lastEnterCount    = -1;
      objects.leaveCount        =  0;
      objects.lastLeaveCount    = -1;
      objects.gateCount         =  0;
      objects.lastGateCount     = -1;
      objects.lastAvgLifespan   = -1;
      objects.lifespanSum       =  0;
      objects.lifespanCount     =  0;
      objects.switch_timestamp  =  0;
      objects.switchDurationSum =  0;
    }
  }
  
  void	start()
  {
    for ( int i = numRects()-1; i >= 0; --i )
    {
      ObsvObjects &objects( rect(i).objects );
      objects.lastCount         = -1;
      objects.enterCount        =  0;
      objects.lastEnterCount    = -1;
      objects.leaveCount        =  0;
      objects.lastLeaveCount    = -1;
      objects.gateCount         =  0;
      objects.lastGateCount     = -1;
      objects.lastAvgLifespan   = -1;
      objects.lifespanSum       =  0;
      objects.lifespanCount     =  0;
      objects.switch_timestamp  =  0;
      objects.switchDurationSum =  0;
    }
  }
  
  void	stop()
  {}
  
};
 

/***************************************************************************
*** 
*** TrackableObserver
***
****************************************************************************/

class TrackableObserver
{
protected:
  std::mutex		 mutex;
  std::thread		*thread;
  std::atomic<bool>	 isFlushed;
  bool			 exitThread;
  
  void lock()
  { mutex.lock(); }

  void unlock()
  { mutex.unlock(); }

  void flush()
  {
    uint64_t start_time = getmsec();
    uint64_t now = start_time;
    
    isFlushed = false;
  
    while ( !isFlushed && now - start_time < 2000 ) // try to flush for 2 sec
    { usleep( 10000 );
      now = getmsec();
    }
  }

  static inline void sigHandler( int sig )
  {
    info( "TrackableObserver [%d] caught signal %d", gettid(), sig );
  }

  static inline void setSignalHandler( int sig, struct sigaction &sa )
  {
//    info( "TrackableObserver [%d] catch signal %d\n", gettid(), sig );
    sigaction(sig, &sa, NULL);
  }

  static inline void setSignalHandlers()
  {
    struct sigaction sa;
    sa.sa_handler = sigHandler;
    sigfillset(&sa.sa_mask);
    // setSignalHandler( SIGINT,  sa );
    // setSignalHandler( SIGHUP,  sa );
    // setSignalHandler( SIGTERM, sa );
    setSignalHandler( SIGPIPE, sa );
  }

  virtual void threadFunction()
  {
    usleep( 10*1000 );

    lock();
    std::vector<std::string> msgs( messages );
    messages.clear();
    unlock();

    if ( msgs.size() > 0 )
      write( msgs );

    lock();
    isFlushed = (messages.size() != 0);
    unlock();
  }
  
  void ThreadFunction()
  {
    //    setSignalHandlers();

    while( !exitThread )
    { 
      threadFunction();
    }
  }

  static inline void runThread( TrackableObserver *observer )
  { observer->ThreadFunction(); }

  virtual void startThread()
  { 
    if ( isThreaded && thread == NULL )
      thread = new std::thread( runThread, this );  
  }
  
  virtual void stopThread()
  { if ( thread == NULL )
      return;

    exitThread = true;
    thread->join();
    delete thread;
    thread = NULL;
  }


public:
  class ObsvValue : public std::variant<int64_t, int32_t, float, std::string>
  {
    public:
      const char *name;
      const char *alias;

      ObsvValue( float   value )     { std::variant<int64_t, int32_t, float, std::string>::operator = ( value ); }
      ObsvValue( int64_t value )     { std::variant<int64_t, int32_t, float, std::string>::operator = ( value ); }
      ObsvValue( int32_t value )     { std::variant<int64_t, int32_t, float, std::string>::operator = ( value ); }
      ObsvValue( std::string value ) { std::variant<int64_t, int32_t, float, std::string>::operator = ( value ); }

      std::string asString() const
      {
	if ( std::holds_alternative<int64_t>(*this) )
	  return std::to_string( std::get<int64_t>( *this ) );

	if ( std::holds_alternative<float>(*this) )
        { char s[100];
          sprintf( s, "%g", std::get<float>( *this ) );
	  return std::string( s );
	}
	if ( std::holds_alternative<int32_t>(*this) )
	  return std::to_string( std::get<int32_t>( *this ) );

	return std::get<std::string>( *this ); 
      }
  };

  typedef std::function<ObsvValue(const char*alias,bool&hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects*objects,ObsvObject*object)> ObsvValueGetFunc;

  class ObsvValueGetter
  {
    public:
      const char *name;
      const char *alias;
      ObsvValueGetFunc func;
      ObsvValueGetter( const char *name, const char *alias, ObsvValueGetFunc func )
	: name( name ),
	  alias( alias ),
	  func( func )
	  {}
  };

  class ObsvValueGet : public std::map<std::string, ObsvValueGetter>
  {  
    public:
  };

  ObsvValueGet obsvValueGet;
  bool obsvValueGetInitialized;

  void addObsvValueGet( const char *name, ObsvValueGetFunc func )
  {
    const char *alias;
    std::map<std::string,std::string>::iterator iter( obsvFilter.KeyMap.find(name) );
    if ( iter == obsvFilter.KeyMap.end() )
      alias = name;
    else
      alias = iter->second.c_str();
     
    obsvValueGet.emplace( name, ObsvValueGetter( name, alias, func ) );
  }

  inline bool isMoving( ObsvObject &object )
  { return (continuous || object.d >= reportDistance) && (object.status == ObsvObject::Move); }

  void initObsvValueGet()
  {
    addObsvValueGet( Filter::ObsvX, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate |= isMoving(*object);
      hasDynamic = true;
      return ObsvValue( (object->x - objects->centerX) *  objects->scaleX );
    } );

    addObsvValueGet( Filter::ObsvY, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate |= isMoving(*object);
      hasDynamic = true;
      return ObsvValue( (object->y - objects->centerY) *  objects->scaleY );
    } );

    addObsvValueGet( Filter::ObsvZ, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate |= isMoving(*object);
      hasDynamic = true;
      return ObsvValue( (object->z - objects->centerZ) *  objects->scaleZ );
    } );

    addObsvValueGet( Filter::Size, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate |= isMoving(*object);
      hasDynamic = true;
      return ObsvValue( object->size );
    } );

    addObsvValueGet( Filter::ObsvId, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { return ObsvValue( (int32_t)object->id ); } );

    addObsvValueGet( Filter::ObsvUUID, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { return ObsvValue( object->uuid.str() ); } );

    addObsvValueGet( Filter::ObsvType, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    {
      hasDynamic = true;
      if ( object->status == ObsvObject::Move )
      { hasUpdate |= isMoving(*object);
	return ObsvValue( obsvFilter.kmc(Filter::ObsvMove) );
      }
      if ( object->status == ObsvObject::Enter )
      { hasUpdate = true;
	return ObsvValue( obsvFilter.kmc(Filter::ObsvEnter) );
      }

      hasUpdate = true;
      return ObsvValue( obsvFilter.kmc(Filter::ObsvLeave) );
    } );

    addObsvValueGet( Filter::ObsvEnter, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate = ( object->status == ObsvObject::Enter );
      hasDynamic = true;
      return ObsvValue( alias );
    } );

    addObsvValueGet( Filter::ObsvMove, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate |= isMoving(*object);
      hasDynamic = true;
      return ObsvValue( alias );
    } );

    addObsvValueGet( Filter::ObsvLeave, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate = ( object->status == ObsvObject::Leave );
      hasDynamic = true;
      return ObsvValue( alias );
    } );

    addObsvValueGet( Filter::ObsvLifeSpan, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate |= ( object->status == ObsvObject::Leave );
      hasDynamic = true;
      return ObsvValue( (int64_t)(object->timestamp_touched - object->timestamp_enter) );
    } );

    addObsvValueGet( Filter::ObsvSwitch, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { 
      hasUpdate |= ( continuous || ((bool)objects->lastCount) != (bool)objects->validCount );
      hasDynamic = true;
      return ObsvValue( (int32_t)(bool)objects->validCount );
    } );

    addObsvValueGet( Filter::ObsvSwitchDuration, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { 
      hasUpdate |= ( continuous || (objects->lastCount > 0 && objects->validCount == 0 && objects->switch_timestamp != 0) );
      hasDynamic = true;
      if ( objects->switch_timestamp == 0 )
	return ObsvValue( (int64_t)0 );
      return ObsvValue( (int64_t)(timestamp-objects->switch_timestamp) );
    } );

    addObsvValueGet( Filter::ObsvCount, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate |= ( continuous || objects->lastCount != (int)objects->validCount );
      hasDynamic = true;
      return ObsvValue( (int32_t)objects->validCount );
    } );

    addObsvValueGet( Filter::ObsvAlive, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate |= (bool)objects->alive;
      hasDynamic = true;
      return ObsvValue( (int32_t)(bool)objects->alive );
    } );

    addObsvValueGet( Filter::ObsvOperational, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate |= (bool)objects->alive;
      hasDynamic = true;
      //      printf( "alive: %d\n", (bool)objects->alive );
      return ObsvValue( objects->operational );
    } );

    addObsvValueGet( Filter::ObsvRegion, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasStatic = true;
      return ObsvValue( objects->region );
    } );

    addObsvValueGet( Filter::ObsvRunMode, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasStatic = true;
      return ObsvValue( runMode );
    } );

    addObsvValueGet( Filter::ObsvRegionX, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasStatic = true;
      return ObsvValue( objects->rect->x + objects->rect->width / 2 );
    } );

    addObsvValueGet( Filter::ObsvRegionY, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasStatic = true;
      return ObsvValue( objects->rect->y + objects->rect->height / 2 );
    } );

    addObsvValueGet( Filter::ObsvRegionWidth, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasStatic = true;
      return ObsvValue( objects->rect->width  );
    } );

    addObsvValueGet( Filter::ObsvRegionHeight, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasStatic = true;
      return ObsvValue( objects->rect->height  );
    } );

    addObsvValueGet( Filter::ObsvRegions, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasStatic = true;
      return ObsvValue( objects->region );
    } );

    addObsvValueGet( Filter::ObsvFrameId, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { 
      hasStatic = true;
      return ObsvValue( (int64_t)objects->frame_id );
    } );

    addObsvValueGet( Filter::ObsvEnterCount, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate |= ( continuous || objects->lastEnterCount != (int)objects->enterCount );
      hasDynamic = true;
      return ObsvValue( (int32_t)objects->enterCount );
    } );

    addObsvValueGet( Filter::ObsvLeaveCount, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasStatic = true;
      return ObsvValue( (int32_t)objects->leaveCount );
    } );

    addObsvValueGet( Filter::ObsvGateCount, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasStatic = true;
      return ObsvValue( (int32_t)objects->gateCount );
    } );

    addObsvValueGet( Filter::ObsvAvgLifeSpan, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasStatic = true;
      return ObsvValue( objects->avgLifespan );
    } );

    addObsvValueGet( Filter::ObsvTimestamp, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasStatic = true;
      return ObsvValue( (int64_t)objects->timestamp );
    } );

    addObsvValueGet( Filter::ObsvAction, [this](const char *alias, bool &hasUpdate,bool&hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasDynamic = true;
      if ( startStopStatusChanged == 1 )
      { hasUpdate = true;
	return ObsvValue( obsvFilter.kmc(Filter::ObsvStart ) );
      }
      if ( startStopStatusChanged == 0 )
      { hasUpdate = true;
	return ObsvValue( obsvFilter.kmc(Filter::ObsvStop ) );
      }
      return ObsvValue( alias );
    } );

    addObsvValueGet( Filter::ObsvStart, [this](const char *alias, bool &hasUpdate,bool &hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate |= ( startStopStatusChanged == 1 );
      hasDynamic = true;
      return ObsvValue( alias );
    } );

    addObsvValueGet( Filter::ObsvStop, [this](const char *alias, bool &hasUpdate,bool &hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { hasUpdate |= ( startStopStatusChanged == 0 );
      hasDynamic = true;
      return ObsvValue( alias );
    } );

    addObsvValueGet( "undefined", [this](const char *alias, bool &hasUpdate,bool &hasStatic,bool&hasDynamic,uint64_t timestamp,ObsvObjects *objects, ObsvObject *object)
    { return ObsvValue( "undefined" );
    } );

    obsvValueGetInitialized = true;
  }

  ObsvValue getObsvValue( const char *name, bool &hasUpdate, bool &hasStatic, bool &hasDynamic, uint64_t timestamp, ObsvObjects *objects=NULL, ObsvObject *object=NULL )
  {
    ObsvValueGet::iterator iter( obsvValueGet.find( name ) );
    if ( iter == obsvValueGet.end() )
    { ObsvValue value( name );
      value.name  = name;
      value.alias = name;
      return value;
    }
    
    ObsvValueGetter &getter( iter->second );
    ObsvValue value( getter.func( getter.alias, hasUpdate, hasStatic, hasDynamic, timestamp, objects, object ) );
    value.name  = getter.name;
    value.alias = getter.alias;
    
    return value;
  }

  ObsvValue getObsvValue( const char *name, uint64_t timestamp, ObsvObjects *objects=NULL, ObsvObject *object=NULL )
  {
    bool hasUpdate  = false;
    bool hasStatic  = false;
    bool hasDynamic = false;
    return getObsvValue( name, hasUpdate, hasStatic, hasDynamic, timestamp, objects, object );
  }

  class SchemeMessage 
  {
    public:
    
    bool forceUpdate;
    std::vector<std::string> condition;
    std::vector<std::string> components;
    std::function<bool(std::string&,std::string&)> condition_operator;

    bool setCondition( std::string &cond )
    {
      std::vector<std::string> comp( split( cond, ' ' ) );
      if ( comp.size() != 3 )
	return false;

      for ( int i = 0; i < comp.size(); ++i )
      { trim( comp[i] );
	if ( comp[i].empty() )
	  return false;
      }

      condition = comp;

      if ( condition[1] == "==" )
	condition_operator = [](std::string &v0,std::string &v1) { return v0 == v1; };
      else if ( condition[1] == "!=" )
	condition_operator = [](std::string &v0,std::string &v1) { return v0 != v1; };
      else if ( condition[1] == "<" )
	condition_operator = [](std::string &v0,std::string &v1) { return std::stod(v0) < std::stod(v1); };
      else if ( condition[1] == "<=" )
	condition_operator = [](std::string &v0,std::string &v1) { return std::stod(v0) <= std::stod(v1); };
      else if ( condition[1] == ">" )
	condition_operator = [](std::string &v0,std::string &v1) { return std::stod(v0) > std::stod(v1); };
      else if ( condition[1] == ">=" )
	condition_operator = [](std::string &v0,std::string &v1) { return std::stod(v0) >= std::stod(v1); };
      else 
	condition_operator = [](std::string &v0,std::string &v1) { return true; };

      return true;
    }

    SchemeMessage( bool forceUpdate=false ) : forceUpdate( forceUpdate ) {}
      SchemeMessage( std::string &condition, std::vector<std::string> &components, bool forceUpdate=false )
	: components ( components ),
	  forceUpdate( forceUpdate )
	{ setCondition( condition ); }
  };

  class Scheme : public std::vector<SchemeMessage>
  {
    public:
    bool forceUpdate;
    Scheme( bool forceUpdate=false ) : forceUpdate( forceUpdate ) {}
  };

  std::map<std::string,Scheme> schemes;
  bool          hasScheme;

public:
  
  enum ObsvType 
  {
    Multi      		= (1<<1),
    File       		= (1<<2),
    PackedFile 		= (1<<3),
    PackedWebSocket  	= (1<<4),
    UDP        		= (1<<5),
    OSC        		= (1<<6),
    MQTT       		= (1<<7),
    WebSocket  		= (1<<8),
    Lua     		= (1<<9),
    InfluxDB   		= (1<<10),
    Bash     		= (1<<11),
    Image      		= (1<<12),
    HeatMap    		= (1<<13),
    FlowMap    		= (1<<14),
    TraceMap   		= (1<<15)
  };

  std::vector<std::string>                       operationalDevices;

  uint64_t				         start_timestamp;
  uint64_t				         stalled_timestamp;
  uint64_t					 timestamp;
  uint64_t 					 frame_id;
  std::string					 name;
  std::string		 			 logFileTemplate;
  std::string		 			 logFileName;
   
  float						 maxFPS;
  float						 validDuration;
  float						 aliveTimeout;
  float						 smoothing;
  int                                            isStarted;
  int                                            startStopStatusChanged;
  int						 type;
  int						 isStalled;
  bool						 isResuming;
  bool						 alwaysOn;
  bool						 continuous;
  bool						 fullFrame;
  bool						 isJson;
  bool						 isThreaded;
  bool						 reporting;
  int						 verbose;
  bool		    				 test;
  bool		    				 useLatent;
  bool		    				 dropPrivate;
  bool		    				 showCountStatus;
  bool		    				 rectCentered;
  bool		    				 rectNormalized;
  bool		    				 showSwitchStatus;
  ObsvRects					 rects;
  Filter::ObsvFilter	   			 obsvFilter;
  float						 reportDistance;
  std::string					 statusMsg;
  std::vector<std::string>			 messages;
  std::string					 runMode;
  ObsvUserData 					*userData;

  TrackableObserver()
  : name          ( "unnamed" ),
    obsvValueGetInitialized( false ),
    hasScheme     ( false ),
    logFileName   (),
    isStarted     ( -1 ),
    startStopStatusChanged( -1 ),
    isStalled     ( 0 ),
    isResuming    ( false ),
    alwaysOn      ( false ),
    isThreaded    ( false ),
    thread        ( NULL  ),
    exitThread    ( false ),
    isFlushed     ( false ),
    maxFPS        ( 0.0f  ),
    validDuration ( 5.0f  ),
    aliveTimeout  ( 1.0f  ),
    smoothing     ( 0.0f  ),
    continuous    ( true  ),
    reporting     ( true  ),
    fullFrame     ( true  ),
    isJson        ( false ),
    verbose       ( false ),
    test          ( false ),
    useLatent     ( false ),
    dropPrivate   ( false ),
    showSwitchStatus( false ),
    showCountStatus( false ),
    rectCentered  ( false ),
    rectNormalized( false ),
    reportDistance( 0.5 ),
    messages	  (),
    runMode	  (),
    userData	  ( NULL )
  {
  }

  virtual ~TrackableObserver()
  {
    alwaysOn = false;
    stop( timestamp, true );
    stopThread();

    if ( userData != NULL )
      delete userData;
  }

  inline ObsvRect &rect()
  { return rects.rect();
  }

  virtual ObsvRect *setRect( float x=-3.0, float y=-3.0, float width=6.0, float height=6.0, ObsvRect::Edge edge=ObsvRect::Edge::EdgeNone, ObsvRect::Shape shape=ObsvRect::Shape::ShapeRect )
  { return rects.set( x, y, width, height, edge, shape );
  }

  virtual ObsvRect *setRect( const char *name, float x=-3.0, float y=-3.0, float width=6.0, float height=6.0, ObsvRect::Edge edge=ObsvRect::Edge::EdgeNone, ObsvRect::Shape shape=ObsvRect::Shape::ShapeRect )
  { return rects.set( name, x, y, width, height, edge, shape );
  }

  virtual ObsvRect *getRect( const char *name )
  { return rects.get( name );
  }

  static inline std::string applyDateToString( const char *string, uint64_t timestamp=0 )
  {
    if ( strchr( string, '\%' ) == NULL )
     return std::string( string );
     
    if ( timestamp == 0 )
      timestamp = getmsec();
  
    time_t t = timestamp / 1000;
    struct tm timeinfo = *localtime( &t );

    const int maxLen = 2000;
    char buffer[maxLen+1];
    strftime( buffer, maxLen, string, &timeinfo );

    return std::string( buffer );
  }

  std::string templateToFileName( uint64_t timestamp=0 )
  {
    std::string dateString( applyDateToString( logFileTemplate.c_str(), timestamp ) );

    return configFileName( dateString.c_str() );
  }

  inline std::string valueAsString( std::string &string, bool &hasUpdate, bool &hasStatic, bool &hasDynamic, uint64_t timestamp, ObsvObjects *objects=NULL, ObsvObject *object=NULL )
  {  
    ObsvValue value( getObsvValue( string.c_str(), hasUpdate, hasStatic, hasDynamic, timestamp, objects, object ) );
    return value.asString();
  }
  

  std::string schemeComponentAsString( std::string &component, bool &hasUpdate, bool &hasStatic, bool &hasDynamic, uint64_t timestamp, ObsvObjects *objects=NULL, ObsvObject *object=NULL )
  {
    std::string result;
    size_t pos = 0;
    size_t start_pos;

    while( (start_pos = component.find( '<', pos )) != std::string::npos )
    {
      if ( start_pos != pos )
	result += component.substr( pos, start_pos-pos);

      size_t end_pos = component.find( '>', start_pos+1 );

      if ( end_pos != std::string::npos )
      { std::string key( component.substr(start_pos+1,end_pos-1-start_pos) );
	result += valueAsString( key, hasUpdate, hasStatic, hasDynamic, timestamp, objects, object );
      }
      pos = end_pos+1;
    }

    if ( pos < component.length() )
      result += component.substr( pos, component.length()-pos );

    return result;
  }

  virtual bool schemeCondition( SchemeMessage &schemeMessage, uint64_t timestamp, ObsvObjects *objects=NULL, ObsvObject *object=NULL )
  {
    std::vector<std::string> &condition( schemeMessage.condition );
    if ( condition.size() != 3 )
      return true;

    bool hasUpdate  = false;
    bool hasStatic  = false;
    bool hasDynamic = false;
    std::string v0( schemeComponentAsString( condition[0], hasUpdate, hasStatic, hasDynamic, timestamp, objects, object ) );

    if( !(hasUpdate || (hasStatic&&!hasDynamic)))
      return false;

    std::string v1( schemeComponentAsString( condition[2], hasUpdate, hasStatic, hasDynamic, timestamp, objects, object ) );

    return (hasUpdate || (hasStatic&&!hasDynamic)) && schemeMessage.condition_operator( v0, v1 );
  }

  virtual bool setScheme( std::string scheme, bool fromFile=false )
  {
    if ( fromFile )
    { 
      scheme = configFileName( scheme.c_str() );
      if ( !fileExists( scheme.c_str() ) )
      { error( "TrackableObserver::setScheme() ERROR: file %s does not exist", scheme.c_str() );
	return false;
      }

      std::string fileName( scheme );
      std::getline(std::ifstream(fileName.c_str()), scheme, '\0');
    }

    std::vector<std::string> lines( split( scheme, '\n' ) );
    if ( lines.size() == 0 )
      return false;

    scheme.clear();

    for ( int i = 0; i < lines.size(); ++i )
    {
      trim( lines[i] );
      std::string &line( lines[i] );
      std::string name;
      std::string condition;
 
      size_t start_pos = 0;
      if( (start_pos = line.find( '(', start_pos )) != std::string::npos )
      {
	size_t end_pos = line.find( ')', start_pos+1 );

	if ( end_pos != std::string::npos )
        { 
	  name = line.substr( start_pos+1,end_pos-1-start_pos );
	  trim( name ); 

	  line = line.substr( end_pos+1, line.length()-end_pos );
	  trim( line );

	  size_t end_pos = name.find( '?', 0 );
	  if ( end_pos != std::string::npos )
          { 
	    condition = name.substr( end_pos+1,name.length()-end_pos );
	    trim( condition );

	    name = name.substr( 0, end_pos );
	    trim( name );
	  }
	}
      }

      std::vector<std::string> components( split( line, ' ' ) );

      for ( int j = ((int)components.size())-1; j >= 0; --j )
      { trim( components[j] );
	if ( components[j].empty() )
	  components.erase( components.begin()+j );
      }

      if ( components.size() > 0 )
      { if ( schemes.count(name) == 0 )
	{ schemes.emplace( name, Scheme() );
	  if ( startsWith("start",name) || startsWith("stop",name) )
	    schemes[name].forceUpdate = true;
	  if ( verbose )
	  {
	    info( "added observer %s scheme: %s (c:%s)", this->name.c_str(), name.c_str(), condition.c_str() );

	    std::string msg;
	    for ( int j = 0; j < components.size(); ++j )
	    { msg += " ";
	      msg += components[j];
	    }
	    info( msg.c_str() );
	  }
	}
	schemes[name].emplace_back( condition, components, schemes[name].forceUpdate );
      }
    }

    hasScheme = (schemes.size() != 0);

    return hasScheme;
  }
  
  virtual void setParam( KeyValueMap &descr )
  {
    std::string filter;
    if ( descr.get( "filter", filter )  )
      obsvFilter.parseFilter( filter.c_str() );
/*
    std::string fileName;
    if ( descr.get( "file", fileName ) )
      setFileName( fileName.c_str() );
*/
    descr.get( "logDistance",      reportDistance );
    descr.get( "reportDistance",   reportDistance );
    descr.get( "verbose",          verbose    );
    descr.get( "test",     	   test );
    descr.get( "useLatent",        useLatent  );
    descr.get( "regionCentered",   rectCentered );
    descr.get( "regionNormalized", rectNormalized );
    descr.get( "reporting",        reporting  );
    descr.get( "streamData",       reporting  );
    descr.get( "continuous",       continuous );
    descr.get( "alwaysOn",    	   alwaysOn );
    descr.get( "fullFrame",        fullFrame  );
    descr.get( "maxFPS",           maxFPS     );
    descr.get( "validDuration",    validDuration );
    descr.get( "aliveTimeout",     aliveTimeout );
    descr.get( "smoothing",        smoothing   );
    descr.get( "isThreaded",       isThreaded  );
    descr.get( "showSwitchStatus", showSwitchStatus  );
    descr.get( "showCountStatus",  showCountStatus  );
    descr.get( "runMode",          runMode  );
    
    std::string scheme;
    if ( descr.get( "scheme", scheme ) )
      setScheme( scheme, false );

    std::string schemeFile;
    if ( descr.get( "schemeFile", schemeFile ) )
      setScheme( schemeFile, true );

    std::string operationalDevs;
    if ( descr.get( "operationalDevices", operationalDevs ) )
      operationalDevices = split( operationalDevs, ',' );
  }
  
  static std::string configFileName( const char *fileName );
  static void (*error)		( const char *format, ... );
  static void (*warning)	( const char *format, ... );
  static void (*log)  		( const char *format, ... );
  static void (*info)  		( const char *format, ... );
  static void (*notification)  	( const char *tags, const char *format, ... );

  std::string replaceTemplates( const char *fileName )
  {
    std::string result( fileName );

    replace( result, "%monthly",  "%Y-%m" );
    replace( result, "%weekly",   "%Y-%V" );
    replace( result, "%daily",    "%Y-%m-%d" );
    replace( result, "%hourly",   "%Y-%m-%d-%H:00" );
    replace( result, "%minutely", "%Y-%m-%d-%H:%M" );

    result = configFileName( result.c_str() );

    return result;
  }

  virtual void setFileName( const char *fileName )
  {
    logFileTemplate = replaceTemplates( fileName );
    logFileName     = templateToFileName();
  }

  virtual bool isValidDuration( int64_t duration ) const
  {
    return duration > 0 && duration < validDuration*1000;
  }

  virtual bool isValidSpeed( int64_t duration, float distance ) const
  {
    return isValidDuration(duration) && distance / (duration/1000.0) < 2.0; // speed limit is 2.0m/s
  }

  virtual bool observe( const ObsvObjects &other, bool force=false )
  {
    if ( isStarted != 1 )
      return false;

    int64_t time_diff = other.timestamp - timestamp;
    
    if ( !force && maxFPS > 0.0 && time_diff > 0 && 1000.0/time_diff > maxFPS )
      return false;
    
    timestamp 	   = other.timestamp;
    frame_id       = other.frame_id;
    
    for ( int i = rects.numRects()-1; i >= 0; --i )
    {
      ObsvRect &rect( rects.rect(i) );
      ObsvObjects &objects( rect.objects );

      objects.rect = &rect;
      if ( rectCentered || rectNormalized )
      {
	if ( rectNormalized )
	{ objects.centerX = rect.x;
	  objects.centerY = rect.y;
	  objects.scaleX  = 1.0 / rect.width;
	  objects.scaleY  = 1.0 / rect.height;
	}
	else
        {
	  objects.centerX = rect.x + rect.width  / 2;
	  objects.centerY = rect.y + rect.height / 2;
	}
      }

      objects.timestamp       = other.timestamp;
      objects.alive           = ((objects.timestamp - objects.alive_timestamp)/1000.0 > aliveTimeout);
      objects.frame_id        = other.frame_id;
      objects.lastCount       = objects.validCount;
      objects.lastEnterCount  = objects.enterCount;
      objects.lastLeaveCount  = objects.leaveCount;
      objects.lastGateCount   = objects.gateCount;
      objects.lastAvgLifespan = objects.avgLifespan;

      if ( objects.validCount == 0 )
	objects.switch_timestamp = 0;

      if ( objects.alive )
	objects.alive_timestamp = objects.timestamp;

      for ( auto &iter: objects )
	iter.second.status = ObsvObject::Invalid;

      for ( auto &iter: other )
      { const ObsvObject &object( iter.second );

	if ( useLatent || !object.isLatent() )
	{
	  if ( rects.contains( i, object.x, object.y, 0.0 ) )
          { ObsvObject *obj = objects.get( object.id );

	    if ( obj == NULL )
	    { auto pair( objects.emplace(std::make_pair(object.id,object)) );
	      obj = &pair.first->second;
	      obj->objects 	     = &objects;
	      obj->status            = ObsvObject::Enter;
	      obj->timestamp_enter   = obj->timestamp;
	      obj->timestamp_touched = obj->timestamp;
	      obj->edge              = rects.edgeCrossed( i, object, ObsvObject::Enter );
	      objects.enterCount    += rects.countEdge  ( i, (ObsvRect::Edge) obj->edge );
	      objects.gateCount      =  objects.enterCount - objects.leaveCount;
	      if ( objects.gateCount < 0 )
		objects.gateCount = 0;
	    
	      obj->track( object );
	      obj->moveDone();
	      obj->update();
	    }
	    else
            { obj->track( object, smoothing );
	      obj->d	= obj->distanceMoved();
	      obj->status = ObsvObject::Move;
	      obj->edge   = ObsvRect::Edge::EdgeNone;
	    }
	    
	    obj->flags   = object.flags;
	    if ( object.isTouched() )
	      obj->timestamp_touched = other.timestamp;
	  }
	}
      }
    
      int invalidCount = 0;
      
      for ( auto &iter: objects )
	if ( iter.second.status == ObsvObject::Invalid )
        { 
	  ObsvObject &object( iter.second );
	  object.moveDone();
	  object.status = ObsvObject::Leave;

	  const ObsvObject *obj = other.get( object.id );

	  if ( obj != NULL )
	    object.edge = rects.edgeCrossed( i, *obj, ObsvObject::Leave );
	  else
	    object.edge = rects.edgeCrossed( i, object, ObsvObject::Leave );

	  objects.leaveCount += rects.countEdge( i, (ObsvRect::Edge) object.edge );

	  objects.gateCount   =  objects.enterCount - objects.leaveCount;
	  if ( objects.gateCount < 0 )
	    objects.gateCount = 0;

	  uint64_t lifespan      = object.timestamp_touched - object.timestamp_enter;
	  objects.lifespanSum   += lifespan;
	  objects.lifespanCount += 1;
	  objects.avgLifespan    = objects.lifespanSum / objects.lifespanCount;

	  invalidCount += 1;
	}

      objects.validCount = objects.size() - invalidCount;

      if ( objects.validCount > 0 && objects.lastCount <= 0 )
	objects.switch_timestamp = objects.timestamp;
      else if ( objects.validCount == 0 && objects.lastCount > 0 && objects.switch_timestamp > 0 )
	objects.switchDurationSum += objects.timestamp - objects.switch_timestamp;
    }

    if ( reporting )
      report();

    for ( int i = rects.numRects()-1; i >= 0; --i )
    {
      ObsvObjects &objects( rects.rect(i).objects );
    
      for ( auto iter = objects.begin(); iter != objects.end(); )
      { 
	if (iter->second.status == ObsvObject::Leave )
	  iter = objects.erase(iter);
	else 
	  ++iter;
      }
    }
    
    isResuming = false;

    return true;
  }

  virtual void reportScheme( std::vector<SchemeMessage> &scheme, uint64_t timestamp, ObsvObjects *objects=NULL, ObsvObject *object=NULL )
  {
    for ( int i = 0; i < scheme.size(); ++i )
    {
      if ( schemeCondition( scheme[i], timestamp, objects, object ) )
      {
	std::string msg;
	bool hasUpdate  = false;
	bool hasStatic  = false;
	bool hasDynamic = false;

	std::vector<std::string> &components( scheme[i].components );
	for ( int c = 0; c < components.size(); ++c )
        { 
	  std::string result( schemeComponentAsString( components[c], hasUpdate, hasStatic, hasDynamic, timestamp, objects, object ) );

	  //	  printf( "has(%s) %d\n", components[c].c_str(), hasUpdate );
	  if ( !result.empty() )
	  { if ( !msg.empty() )
	      msg += " ";
	    msg += result;
	  }
	}

	if ( hasUpdate || (hasStatic&&!hasDynamic) || scheme[i].forceUpdate )
	{
	  if ( thread != NULL )
          { lock();
	    messages.push_back( msg );
	    unlock();
	  }
	  else
          { messages.push_back( msg );
	    write( messages );
	    messages.clear();
	    isFlushed = true;
	  }
	}
      }
    }
  }

  void reportScheme( std::vector<SchemeMessage> &scheme, uint64_t timestamp, ObsvRects &rects )
  {
    ObsvRect &rect( rects.rect(0) );
    rect.objects.rect = &rect;
    reportScheme( scheme, timestamp, &rect.objects );
  }

  void reportSchemes()
  {
    for ( int i = rects.numRects()-1; i >= 0; --i )
    {
      ObsvObjects &objects( rects.rect(i).objects );
	
      if ( schemes.count("frame_begin") > 0 )
	reportScheme( schemes["frame_begin"], objects.timestamp, &objects );

      if ( schemes.count("objects_begin") > 0 )
      { std::vector<SchemeMessage> &scheme( schemes["objects_begin"] );
	for ( auto &iter: objects )
	  reportScheme( scheme, objects.timestamp, &objects, &iter.second  );
      }

      std::vector<SchemeMessage> *objectScheme = (schemes.count("object") > 0 ? &schemes["object"] : NULL);

      for ( auto &iter: objects )
      { ObsvObject &object( iter.second );

	bool reportMove  = ((object.status == ObsvObject::Move)  &&  (continuous || object.d >= reportDistance));
	if ( objectScheme != NULL )
	  reportScheme( *objectScheme, objects.timestamp, &objects, &object );
	
	if ( reportMove )
	  object.moveDone();
      }
	
      if ( schemes.count("objects_end") > 0 )
      { std::vector<SchemeMessage> &scheme( schemes["objects_end"] );
	for ( auto &iter: objects )
	  reportScheme( scheme, objects.timestamp, &objects, &iter.second  );
      }

      if ( schemes.count("frame_end") > 0 )
	reportScheme( schemes["frame_end"], objects.timestamp, &objects );
    }    
  }

  virtual void report()
  {
    if ( hasScheme )
      reportSchemes();
    else if ( isJson )
      reportJsonMessages();
  }

  virtual void writeJsonMsg( std::string msg, uint64_t timestamp )
  {
    std::string message( "{" );
    
    if ( obsvFilter.filterEnabled( Filter::TIMESTAMP ) )
    {
      if ( timestamp == (uint64_t)0 )
	timestamp = this->timestamp;
      
      std::string kmcTimestamp( obsvFilter.kmc(Filter::ObsvTimestamp) );
      const char *templ = NULL;

      char *column = (char *) strchr( kmcTimestamp.c_str(), '@' );
      if ( column != NULL )
      { templ = &column[1];
	column[0] = '\0';
      }
      message += "\"";
      message += kmcTimestamp.c_str();
      message += "\":";
      message += timestampString( templ, timestamp );
      if ( !msg.empty() )
	message += ",";
    }
    
    message += msg;
    message += "}";
    
    if ( thread != NULL )
    {
      lock();
      messages.push_back( message );
      unlock();
    }
    else
    { messages.push_back( message );
      write( messages );
      messages.clear();
      isFlushed = true;
    }
  }

  inline bool hasReportObjects() const
  { 
    return obsvFilter.filterEnabled( Filter::OBSV_MOVE  )      ||
           obsvFilter.filterEnabled( Filter::OBSV_ENTER )      ||
           obsvFilter.filterEnabled( Filter::OBSV_LEAVE )      || 
           obsvFilter.filterEnabled( Filter::OBSV_ENTEREDGE )  ||
           obsvFilter.filterEnabled( Filter::OBSV_LEAVEEDGE )  || 
           obsvFilter.filterEnabled( Filter::OBSV_OBJECTS )    ||
           obsvFilter.filterEnabled( Filter::OBSV_OBJECT );
  }
  

  inline void checkJsonEmpty( std::string &msg )
  { if ( !msg.empty() && msg.back() != '{' )
      msg += ",";
  }

  inline void setJsonInt( std::string &msg, std::string key, int value )
  { checkJsonEmpty( msg );
    msg += "\"" + key + "\":" + std::to_string(value);
  }

  inline void setJsonFloat( std::string &msg, std::string key, float value )
  { checkJsonEmpty( msg );
    char s[20];
    sprintf( s, "%.3f", value );
    msg += "\"" + key + "\":" + s;
  }

  inline void setJsonString( std::string &msg, std::string key, std::string value )
  { checkJsonEmpty( msg );
    msg += "\"" + key + "\":\"" + value + "\"";
  }
  
  virtual void bracket( std::string &msg )
  {
    if ( !msg.empty() )
    { std::string m( "{" );
      m +=  msg;
      m += "}";
      msg = m;
    }
  }

  virtual std::string reportJsonCountMessages( ObsvObjects &objects )
  {
    std::string msg;

    if ( obsvFilter.filterEnabled( Filter::OBSV_COUNT ) )
    { if ( continuous || objects.lastCount != (int)objects.validCount )
	setJsonInt( msg, obsvFilter.kmc(Filter::ObsvCount), (int)objects.validCount );
    }
    if ( obsvFilter.filterEnabled( Filter::OBSV_SWITCH ) )
    { if ( continuous || ((bool)objects.lastCount) != (bool)objects.validCount )
	setJsonInt( msg, obsvFilter.kmc(Filter::ObsvSwitch), (int)(bool)objects.validCount );
    }
    if ( obsvFilter.filterEnabled( Filter::OBSV_SWITCH_DURATION ) )
    {
      if ( objects.lastCount > 0 && objects.switch_timestamp != 0 && (objects.validCount == 0 || continuous) )
	setJsonInt( msg, obsvFilter.kmc(Filter::ObsvSwitchDuration), (int)(objects.timestamp-objects.switch_timestamp) );
      else if ( continuous )
	setJsonInt( msg, obsvFilter.kmc(Filter::ObsvSwitchDuration), (int)0 );
    }
    if ( obsvFilter.filterEnabled( Filter::OBSV_ALIVE ) )
    { if ( objects.alive )
	setJsonInt( msg, obsvFilter.kmc(Filter::ObsvAlive), (int)(bool)objects.alive );
    }
    if ( obsvFilter.filterEnabled( Filter::OBSV_OPERATIONAL ) )
    { if ( objects.alive )
	setJsonFloat( msg, obsvFilter.kmc(Filter::ObsvOperational), (int)(bool)objects.operational );
    }
    if ( obsvFilter.filterEnabled( Filter::OBSV_ENTERCOUNT ) )
    { if ( continuous || objects.lastEnterCount != (int)objects.enterCount )
	setJsonInt( msg, obsvFilter.kmc(Filter::ObsvEnterCount), (int)objects.enterCount );
    }
    if ( obsvFilter.filterEnabled( Filter::OBSV_LEAVECOUNT ) )
    { if ( continuous || objects.lastLeaveCount != (int)objects.leaveCount )
	setJsonInt( msg, obsvFilter.kmc(Filter::ObsvLeaveCount), (int)objects.leaveCount );
    }
    if ( obsvFilter.filterEnabled( Filter::OBSV_GATECOUNT ) )
    { if ( continuous || objects.lastGateCount != (int)objects.gateCount )
	setJsonInt( msg, obsvFilter.kmc(Filter::ObsvGateCount), (int)objects.gateCount );
    }
    if ( obsvFilter.filterEnabled( Filter::OBSV_AVGLIFESPAN ) )
    { if ( continuous || objects.lastAvgLifespan != (int)objects.avgLifespan )
	setJsonInt( msg, obsvFilter.kmc(Filter::ObsvAvgLifeSpan), (int)objects.avgLifespan );
    }

    return msg;
  }
  
  virtual std::string reportJsonStartMessage()
  {
    std::string msg;
    
    if ( obsvFilter.filterEnabled( Filter::OBSV_START ) )
    { setJsonString( msg, obsvFilter.kmc(Filter::ObsvAction), obsvFilter.kmc(Filter::ObsvStart) );
      if ( obsvFilter.filterEnabled( Filter::OBSV_RUNMODE ) )
	setJsonString( msg, obsvFilter.kmc(Filter::ObsvRunMode), runMode );
    }
    
    return msg;
  }
  
  virtual std::string reportJsonStopMessage()
  {
    std::string msg;
    
    if ( obsvFilter.filterEnabled( Filter::OBSV_STOP ) )
    { setJsonString( msg, obsvFilter.kmc(Filter::ObsvAction), obsvFilter.kmc(Filter::ObsvStop) );
      if ( obsvFilter.filterEnabled( Filter::OBSV_RUNMODE ) )
	setJsonString( msg, obsvFilter.kmc(Filter::ObsvRunMode), runMode );

      if ( obsvFilter.filterEnabled( Filter::OBSV_STATISTICS ) )
      {
	bool reportRegions = (rects.numRects() > 1);
	msg += ",";
      
	if ( reportRegions )
        { msg += obsvFilter.kmc(Filter::ObsvRegions);
	  msg += ":[";
	}
	for ( int i = 0; i < rects.numRects(); ++i )
        {
	  ObsvObjects &objects( rects.rect(i).objects );
	  std::string m;

	  if ( reportRegions )
	  { 
	    if ( i > 0 )
	      msg += ",";
	      
	    msg += "{";
	    setJsonString( m, obsvFilter.kmc(Filter::ObsvRegion), objects.region );
	  }

	  reportJsonStatistics( objects, m );
	  msg += m;
	  if ( reportRegions )
	    msg += "}";
	}
	if ( reportRegions )
	  msg += "]";
      }
    }

    return msg;
  }

  virtual void reportJsonStatistics( ObsvObjects &objects, std::string &msg )
  {
    uint64_t duration = objects.timestamp - start_timestamp;
    setJsonInt  ( msg, "duration",    (int)duration );
    setJsonInt  ( msg, "countSum",    (int)objects.lifespanCount );
    setJsonInt  ( msg, "avgLifespan", (int)objects.avgLifespan );
    setJsonInt  ( msg, "switchDurationSum", (int)objects.switchDurationSum );
    setJsonFloat( msg, "switchFraction", objects.switchDurationSum / (double)duration );
  }

  virtual bool hasMovedObject( ObsvObjects &objects )
  {
    if ( !obsvFilter.filterEnabled( Filter::OBSV_MOVE  ) )
      return false;
    
    if ( objects.lastCount != (int)objects.validCount )
      return true;
    
    for ( auto &iter: objects )
    { ObsvObject &object( iter.second );
      bool moved = ((object.status == ObsvObject::Move) && (continuous || object.d >= reportDistance));
      if ( moved )
	return true;
    }
    
    return false;
  }
  
  virtual std::string reportJsonMessage( ObsvObjects &objects, ObsvObject &object )
  {
    std::string msg;

    bool enterEnabled      = obsvFilter.filterEnabled( Filter::OBSV_ENTER );
    bool moveEnabled       = obsvFilter.filterEnabled( Filter::OBSV_MOVE  );
    bool leaveEnabled      = obsvFilter.filterEnabled( Filter::OBSV_LEAVE );
    bool enterEdgeEnabled  = obsvFilter.filterEnabled( Filter::OBSV_ENTEREDGE );
    bool leaveEdgeEnabled  = obsvFilter.filterEnabled( Filter::OBSV_LEAVEEDGE );

    bool anyEnabled = (enterEnabled || moveEnabled || leaveEnabled || enterEdgeEnabled || leaveEdgeEnabled);
    
    if ( !anyEnabled && (obsvFilter.filterEnabled( Filter::OBSV_OBJECTS ) || obsvFilter.filterEnabled( Filter::OBSV_OBJECT )) )
      moveEnabled = true;

    anyEnabled |= moveEnabled;
    
    bool reportMove  	  = ((object.status == ObsvObject::Move)  &&  moveEnabled  && (continuous || fullFrame || object.d >= reportDistance));
    bool reportEnter 	  = ((object.status == ObsvObject::Enter) &&  enterEnabled);
    bool reportLeave 	  = ((object.status == ObsvObject::Leave) && (leaveEnabled || obsvFilter.filterEnabled( Filter::OBSV_LIFESPAN )));
    bool reportEnterEdge  = ((object.status == ObsvObject::Enter) &&  enterEdgeEnabled  && object.edge != ObsvRect::Edge::EdgeNone);
    bool reportLeaveEdge  = ((object.status == ObsvObject::Leave) &&  leaveEdgeEnabled  && object.edge != ObsvRect::Edge::EdgeNone);

    bool reportAny = (reportEnter || reportMove || reportLeave || reportEnterEdge || reportLeaveEdge);
    
    if ( !reportAny )
      return msg;
    
    object.moveDone();

    if ( obsvFilter.filterEnabled( Filter::OBSV_TYPE ) )
    { if ( reportEnter )
	setJsonString( msg, obsvFilter.kmc(Filter::ObsvType), obsvFilter.kmc(Filter::ObsvEnter) );
      if ( reportMove  )
	setJsonString( msg, obsvFilter.kmc(Filter::ObsvType), obsvFilter.kmc(Filter::ObsvMove) );
      if ( reportLeave )
	setJsonString( msg, obsvFilter.kmc(Filter::ObsvType), obsvFilter.kmc(Filter::ObsvLeave) );
    }

    if ( reportEnterEdge )
      setJsonString( msg, obsvFilter.kmc(Filter::ObsvEnterEdge), object.edgeAsString() );
    if ( reportLeaveEdge )
      setJsonString( msg, obsvFilter.kmc(Filter::ObsvLeaveEdge), object.edgeAsString() );

    if ( !fullFrame )
    { if ( obsvFilter.filterEnabled( Filter::FRAME_ID ) )
	setJsonInt( msg, obsvFilter.kmc(Filter::FrameId), frame_id );
      if ( (obsvFilter.filterEnabled( Filter::OBSV_REGIONS ) || obsvFilter.filterEnabled( Filter::OBSV_REGION )) && !objects.region.empty() )
	setJsonString( msg, obsvFilter.kmc(Filter::ObsvRegion), objects.region );
    }
    
    if ( reportLeave && obsvFilter.filterEnabled( Filter::OBSV_LIFESPAN ) )
      setJsonInt( msg, obsvFilter.kmc(Filter::ObsvLifeSpan), object.timestamp_touched - object.timestamp_enter );

    if ( obsvFilter.filterEnabled( Filter::OBSV_ID ) )
      setJsonInt( msg, obsvFilter.kmc(Filter::ObsvId), object.id );

    if ( obsvFilter.filterEnabled( Filter::OBSV_UUID ) )
      setJsonString( msg, obsvFilter.kmc(Filter::ObsvUUID), object.uuid.str() );

    if ( obsvFilter.filterEnabled( Filter::OBSV_X ) )
      setJsonFloat( msg, obsvFilter.kmc(Filter::ObsvX), (object.x-objects.centerX) * objects.scaleX );
    if ( obsvFilter.filterEnabled( Filter::OBSV_Y ) )
      setJsonFloat( msg, obsvFilter.kmc(Filter::ObsvY), (object.y-objects.centerY) * objects.scaleY );
    if ( obsvFilter.filterEnabled( Filter::OBSV_Z ) && !isnan(object.z) )
      setJsonFloat( msg, obsvFilter.kmc(Filter::ObsvZ), (object.z-objects.centerZ) * objects.scaleZ );

    if ( obsvFilter.filterEnabled( Filter::OBSV_SIZE ) && !isnan(object.size) )
      setJsonFloat( msg, obsvFilter.kmc(Filter::ObsvSize), object.size );

    if ( !msg.empty() && obsvFilter.filterEnabled( Filter::OBSV_OBJECT ) )
    { std::string m;
      m +=  "\"" + std::string(obsvFilter.kmc(Filter::ObsvObject)) + "\":{";
      m +=  msg;
      m += "}";
      msg = m;
    }
    
    return msg;
  }

  virtual std::string reportJsonMessageObjects( ObsvObjects &objects )
  {
    bool reportRegions = (rects.numRects() > 1 || obsvFilter.filterEnabled( Filter::OBSV_REGIONS ) || obsvFilter.filterEnabled( Filter::OBSV_REGION ));
      
    std::string msg, objectsMsg;
	
    if ( hasReportObjects() )
    {
      for ( auto &iter: objects )
      { std::string m( reportJsonMessage( objects, iter.second ) );
	if ( !m.empty() )
	{ bracket( m );
	  checkJsonEmpty( objectsMsg );
	  objectsMsg += m;
	}
      }
    }

    if ( !objectsMsg.empty() || fullFrame )
    { 
      std::string m( "\"" );
      m += obsvFilter.kmc(Filter::ObsvObjects);
      m += "\":[";
      m +=  objectsMsg;
      m += "]";
      objectsMsg = m;
    }

    std::string countMsg( reportJsonCountMessages( objects ) );

    if ( objectsMsg.empty() && countMsg.empty() )
      return msg;

    if ( !countMsg.empty() )
      msg = countMsg;

    if ( !objectsMsg.empty() )
    { checkJsonEmpty( msg );
      msg += objectsMsg;
    }

    if ( rects.numRects() > 1 || obsvFilter.filterEnabled( Filter::OBSV_REGIONS ) )
    { 
      std::string m( "{" );
      setJsonString( m, obsvFilter.kmc(Filter::ObsvRegion), objects.region );
      checkJsonEmpty( m );
      m += msg;
      msg = m + "}";
    }
    else if ( obsvFilter.filterEnabled( Filter::OBSV_REGION ) )
    {
      std::string m;
      setJsonString( m, obsvFilter.kmc(Filter::ObsvRegion), objects.region );
      checkJsonEmpty( m );
      msg = m + msg;
    }
    
    return msg;
  }
    
  virtual void reportJsonMessagesFullFrame()
  {
    std::string regionsMsg;

    for ( int i = rects.numRects()-1; i >= 0; --i )
    {
      ObsvObjects &objects( rects.rect(i).objects );

      if ( continuous || hasMovedObject( objects ) )
      {
	std::string objectsMsg( reportJsonMessageObjects( objects ) );
	if ( !objectsMsg.empty() )
	{ checkJsonEmpty( regionsMsg );
	  regionsMsg += objectsMsg;
	}
      }
    }
    
    if ( !regionsMsg.empty() )
    {
      if ( rects.numRects() > 1 || obsvFilter.filterEnabled( Filter::OBSV_REGIONS ) )
      { 
	std::string m( "\"" );
	m += obsvFilter.kmc(Filter::ObsvRegions);
	m += "\":[";
	m +=  regionsMsg;
	m += "]";
	regionsMsg = m;
      }
    }

    std::string msg;

    if ( (continuous || !regionsMsg.empty()) && obsvFilter.filterEnabled( Filter::FRAME_ID ) )
      setJsonInt( msg, obsvFilter.kmc(Filter::FrameId), frame_id );
    
    if ( !regionsMsg.empty() )
    { checkJsonEmpty( msg );
      msg += regionsMsg;
    }

    if ( continuous || !msg.empty() )
      writeJsonMsg( msg, timestamp );
  }

  virtual void reportJsonMessages()
  {
    if ( fullFrame )
      reportJsonMessagesFullFrame();
    else
    {
      bool reportObjects = hasReportObjects();
      bool reportRegions = (obsvFilter.filterEnabled( Filter::OBSV_REGIONS ) || obsvFilter.filterEnabled( Filter::OBSV_REGION ));

      for ( int i = rects.numRects()-1; i >= 0; --i )
      {
	ObsvObjects &objects( rects.rect(i).objects );

	std::string msg( reportJsonCountMessages( objects ) );
	if ( !msg.empty() )
        { if ( obsvFilter.filterEnabled( Filter::FRAME_ID ) )
	    setJsonInt( msg, obsvFilter.kmc(Filter::FrameId), frame_id );
	  if ( reportRegions && !objects.region.empty() )
	    setJsonString( msg, obsvFilter.kmc(Filter::ObsvRegion), objects.region );
	  writeJsonMsg( msg, objects.timestamp );
	}

	if ( reportObjects )
        { for ( auto &iter: objects )
	  { std::string msg( reportJsonMessage( objects, iter.second ) );
	    if ( !msg.empty() )
	      writeJsonMsg( msg, iter.second.timestamp );
	  }
	}
      }
    }
  }
  
  virtual void write( std::vector<std::string> &messages, uint64_t timestamp=0 )
  {
  }

  virtual bool stall( uint64_t timestamp=0 )
  {
    if ( isStalled == 1 )
      return false;

    isStalled = 1;
    isResuming = false;
    
    stalled_timestamp = timestamp;

    return true;
  }
  
  virtual bool resume( uint64_t timestamp=0 )
  {
    if ( isStalled == 0 )
      return false;

    isStalled  = 0;
    isResuming = true;
   
    rects.start();

    return true;
  }
  
  virtual bool start( uint64_t timestamp=0, bool startRects=false )
  {
    if ( isStarted == 1 )
      return false;

    if ( !obsvValueGetInitialized )
      initObsvValueGet();

    isStalled  = 0;
    isResuming = false;

    isStarted  = 1;
 
    if ( startRects )
      rects.start();

    if ( !reporting )
      return true;

    if ( timestamp == 0 )
      timestamp = getmsec();
    this->timestamp = timestamp;
    start_timestamp = timestamp;

    if ( hasScheme )
    {
      if ( schemes.count("start") > 0 )
      { startStopStatusChanged = 1;
	reportScheme( schemes["start"], timestamp, rects );
 	startStopStatusChanged = -1;
      }
    }
    else if ( isJson )
    {
      std::string msg( reportJsonStartMessage() );
      if ( !msg.empty() )
      { if ( timestamp == 0 )
	  timestamp = getmsec();
	writeJsonMsg( msg, timestamp );
      }
    }
    
    startThread();
    
    return true;
  }
  
  virtual bool stop( uint64_t timestamp=0, bool stopRects=false )
  {
    if ( isStarted == 0 || alwaysOn )
      return false;

    isStalled  = 0;
    isResuming = false;

    isStarted  = 0;

    if ( stopRects )
      rects.stop();

    if ( !reporting )
      return true;

    if ( timestamp == 0 )
      timestamp = getmsec();
    this->timestamp = timestamp;

    if ( hasScheme )
    {
      if ( schemes.count("stop") > 0 )
      { startStopStatusChanged = 0;
	reportScheme( schemes["stop"], timestamp, rects );
	startStopStatusChanged = -1;
      }
    }
    else if ( isJson )
    { std::string msg( reportJsonStopMessage() );
      if ( !msg.empty() )
      { if ( timestamp == 0 )
	  timestamp = getmsec();
	writeJsonMsg( msg, timestamp );
      }
    }
    
    return true;
  }

  virtual void reset( uint64_t timestamp=0 )
  {
    int started = isStarted;

    if ( started == 1 )
    {
//      const ObsvObjects objects;
//      observe( objects );

      isStarted = -1;
      stop( timestamp );
    }

    rects.reset();

    if ( started == 1 )
    { isStarted = -1;
      start( timestamp );
    }
  }
  
};

/***************************************************************************
*** 
*** TrackableObserverCreator
***
****************************************************************************/

typedef TrackableObserver *(*TrackableObserverCreator)( ... );

/***************************************************************************
*** 
*** TrackableMultiObserver
***
****************************************************************************/

class TrackableMultiObserver : public TrackableObserver
{
public:
  std::vector<TrackableObserver*>	observer;

  TrackableMultiObserver()
  : TrackableObserver(),
    observer	     ()
  { type = Multi;
    name = "multi";
  }

  void	addObserver( TrackableObserver *observer )
  { this->observer.push_back( observer );
  }

  void removeObserver( int i, bool deleteIt=false )
  { if ( deleteIt )
      delete observer[i];
    observer.erase( observer.begin()+i );
  }
    
  bool	observe( const ObsvObjects &other, bool force=false )
  {
    for ( int i = 0; i < observer.size(); ++i )
      if ( observer[i]->isStarted == 1 || observer[i]->alwaysOn )
	observer[i]->observe( other, force );

    return true;
  }

  virtual bool stall( uint64_t timestamp=0 )
  {
    if ( !TrackableObserver::stall(timestamp) )
      return false;

    for ( int i = 0; i < observer.size(); ++i )
      observer[i]->stall( timestamp );

    return true;
  }
  
  virtual bool resume( uint64_t timestamp=0 )
  { 
    if ( !TrackableObserver::resume(timestamp) )
      return false;
    
    for ( int i = 0; i < observer.size(); ++i )
      observer[i]->resume( timestamp );

    return true;
  }

  virtual bool start( uint64_t timestamp=0, bool startRects=false )
  {
    if ( !TrackableObserver::start(timestamp,startRects) )
      return false;

    for ( int i = 0; i < observer.size(); ++i )
      observer[i]->start( timestamp, startRects );

    return true;
  }
  
  virtual void startAlwaysObserver( uint64_t timestamp=0, bool startRects=false )
  {
    for ( int i = 0; i < observer.size(); ++i )
      if ( observer[i]->alwaysOn )
	observer[i]->start( timestamp, startRects );
  }

  virtual bool stop( uint64_t timestamp=0, bool stopRects=false )
  { 
    if ( !TrackableObserver::stop(timestamp,stopRects) )
      return false;
    
    for ( int i = 0; i < observer.size(); ++i )
      observer[i]->stop( timestamp, stopRects );

    return true;
  }

  virtual void	reset( uint64_t timestamp=0 )
  {
    for ( int i = 0; i < observer.size(); ++i )
      observer[i]->reset( timestamp );
  }
};

/***************************************************************************
*** 
*** TrackableBashObserver
***
****************************************************************************/

class TrackableBashObserver : public TrackableObserver
{
public:
  std::string	scriptParameter;
  bool		isCount, isSwitch;
  
 
  TrackableBashObserver()
  : TrackableObserver(),
    isCount ( false ),
    isSwitch( false )
  {
    type = Bash;
    name = "bash";

    obsvFilter.parseFilter( "count" );
  }
  
  virtual void setParam( KeyValueMap &descr )
  {
    TrackableObserver::setParam( descr );
    
    std::string scriptType;
    
    std::string fileName;
    if ( descr.get( "script", fileName ) )
      setFileName( fileName.c_str() );

    descr.get( "scriptParameter", scriptParameter );

    if ( descr.get( "scriptType", scriptType ) )
    {
      tolower( scriptType );
      if ( scriptType == "count" )
	isCount = true;
      else if ( scriptType == "switch" )
	isSwitch = true;
    }
    descr.get( "count",  isCount  );
    descr.get( "switch", isSwitch );
  }
  
  bool observe( const ObsvObjects &other, bool force=false )
  { 
    if ( !TrackableObserver::observe( other, force ) )
      return false;
    
    for ( int i = rects.numRects()-1; i >= 0; --i )
    {
      ObsvRect &rect( rects.rect(i) );
      ObsvObjects &objects( rect.objects );
      
      int count = -1;

      if ( isCount || obsvFilter.filterEnabled( Filter::OBSV_COUNT ) )
      { if ( objects.lastCount != (int)objects.validCount )
	  count = (int)objects.validCount;
      }
      else if ( isSwitch || obsvFilter.filterEnabled( Filter::OBSV_SWITCH ) )
      { if ( ((bool)objects.lastCount) != (bool)objects.validCount )
	  count = (bool)objects.validCount;
      }

      if ( count >= 0 )
      {
	if ( !logFileName.empty() )
        {
	  std::string cmd( logFileName );

	  if ( cmd[0] != '.' && cmd[0] != '/' )
	    cmd = "./" + cmd;
	  
	  bool cmdExists = fileExists( cmd );
	  if ( verbose && !cmdExists)
	    error( "TrackableBashObserver: %s does not exist !!!", cmd.c_str() );

	  std::string param;
	  if ( isCount || obsvFilter.filterEnabled( Filter::OBSV_COUNT ) )
	  { param = "type=count count=";
	    param += std::to_string( count );
	  }
	  else
	  { param = "type=switch switch=";
	    param += (count ? "true" : "false");

	    if ( obsvFilter.filterEnabled( Filter::OBSV_SWITCH_DURATION ) )
            {
	      param += " switchduration=";
	      param += std::to_string( (int)(objects.timestamp-objects.switch_timestamp) );
	    }
	  }
	  
	  param += " ";

	  if ( obsvFilter.filterEnabled( Filter::OBSV_REGION ) )
          {
	    param += "region=\"";
	    param += objects.region;
	    param += "\" ";
	  }

	  if ( !scriptParameter.empty() )
	  { param += scriptParameter;  
	    param += " ";
	  }
	  
	  cmd = param + cmd;

	  if ( verbose )
	    info( "EXEC: %s\n", cmd.c_str() );

#if __LINUX__
	  if ( cmdExists )
	  {
	    cmd += " &";
	    system( cmd.c_str() );
	  }
#endif
	}
      }
    }

    return true;
  }
  
};


/***************************************************************************
*** 
*** TrackableFileObserver
***
****************************************************************************/

class TrackableFileObserver : public TrackableObserver
{
public:

  TrackableFileObserver()
  : TrackableObserver()
  {
    type 	   = File;
    continuous	   = false;
    fullFrame      = false;
    isJson	   = true;
    isThreaded     = true;
    name           = "file";

    obsvFilter.parseFilter( "timestamp=ts,action,start,stop,frame,regions,objects,type,enter,move,leave,x,y,z,size,id,lifespan,count" );
  }

  virtual void setParam( KeyValueMap &descr )
  {
    TrackableObserver::setParam( descr );
    
    std::string fileName;
    if ( descr.get( "file", fileName ) )
      setFileName( fileName.c_str() );
  }

  void write( std::vector<std::string> &messages, uint64_t timestamp=0  )
  {
    if ( timestamp == 0 )
      timestamp = getmsec();

    std::string fn( templateToFileName( timestamp ) );
	
    if ( verbose )
      info( "file: %s", fn.c_str() );

    std::ofstream file;
    if ( fn != "-" )
    { std::string path( filePath( fn ) );
      if ( !path.empty() && !fileExists( path.c_str() ) )
	std::filesystem::create_directories( path.c_str() );
      file.open( fn, std::ios_base::app );
    }
    
    std::ostream &out( fn == "-" ? std::cout : file );
    for ( int i = 0; i < messages.size(); ++i )
    {
      out << messages[i] << std::endl;
      if ( verbose )
	info( "log: %s",  messages[i].c_str() );
    }
  }
};

  
/***************************************************************************
*** 
*** TrackablePackedFileObserver
***
****************************************************************************/

class TrackablePackedFileObserver : public TrackableObserver
{
public:

  std::string	lastFileName;

  PackedTrackable::OFile	*file;

  TrackablePackedFileObserver()
  : TrackableObserver(),
    file( NULL )
  {
    type 	   = PackedFile;
    continuous	   = true;
    fullFrame      = true;
    isJson	   = false;
    isThreaded     = false;
    useLatent      = true;
    name           = "packedfile";
  }

  ~TrackablePackedFileObserver()
  {
    if ( file != NULL )
      delete file;
  }

  virtual void setParam( KeyValueMap &descr )
  {
    TrackableObserver::setParam( descr );
    
    std::string fileName;
    if ( descr.get( "file", fileName ) )
      setFileName( fileName.c_str() );
  }

  bool checkFile( uint64_t timestamp )
  {
    std::string fn( templateToFileName( timestamp ) );
    if ( fn != lastFileName && file != NULL )
    { delete file;
      file = NULL;
    }
    
    if ( file == NULL )
    { file = new PackedTrackable::OFile( fn.c_str() );
      lastFileName = fn;
    }

    return file != NULL;
  }
  

  bool observe( const ObsvObjects &other, bool force=false )
  { 
    if ( maxFPS <= 0.0 )
      maxFPS = 5;
    else if ( maxFPS > 100.0 )
      maxFPS = 100.0;

    if ( !TrackableObserver::observe( other, force ) )
      return false;
    
    if ( !reporting || !checkFile( other.timestamp ) )
      return false;
    
    PackedTrackable::BinaryFrame frame( other.timestamp, other.uuid );

    for ( auto &iter: other )
    { const ObsvObject &object( iter.second );
      frame.add( object.id, object.x, object.y, object.size, object.flags ); 
    }

    if ( verbose )
      info( "packedfile: %s put %d objects", lastFileName.c_str(), (int)frame.size() );

    file->put( frame );
    
    return true;
  }

  virtual bool start( uint64_t timestamp=0, bool startRects=false )
  {
    if ( timestamp == 0 )
      timestamp = getmsec();
    
    if ( !TrackableObserver::start(timestamp,startRects) )
      return false;

    if ( !reporting || !checkFile( timestamp ) )
      return true;
    
    PackedTrackable::Header header( timestamp, PackedTrackable::StartHeader );
    file->put( header );

    return true;
  }
  
  virtual bool stop( uint64_t timestamp=0, bool stopRects=false )
  { 
    if ( !TrackableObserver::stop(timestamp,stopRects) )
      return false;
    
    if ( !reporting || !checkFile( timestamp ) )
      return false;

    PackedTrackable::Header header( timestamp, PackedTrackable::StopHeader );
    file->put( header );

    return true;
  }

};

  
/***************************************************************************
*** 
*** TrackableUDPObserver
***
****************************************************************************/

class TrackableUDPObserver : public TrackableObserver
{
public:
  int sock;
  struct sockaddr_in destination;
  std::string hostname;
  std::string port;
  uint64_t    connectionFailedTimestamp;

  TrackableUDPObserver()
    : TrackableObserver(),
    sock( -1 ),
    connectionFailedTimestamp( 0 )
  {
    type 	   = UDP;
    isJson	   = true;
    isThreaded     = false;
    continuous	   = true;
    fullFrame	   = false;
    name           = "udp";
  }

  ~TrackableUDPObserver()
  {
    if ( sock != -1 )
      ::close(sock);
  }


  void setURL( const char *URL )
  {
    std::string url( URL );
    
    setFileName( url.c_str() );

    if ( verbose )
      info( "TrackableUDPObserver set url: %s", url.c_str() );
 
    const char *col = strstr(url.c_str(), ":");
    if ( col != 0 )
    {
      hostname.assign(url.c_str(), col);
      const char *s = col+1;
      port = s;
    }
    else
      port = url;

    if ( hostname.empty() )
      hostname = "localhost";
  }

  virtual void setParam( KeyValueMap &descr )
  {
    TrackableObserver::setParam( descr );
    
    std::string url;
    if ( descr.get( "url", url ) )
      setURL( url.c_str() );
  }

  bool openSocket( const std::string &hostname, const std::string &port )
  {
    if ( verbose )
      info( "open udp: %s:%s", hostname.c_str(), port.c_str() );
    
    struct addrinfo hints;
    struct addrinfo *result = 0, *rp = 0;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    int err = 0;

    err = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
    if (err != 0 )
    { error( "TrackableUDPObserver:  open udp: %s:%s  fails", hostname.c_str(), port.c_str() );
      return false;
    }

    for (rp = result; rp && sock==-1; rp = rp->ai_next)
    {
      sock = ::socket(rp->ai_family, rp->ai_socktype, 0);

      if ( sock != -1 )
      {    
	if (connect(sock, rp->ai_addr, (socklen_t)rp->ai_addrlen) != 0)
        { ::close(sock);
	  sock = -1;
	}
	else
	  break;
      }
    }

    freeaddrinfo(result);

    if ( sock == -1 )
      error( "TrackableUDPObserver:  open udp: %s:%s  fails", hostname.c_str(), port.c_str() );
    else if ( verbose )
      info( "TrackableUDPObserver:  open udp: %s:%s  succesful", hostname.c_str(), port.c_str() );

    return sock != -1;
  }

  void write( std::vector<std::string> &messages, uint64_t timestamp=0  )
  {
    if ( sock == -1 && !hostname.empty() && !port.empty() && getmsec()-connectionFailedTimestamp >= 1000 )
    {
      if ( !openSocket( hostname, port ) )
	connectionFailedTimestamp = getmsec();
    }

    if ( sock == -1 )
      return;
 
    int    flags    = MSG_NOSIGNAL;
    flags 	   |= MSG_DONTWAIT;

    for ( int i = 0; i < messages.size(); ++i )
    {
      std::string &msg( messages[i] );
      int len     = msg.length()+1;
      int n_bytes = ::send(sock, msg.c_str(), len, flags );
      if ( n_bytes != len )
      {
	if ( verbose )
        { 
	  //	  perror( "ibIPSocket::recvRawData()" );
	  error( "ERROR(%d): udp(%d,%s:%s): sending %d bytes '%s'", errno, sock, hostname.c_str(), port.c_str(), len, msg.c_str() );
	}
	::close(sock);
	sock = -1;
      }
      else if ( verbose )
	info( "udp(%d,%s:%s): '%s'", sock, hostname.c_str(), port.c_str(), msg.c_str() );
    }
  }
};

  
#endif // TRACKABLE_OBSERVER_H

