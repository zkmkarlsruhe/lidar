// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef TRACKABLE_H
#define TRACKABLE_H

#include "helper.h"

#include <mutex>
#include <stdio.h>
#include <stdlib.h> /* exit() */

#include <chrono>
#include <unistd.h>

#include <math.h>

#include <string.h>
#include <assert.h>
#include <regex>

#include "UUID.h"

namespace pv {

/***************************************************************************
*** 
*** Helper
***
****************************************************************************/

inline void incFrameCount( uint64_t &frame_count, uint64_t maxFrameCount=0xffffffff )
{ frame_count = (frame_count + 1) % maxFrameCount; }

/***************************************************************************
*** 
*** LatentTrackableIds
***
****************************************************************************/

class LatentId
{
  public:

  UUID 	   uuid;
  uint64_t timestamp;

  LatentId( UUID &uuid, uint64_t timestamp )
  : uuid( uuid ),
    timestamp( timestamp )
  {}

};

class LatentIds : public std::map<std::string,LatentId>
{
public:

  inline void put( std::string &id, UUID &uuid, uint64_t timestamp )
  {
    iterator iter( find(id) );
    
    if ( iter != end() )
      return;
        
    emplace( std::make_pair(id,LatentId(uuid,timestamp)) );
  }
  
  inline void remove( std::string &id )
  { erase( id );
  }
  
  inline bool isLatent( std::string &id )
  { 
    return find(id) != end();
  }
  
  
  inline bool get( std::string &id, uint64_t &timestamp )
  { 
    iterator oldestIter;
    uint64_t oldest = 0;

    for ( iterator iter = begin(); iter != end(); ++iter )
      if ( oldest == 0 || iter->second.timestamp < oldest )
      { oldest = iter->second.timestamp;
	oldestIter = iter;
      }
    
    if ( oldest == 0  )
      return false;
        
    id        = oldestIter->first;
    timestamp = oldestIter->second.timestamp;
    
    return true;
  }
  
  
  inline void addTime( uint64_t time )
  { 
    for ( iterator iter = begin(); iter != end(); ++iter )
      iter->second.timestamp += time;
  }

  inline void cleanup( int olderThanMSec, uint64_t timestamp=0 )
  { 
    if ( timestamp == 0 )
      timestamp = getmsec();
    
    for ( iterator iter = begin(); iter != end(); )
      if ( iter->second.timestamp+olderThanMSec < timestamp  )
      {
//	printf( "drop %s\n", iter->first.c_str() );
	iter = erase(iter);
      }
      else
	++iter;
  }

};


/***************************************************************************
*** 
*** Trackable
***
****************************************************************************/

template<typename Type> class Trackable : public Type
{
public:
  enum 
  {
    VALID=(1<<0)
  };
  
  enum  Flags
  { Touched  =  (1<<0),
    Private  =  (1<<1),
    Portal   =  (1<<2),
    Green    =  (1<<3),
    Latent   =  (1<<4),
    Immobile =  (1<<5),
    Occluded =  (1<<7),
    Default  =      0
  };

  int		numWeight;
  uint64_t 	firstTime;
  uint64_t 	lastTime;
  uint64_t 	firstPrivateTime;
  uint64_t 	firstImmobileTime;
  uint16_t	flags;
  bool		isActivated;
  bool		erasable;
  int		user1;
  int		user2;
  float		user3;
  float		user4;
  float		user5;
  float		confidence;
  float		splitProb;
  
  float		motionVector[3];
  float		Pos[3];
  float		Size;
  float		predictedPos[3];
  float		firstImmobilePos[3];
  
  UUID		uuid;

  typedef std::shared_ptr<Trackable<Type>> Ptr;

  std::string   Id;

  LatentIds    latentIds;
  
/*
  Trackable()
  : Type(),
  numWeight( 0 ),
  firstTime( 0 ),
  lastTime( 0 ),
  flags(  ),
  isActivated( false ),
  uuid()
  {
    motionVector[0] = 0.0;
    motionVector[1] = 0.0;
    motionVector[2] = 0.0;
  }
*/

  inline void
  dump( const char *prefix="" )
  {
    printf( "%sT(%s,%d): (%d) (%g,%g - %g) -> (%g,%g)\n", prefix, Id.c_str(), isActivated, numWeight, this->p[0], this->p[1], this->size, motionVector[0], motionVector[1] );
  }
       

  inline void init( uint64_t timestamp, bool initValues=true )
  { firstTime   = timestamp;
    lastTime    = timestamp;
    firstPrivateTime  = 0;
    firstImmobileTime = 0;
    isActivated = false;
    erasable    = false;
    flags       = 0;
    numWeight   = 1;
    if ( initValues )
    { confidence  = 0;
      splitProb   = 0;
    }
    motionVector[0] = 0;
    motionVector[1] = 0;
    motionVector[2] = 0;
    Pos[0]    = this->p[0];
    Pos[1]    = this->p[1];
    Pos[2]    = this->p[2];
    Size      = this->size;
  }
  
  static inline uint64_t getmsec()
  { return ::getmsec();
  }
  
  inline void		touchTime( uint64_t timestamp )
  { lastTime = timestamp;
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
    { if ( firstPrivateTime == 0 )
	firstPrivateTime = timestamp;
      else if ( timestamp - firstPrivateTime > timeout )
	setPrivate( true );
    }
    else
      firstPrivateTime = 0;
  }
  
  inline bool		isPortal() const
  { return flags & Portal;
  }

  inline void		setPortal( bool set )
  { if ( set == (flags & Portal) )
      return;
    if ( set )
      flags |=  Portal;
    else
      flags &= ~Portal;
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
    double d0 = firstImmobilePos[0] - Pos[0];
    double d1 = firstImmobilePos[1] - Pos[1];
    double distance = sqrt( d0*d0 + d1*d1 );
    if ( distance > maxDist )
    { firstImmobilePos[0] = Pos[0];
      firstImmobilePos[1] = Pos[1];
      firstImmobilePos[2] = Pos[2];
      firstImmobileTime   = timestamp;
      setImmobile( false );
    }
    else
    {
      if ( firstImmobileTime == 0 )
	firstImmobileTime = timestamp;
      else if ( !(flags&Immobile) && timestamp - firstImmobileTime > timeout )
	setImmobile( true );
    }
  }
  
  static inline std::string	nextId( bool reset=false )
  {
    static uint32_t id = 0;
    if ( reset )
      id = 0;
    else if ( (id+=1) == 0 )
      id = 1;
    
    return std::to_string( id );
  }
  
  std::string	&id( uint64_t starttime=0 )
  { if ( Id.empty() || starttime != 0 )
    {
      Id = nextId();
      if ( starttime != 0 )
	uuid.update( starttime, std::atoi(Id.c_str()) );
      else
	uuid.update();
    }
    return Id;
  }
  
  void setId( std::string &id, uint64_t starttime )
  { Id = id;
    uuid.update( starttime, std::atoi(Id.c_str()) );
  }
  
  void	swapId( Trackable<Type> &other )
  { std::string tmpId( id() );
    Id = other.id();
    other.Id = tmpId;
    
    UUID tmpUUID( uuid );
    uuid = other.uuid;
    other.uuid = tmpUUID;

    uint16_t tmpFlags( flags&(Private|Immobile) );
    setPrivate ( other.flags&Private );
    setImmobile( other.flags&Immobile );
    other.setPrivate( tmpFlags&Private );
    other.setImmobile( tmpFlags&Immobile );
  }
  
  double distanceTo( Trackable &other, float offsetX=0.0f, float offsetY=0.0f, float offsetZ=0.0f )
  {
    double d0 = Pos[0] - other.Pos[0] + offsetX;
    double d1 = Pos[1] - other.Pos[1] + offsetY;
    
//    if ( isnan(Pos[2]) )
      return sqrt( d0*d0 + d1*d1 );
    
//    double d2 = Pos[2] - other.Pos[2] + offsetZ;

//    return sqrt( d0*d0 + d1*d1 + d2*d2 );
  }
  
  void mixWith( Trackable &other, double weight=-1.0 )
  {
    if ( weight < 0 )
    { weight = 0.5;
      if ( Size > 0 && other.Size > 0 )
	weight = Size / (Size + other.Size);
    }
    
    double oneMinusWeight = 1.0-weight;
    
    Pos[0] = weight * Pos[0] + oneMinusWeight * other.Pos[0];
    Pos[1] = weight * Pos[1] + oneMinusWeight * other.Pos[1];
    if ( !isnan(Pos[2]) )
      Pos[2] = weight * Pos[2] + oneMinusWeight * other.Pos[2];

//    printf( "size: %g %g\n", size, other.size );

    Size = weight * Size + oneMinusWeight * other.Size;
  }
  
};
 
/***************************************************************************
*** 
*** Trackables
***
****************************************************************************/

template<typename Type> class Trackables : public std::vector<typename Trackable<Type>::Ptr>
{
public:
  typedef std::shared_ptr<Trackables> Ptr;

  std::mutex mutex;

  void cleanup( int olderThanMSec, uint64_t timestamp=0, uint64_t time_diff=0 )
  { 
    if ( timestamp == 0 )
      timestamp = getmsec();
    
    for ( int i = 0; i < this->size(); ++i )
    {
      Trackable<Type> &trackable( *(*this)[i] );
      if ( trackable.splitProb > 0.85 )
	trackable.latentIds.addTime( time_diff );
      
      trackable.latentIds.cleanup( olderThanMSec, timestamp );
    }
  }

  rapidjson::Value toJson()
  {	
    rapidjson::Value json( rapidjson::kArrayType );
      
    for( int i = 0; i < this->size(); ++i )
      json[i] = (*this)[i]->toJson();
	
    return json;
  }

  bool fromJson( rapidjson::Value &json )
  {
    bool result = true;

    for ( int i = 0; i < json.Size(); ++i )
    { 
      Trackable<Type> *trackable;
      rapidjson::Value &trackableJson( json[i] );
      this->push_back(trackable=new Trackable<Type>);
      result = (trackable->fromJson( trackableJson ) || result );
    }

    return result;
  }
};

/***************************************************************************
*** 
*** TrackableStage
***
****************************************************************************/

template<typename Type> class TrackableStage
{
public:
  typedef std::shared_ptr<TrackableStage> Ptr;

  bool		isMulti;
  std::string   stageId;
  std::mutex 	mutex;
  std::mutex 	mutexCurrent;

  typename Trackables<Type>::Ptr latest;
  typename Trackables<Type>::Ptr current;

  uint64_t lastTime;
  uint64_t frame_count;

  TrackableStage()
    : isMulti( false ),
      frame_count( 0 ),
      stageId(),
      latest( new Trackables<Type> ),
      current( new Trackables<Type> )
  {
  }
  
  TrackableStage( std::string id )
    : isMulti( false ),
      frame_count( 0 ),
      stageId( id ),
      latest( new Trackables<Type> ),
      current( new Trackables<Type> )
  {
  }
  
  inline void lock()
  { mutex.lock();
  }
  
  inline void unlock()
  { mutex.unlock();
  }
  
  inline void lockCurrent()
  { mutexCurrent.lock();
  }
  
  inline void unlockCurrent()
  { mutexCurrent.unlock();
  }
  
  inline int size()
  { return latest->size();
  }
  
  inline void add( typename Trackable<Type>::Ptr &trackable )
  { current->push_back( trackable );
  }
  
  inline typename Trackable<Type>::Ptr &operator[]( int index )
  { return (*latest)[index];
  }

  inline void touchTime( uint64_t timestamp=0 )
  { 
    if ( timestamp == 0 )
      timestamp = getmsec();

    lastTime = timestamp;
  }
  
  virtual void finish( uint64_t timestamp )
  { 
    if ( timestamp == 0 )
      timestamp = getmsec();

    incFrameCount( frame_count );
    touchTime( timestamp );
  }

  virtual void swap()
  { 
    latest  = current;
    current = typename Trackables<Type>::Ptr( new Trackables<Type> );
  }

  virtual void reset()
  {
    latest  = typename Trackables<Type>::Ptr( new Trackables<Type> );
    current = typename Trackables<Type>::Ptr( new Trackables<Type> );
  }
  
  virtual Trackable<Type> *createTrackable()
  { return new Trackable<Type>();
  }


  typename Trackable<Type>::Ptr newTrackable( uint64_t timestamp )
  { 
    current->push_back(typename Trackable<Type>::Ptr( createTrackable()) );

    Trackable<Type> &trackable( *current->back() );
    trackable.init( timestamp );

    return current->back();
  }

};

/***************************************************************************
*** 
*** TrackableMultiStages
***
****************************************************************************/

template<typename Type> class TrackableStages : public std::vector<typename TrackableStage<Type>::Ptr>
{
public:
  typedef std::shared_ptr<TrackableStages> Ptr;

  typename TrackableStage<Type>::Ptr getStage( const char *stageId )
  {
    for ( int i = ((int)this->size())-1; i >= 0; --i )
    { 
      if ( (*this)[i]->stageId == stageId )
	return (*this)[i];
    }
    return NULL;
  }  

  void removeStage( const char *stageId )
  {
    for ( int i = ((int)this->size())-1; i >= 0; --i )
    { 
      if ( (*this)[i]->stageId == stageId )
      { this->erase( this->begin()+i );
	return;
      }    
    }
  }  

};

/***************************************************************************
*** 
*** TrackableMultiStage
***
****************************************************************************/

template<typename Type> class TrackableMultiStage : public TrackableStage<Type>
{
public:
  struct Vector3D
  { float x, y, z;
    Vector3D( float p[] ) : x(p[0]), y(p[1]), z(p[2]) {}
    Vector3D( float x, float y, float z=0.0f ) : x(x), y(y), z(z) {}
    float length() const { return sqrt( x*x + y*y + z*z ); }
      
  };
    
  TrackableStages<Type> subStages;
#if USE_CAMERA
  imCameraGroup  	cameras;
#endif
  double		uniteDistance;
  double		trackDistance;
  double		trackOldestFactor;
  double		latentDistance;
  uint64_t		latentLifeTime;
  double		trackMotionPredict;
  double		keepTime;
  double		minActiveTime;
  double		minActiveFraction;
  double		privateTimeout;
  double		immobileTimeout;
  double		immobileDistance;
  double		trackFilterWeight;
  double		trackSmoothing;
  double		objectMaxSize;
  bool			trackDistance2D;
  bool			uniteInSingleStage;
  int			isStarted;
  uint64_t		timestamp;
  uint64_t		starttime;

  UUID			uuid;
  

#ifdef TRACKABLE_OBSERVER_H
  TrackableMultiObserver  *observer;
  ObsvObjects		   obsvObjects;
#endif

  int			(*trackableMask)( Trackable<Type> &trackable );

  struct TrackInfo
  {
    double distance;
    int    currentIndex;
    int    mergedIndex;
  };

  static inline bool compareTrackInfo(const TrackInfo& t1, const TrackInfo& t2) { return t1.distance < t2.distance; }

  typedef std::shared_ptr<TrackableMultiStage> Ptr; 

  typename TrackableStage<Type>::Ptr getByIdentifier( const char *stageId )
  { 
    return subStages.getStage( stageId );
  }

  TrackableMultiStage()
    : TrackableStage<Type>(),
      uniteDistance       ( 0.75 ),
      trackDistance       ( 1.2 ),
      trackOldestFactor   ( 0.0 ),
      latentDistance      ( 0.0 ),
      latentLifeTime 	  ( 10000 ),
      trackMotionPredict  ( 0.0 ),
      keepTime	          ( 1000 ),
      minActiveTime       ( 500 ),
      minActiveFraction   ( 0.25 ),
      trackFilterWeight   ( 0.5 ),
      trackSmoothing      ( 0.6 ),
      objectMaxSize       ( 0.0 ),
      trackDistance2D     ( true ),
      privateTimeout	  ( 5000 ),
      immobileTimeout	  ( 60*60*1000 ),
      immobileDistance	  ( 1.0 ),
      uniteInSingleStage  ( false ),
      isStarted           ( -1 ),
      timestamp		  ( 0 ),
      starttime		  ( 0 ),
      trackableMask 	  ( NULL )
#ifdef TRACKABLE_OBSERVER_H
  ,   observer            ( NULL )
#endif
  {
    this->isMulti = true;
  }

  ~TrackableMultiStage()
  {
    stop( timestamp );
    
#ifdef TRACKABLE_OBSERVER_H
    if ( observer != NULL )
      delete observer;
#endif
  }
  
  void start( uint64_t timestamp=0 )
  {
    if ( isStarted == 1 ) return;
    isStarted = 1;

    starttime = (timestamp == 0 ? getmsec() : timestamp);
    uuid.update( starttime );

    Trackable<Type>::nextId( true );

#ifdef TRACKABLE_OBSERVER_H
    if ( observer != NULL )
      observer->start( timestamp, true );
#endif
  }
  
  void startAlwaysObserver( uint64_t timestamp=0 )
  {
    starttime = (timestamp == 0 ? getmsec() : timestamp);

#ifdef TRACKABLE_OBSERVER_H
    if ( observer != NULL )
      observer->startAlwaysObserver( timestamp, true );
#endif
  }
  
  void stop( uint64_t timestamp=0 )
  {
    if ( isStarted == 0 ) return;
    clear();

    isStarted = 0;

#ifdef TRACKABLE_OBSERVER_H
    if ( observer != NULL )
      observer->stop( timestamp );
#endif
  }
  

  static inline void printArgHelpImpl( const char *name, float value, const char *descr )
  {
    printf( "  %s (default: %g)  \t%s\n", name, value, descr );
  }
  
  virtual void printArgHelp() const
  {
    printArgHelpImpl( "track.uniteDistance",      uniteDistance,      "\tmax distance between objects to be united to a singe layer" );
    printArgHelpImpl( "track.trackDistance",      trackDistance,      "\tmax distance between objects to be identified as the same object in consecutive frames" );
    printArgHelpImpl( "track.trackOldestFactor",  trackOldestFactor,  "if trackable is dropped, search in trackOldestFactor * trackDistance for younger one" );
    printArgHelpImpl( "track.latentDistance",     latentDistance,  "\tif trackable is dropped, keep it latent in the closest neighbour found in latentDistance" );
    printArgHelpImpl( "track.latentLifeTime", 	  latentLifeTime / 1000.0,  "\tkeep latent ids for latentLifeTime seconds" );
    printArgHelpImpl( "track.objectMaxSize",      objectMaxSize,      "\tmax object size before splitting (if implemented)" );
    printArgHelpImpl( "track.trackMotionPredict", trackMotionPredict, "weight of motion prediction in consecutive frames" );
    printArgHelpImpl( "track.keepTime",           keepTime / 1000.0,           "\t\tsec to keep object in tracked layer even if it is not detected. After that time it is dropped" );
    printArgHelpImpl( "track.minActiveTime",      minActiveTime / 1000.0,      "\tmin time an object has to be active before it appears as being tracked" );
    printArgHelpImpl( "track.minActiveFraction",  minActiveFraction,  "fraction of min Active time the object has to be continuousely detected before it appears as being tracked" );
    printArgHelpImpl( "track.trackFilterWeight",  trackFilterWeight,  "filter weight between old and new values. 0 = copy, 1 = no change" );
    printArgHelpImpl( "track.trackSmoothing",     trackSmoothing,     "\tsmoothing of values. 0 = copy, 1 = no change" );
    printArgHelpImpl( "track.distance2D",         trackDistance2D,    "\tdistance calculation: 0 = 3D, 1 = 2D" );
    printArgHelpImpl( "track.privateTimeout",     privateTimeout / 1000.0,     "\tsec to stay in private area until marked as private" );
    printArgHelpImpl( "track.immobileTimeout",    immobileTimeout / 1000.0,     "sec to be immobile until marked as immobile" );
    printArgHelpImpl( "track.immobileDistance",   immobileDistance,     "\tdistance in meter to be moved for not regardes as beeing immobile" );
  }
  
  virtual bool parseArg( int &i, const char *argv[], int &argc ) 
  {
    bool success = true;

    if ( strcmp(argv[i],"track.uniteDistance") == 0 )
    {
      uniteDistance = atof( argv[++i] );
    }
    else if ( strcmp(argv[i],"track.trackDistance") == 0 )
    {
      trackDistance = atof( argv[++i] );
    }
    else if ( strcmp(argv[i],"track.trackOldestFactor") == 0 )
    {
      trackOldestFactor = atof( argv[++i] );
    }
    else if ( strcmp(argv[i],"track.latentDistance") == 0 )
    {
      latentDistance = atof( argv[++i] );
    }
    else if ( strcmp(argv[i],"track.latentLifeTime") == 0 )
    {
      latentLifeTime = 1000 * atoi( argv[++i] );
    }
    else if ( strcmp(argv[i],"track.objectMaxSize") == 0 )
    {
      objectMaxSize = atof( argv[++i] );
    }
    else if ( strcmp(argv[i],"track.trackMotionPredict") == 0 )
    {
      trackMotionPredict = atof( argv[++i] );
    }
    else if ( strcmp(argv[i],"track.keepTime") == 0 )
    {
      keepTime = atof( argv[++i] ) * 1000;
    }
    else if ( strcmp(argv[i],"track.minActiveTime") == 0 )
    {
      minActiveTime = atof( argv[++i] ) * 1000;
    }
    else if ( strcmp(argv[i],"track.minActiveFraction") == 0 )
    {
      minActiveFraction = atof( argv[++i] );
    }
    else if ( strcmp(argv[i],"track.trackFilterWeight") == 0 )
    {
      trackFilterWeight = atof( argv[++i] );
    }
    else if ( strcmp(argv[i],"track.trackSmoothing") == 0 )
    {
      trackSmoothing = atof( argv[++i] );
    }
    else if ( strcmp(argv[i],"track.distance2D") == 0 )
    {
      trackDistance2D = atoi( argv[++i] );
    }
    else if ( strcmp(argv[i],"track.privateTimeout") == 0 )
    {
      privateTimeout = atoi( argv[++i] ) * 1000;
    }
    else if ( strcmp(argv[i],"track.immobileTimeout") == 0 )
    {
      immobileTimeout = atoi( argv[++i] ) * 1000;
    }
    else if ( strcmp(argv[i],"track.immobileDistance") == 0 )
    {
      immobileDistance = atof( argv[++i] );
    }
    else
      success = false;
    
    return success;
  }
  
  virtual void reset()
  {
    for ( int i = ((int)this->subStages.size())-1; i >= 0; --i )
      this->subStages[i]->reset();
    
    clear();

    TrackableStage<Type>::reset();
  }
  
  TrackableStage<Type> &getStage( const char *stageId, bool createIfMissing=false )
  { 
    typename TrackableStage<Type>::Ptr stg( this->subStages.getStage( stageId ) );
    
    if ( stg != NULL )
      return *stg;
    
    if ( !createIfMissing )
      return *this;
     
    this->subStages.push_back( typename TrackableStage<Type>::Ptr(new TrackableStage<Type>( std::string(stageId) )) );

    return *this->subStages.back();
  }
  
  virtual void addStage( Trackables<Type> &merged, Trackables<Type> &subStage )
  {
	/* add new trackable  */
    for ( int i = ((int)subStage.size())-1; i >= 0; --i )
    { 
      Trackable<Type> &subStageTrackable = *subStage[i];

      typename Trackable<Type>::Ptr newTrackable;
      merged.push_back( newTrackable=typename Trackable<Type>::Ptr( this->createTrackable()) );
      *newTrackable = subStageTrackable;
      newTrackable->init( timestamp, false );
    }
  }
  
 	/* sort by distances */
  virtual void mergeStage( Trackables<Type> &merged, Trackables<Type> &subStage )
  {
    std::vector<TrackInfo> trackInfo;
    
    bool mergedUsed[merged.size()+1]     = { false };
    bool subStageUsed[subStage.size()+1] = { false };
    std::fill_n( mergedUsed,   merged.size()+1, false );
    std::fill_n( subStageUsed, subStage.size()+1, false );

	/* calculate distances from substage to merged stage */
    for ( int i = ((int)subStage.size())-1; i >= 0; --i )
    { Trackable<Type> &subStageTrackable = *subStage[i];
      for ( int j = ((int)merged.size())-1; j >= 0; --j )
      { 
	double d = subStageTrackable.distanceTo( *merged[j] );
	if ( d <= uniteDistance )
	  trackInfo.push_back( TrackInfo({d, i, j}) ); /* { distance, currentIndex, mergedIndex } */
      }
    }

 	/* sort by distances */
    sort( trackInfo.begin(), trackInfo.end(), compareTrackInfo );

	/* assign corresponding trackables in subStage to merged stage */
    for ( int i = 0; i < trackInfo.size(); ++i )
    {
      TrackInfo &ti( trackInfo[i] );

	  /* only if they are not already assigned */
      if ( !subStageUsed[ti.currentIndex] && !mergedUsed[ti.mergedIndex] )
      {
	Trackable<Type> &mergedTrackable = *merged[ti.mergedIndex];

	mergedTrackable.numWeight += 1;
	mergedTrackable.mixWith( *subStage[ti.currentIndex], 1.0/mergedTrackable.numWeight );
	subStageUsed[ti.currentIndex] = true;
	mergedUsed[ti.mergedIndex]    = true;
      }
    }
    
	/* add new trackable with no correspondence */
    for ( int i = ((int)subStage.size())-1; i >= 0; --i )
    { 
      if ( !subStageUsed[i] )
      {
	Trackable<Type> &subStageTrackable = *subStage[i];

	typename Trackable<Type>::Ptr newTrackable;
	merged.push_back( newTrackable=typename Trackable<Type>::Ptr( this->createTrackable()) );
	*newTrackable = subStageTrackable;
	newTrackable->init( timestamp, false );
      }
    }
  }

  virtual void addSubStages( Trackables<Type> &stage )
  {
    for ( int i = ((int)this->subStages.size())-1; i >= 0; --i )
      addStage( stage, *this->subStages[i]->latest );
  }
  
  virtual void mergeSubStages( Trackables<Type> &stage )
  {
    for ( int i = ((int)this->subStages.size())-1; i >= 0; --i )
      mergeStage( stage, *this->subStages[i]->latest );
  }
  
  virtual void mergeSubStagesInSingleStage( Trackables<Type> &merged )
  {
    for ( int s = ((int)this->subStages.size())-1; s >= 0; --s )
    {
      Trackables<Type> &subStage( *this->subStages[s]->latest );
      
      for ( int i = ((int)subStage.size())-1; i >= 0; --i )
      {
	Trackable<Type> &trackable = *subStage[i];

	typename Trackable<Type>::Ptr newTrackable;
	merged.push_back( newTrackable=typename Trackable<Type>::Ptr( this->createTrackable()) );
	*newTrackable = trackable;
	newTrackable->init( timestamp, false );
      }
    }

    std::vector<TrackInfo> trackInfo;
    
	/* calculate distances from trackables in substages */
    for ( int i = ((int)merged.size())-1; i > 0; --i )
    { for ( int j = i-1; j >= 0; --j )
      { double d = merged[i]->distanceTo( *merged[j] );
	if ( d <= uniteDistance )
	  trackInfo.push_back( TrackInfo({d, i, j}) );
      }
    }

 	/* sort by distances */
    sort( trackInfo.begin(), trackInfo.end(), compareTrackInfo );

    int  mixedIndex[merged.size()];
    std::fill_n( mixedIndex, merged.size(), -1 );

	/* correlate trackables with min distance */
    for ( int i = 0; i < trackInfo.size(); ++i )
    {
      TrackInfo &ti( trackInfo[i] );

	  /* only if one of them is not already assigned */
      if ( mixedIndex[ti.currentIndex] < 0 || mixedIndex[ti.mergedIndex] < 0 )
      {
	int mergedIndex  = ti.mergedIndex;
	int currentIndex = ti.currentIndex;
	if ( mixedIndex[mergedIndex] < 0 ) // make the merged the assigned one if there is one
	  std::swap( mergedIndex, currentIndex );

	while ( mixedIndex[mergedIndex] >= 0 )
	  mergedIndex = mixedIndex[mergedIndex];

	if ( mergedIndex != currentIndex )
        {
	  Trackable<Type> &mergedTrackable  = *merged[mergedIndex];
	  Trackable<Type> &currentTrackable = *merged[currentIndex];

	    /* only if they are not already assigned */

	  mergedTrackable.numWeight += 1;
	  mergedTrackable.mixWith( currentTrackable, currentTrackable.numWeight/mergedTrackable.numWeight );

	  mixedIndex[currentIndex] = mergedIndex;
	}
      }
    }

    for ( int i = ((int)merged.size())-1; i >= 0; --i )
      if ( mixedIndex[i] >= 0 )
	merged.erase( merged.begin()+i );
    
	/* remove unused trackables */
  }


  bool isCloser( Trackable<Type> &currentTrackable, Trackable<Type> &trackable, float currentSpeed, float time, float &distance )
  {
    Vector3D speed2v( trackable.motionVector[0], trackable.motionVector[1], 0.0 );
    float speed2    = speed2v.length();
    float speedSum  = currentSpeed + speed2;
    float speedDist = 5.0 * time * speedSum;
    
    double d = trackable.distanceTo( currentTrackable );
    
    if ( d < distance + speedDist )
    { distance = d;
      return true;
    }
    
    return false;
  }
  
  
  bool isInPortal( Trackable<Type> &currentTrackable )
  {
    int maskBits = 0;
	  
    if ( trackableMask != NULL )
      maskBits = trackableMask( currentTrackable );
	  
    return maskBits & Trackable<Type>::Portal;
  }
  

  bool swapToOldest( int currentIndex, float maxDistance, float time, int *currentMap )
  {
    if ( maxDistance <= 0.0 )
      return false;
    
    Trackables<Type> &current( *this->current );
    Trackable<Type>  &currentTrackable( *current[currentIndex] );

    if ( !currentTrackable.isActivated )
      return false;

    int swapIndex = -1;

    Vector3D currentSpeedVec( currentTrackable.motionVector[0], currentTrackable.motionVector[1], 0.0 );
    float currentSpeed = currentSpeedVec.length();

    float distance = maxDistance;

    for ( int j = ((int)current.size())-1; j >= 0; --j ) // check if there is a younger one in the near
    { 
      if ( currentMap[j] >= 0 && current[j]->isActivated )
      { 
	Trackable<Type> &trackable( *current[j] );

	if ( currentTrackable.firstTime < trackable.firstTime ) // is older than trackable
	{ if ( isCloser( currentTrackable, trackable, currentSpeed, time, distance ) )
	    swapIndex = j;
	}
      }
    }

    if ( swapIndex >= 0 )
      current[swapIndex]->swapId( currentTrackable );
    
    return swapIndex >= 0;
  }

  void putLatentId( int currentIndex, float maxDistance, float time, uint64_t timestamp )
  {
    if ( maxDistance <= 0.0 )
      return;
    
    Trackables<Type> &current( *this->current );
    Trackable<Type>  &currentTrackable( *current[currentIndex] );

    Vector3D currentSpeedVec( currentTrackable.motionVector[0], currentTrackable.motionVector[1], 0.0 );
    float currentSpeed = currentSpeedVec.length();

    float distance = maxDistance;

    int latentHost = -1;
    
    for ( int i = ((int)current.size())-1; i >= 0; --i ) // check if there is a younger one in the near
    { 
      if ( i != currentIndex && current[i]->isActivated )
      { 
	Trackable<Type> &trackable( *current[i] );
	if ( isCloser( currentTrackable, trackable, currentSpeed, time, distance ) )
	  latentHost = i;
      }
    }

    if ( latentHost >= 0 )
    {
//      printf( "put %s -> %s\n", currentTrackable.id().c_str(), current[latentHost]->id().c_str() );
      current[latentHost]->latentIds.put( currentTrackable.id(), currentTrackable.uuid, timestamp );
    }  
  }
  

  std::string getLatentId( int currentIndex, float maxDistance, float time, uint64_t &timestamp )
  {
    std::string latentId;
    
    if ( maxDistance <= 0.0 )
      return latentId;
    
    Trackables<Type> &current( *this->current );
    Trackable<Type>  &currentTrackable( *current[currentIndex] );

    Vector3D currentSpeedVec( currentTrackable.motionVector[0], currentTrackable.motionVector[1], 0.0 );
    float currentSpeed = currentSpeedVec.length();

    float distance = maxDistance;

    for ( int i = ((int)current.size())-1; i >= 0; --i ) // check if there is a trackable with latent latent in the near
    { 
      if ( current[i]->isActivated )
      { 
	Trackable<Type> &trackable( *current[i] );
	std::string lId;
	if ( trackable.latentIds.get( lId, timestamp ) )
	{ 
	  if ( isCloser( currentTrackable, trackable, currentSpeed, time, distance ) )
	  { latentId = lId;
	    trackable.latentIds.remove( lId );
	  }
	}
      }
    }

    return latentId;
  }

  float length( float *p )
  {
    double d = 0;
    for ( int i = (isnan(p[2]) ? 1 : 2); i >= 0; --i )
      d += p[i] * p[i];

    return sqrt( d );
  }
  
  void limitSpeed( Trackable<Type> &trackable, double maxSpeed )
  {
    double d = 0;
    for ( int i = (isnan(trackable.Pos[2]) ? 1 : 2); i >= 0; --i )
      d += trackable.motionVector[i] * trackable.motionVector[i];

    d = sqrt( d );
    if ( d > maxSpeed )
    { const double factor = maxSpeed / d;
      trackable.motionVector[0] *= factor;
      trackable.motionVector[1] *= factor;
      trackable.motionVector[2] *= factor;
    }
  }

  bool isValidDuration( int64_t duration ) const
  {
    return duration > 0 && duration < 5000;
  }

  bool isValidSpeed( int64_t duration, float distance ) const
  {
    return isValidDuration(duration) && distance / (duration/1000.0) < 2.0; // speed limit is 1m/s
  }

  float motionTime( uint64_t dt ) const
  {
    if ( dt < 4000/30 )
      return dt / 1000.0;

    return 0.0;
  }
  
  float predictWeight( uint64_t dt ) const
  { return trackMotionPredict * motionTime( dt );
  }

  virtual void unite( uint64_t timestamp=0 )
  {
    if ( timestamp == 0 )
      timestamp = getmsec();

	/* merge all substages into a current single layer */

    Trackables<Type> &latest ( *this->latest );
    Trackables<Type> &current( *this->current );
    Trackables<Type> merged;

    if ( isStarted )
    { if ( uniteInSingleStage )
	mergeSubStagesInSingleStage( merged );
      else
	mergeSubStages( merged );
    }
    
    std::vector<TrackInfo> trackInfo;
    
    uint64_t now      = timestamp;
    int64_t time_diff = now - this->timestamp;
    this->timestamp   = now;

    if ( false && time_diff < 0 )
    { printf( "timediff jumps: %ld\n", time_diff );
      time_diff = 1;  
    }
    
    std::vector<Vector3D> lastPos;

    for ( int i = 0; i < current.size(); ++i )
    { Trackable<Type> &currentTrackable = *current[i];
      lastPos.push_back( Vector3D(currentTrackable.Pos) );
    }

    float predictWeight = this->predictWeight( time_diff );
    float time 		= this->motionTime   ( time_diff );

	/* calculate predicted Positions */
    for ( int i = ((int)current.size())-1; i >= 0; --i )
    { Trackable<Type> &currentTrackable = *current[i];
      currentTrackable.predictedPos[0] = currentTrackable.Pos[0] + predictWeight * currentTrackable.motionVector[0];
      currentTrackable.predictedPos[1] = currentTrackable.Pos[1] + predictWeight * currentTrackable.motionVector[1];
      currentTrackable.predictedPos[2] = currentTrackable.Pos[2] + predictWeight * currentTrackable.motionVector[2];
    }
    
	/* calculate distances from merged to current */
    for ( int i = ((int)current.size())-1; i >= 0; --i )
    { Trackable<Type> &currentTrackable = *current[i];

      for ( int j = ((int)merged.size())-1; j >= 0; --j )
      { Trackable<Type> &mergedTrackable( *merged[j] );
	double d = mergedTrackable.distanceTo( currentTrackable,
					       predictWeight * currentTrackable.motionVector[0],
					       predictWeight * currentTrackable.motionVector[1],
					       predictWeight * currentTrackable.motionVector[2] );
	
	if ( d <= trackDistance )
	  trackInfo.push_back( TrackInfo({d, i, j}) );
      }
    }

	/* sort by distances */
    sort( trackInfo.begin(), trackInfo.end(), compareTrackInfo );

    int currentMap[current.size()+1] = { -1 };
    int mergedMap[merged.size()+1]   = { -1 };
    std::fill_n(currentMap, current.size()+1, -1 );
    std::fill_n(mergedMap, merged.size()+1, -1 );

	/* assign corresponding trackables in merged to current */
    for ( int i = 0; i < trackInfo.size() && trackInfo[i].distance < trackDistance; ++i )
    {
      TrackInfo &ti( trackInfo[i] );

	  /* only if they are not already assigned */
      if ( currentMap[ti.currentIndex] < 0 && mergedMap[ti.mergedIndex] < 0 )
      { currentMap[ti.currentIndex] = ti.mergedIndex;
	mergedMap[ti.mergedIndex]   = ti.currentIndex;
      }
    }

	/* check if an older non assigned trackable is close to an assigned non activated trackable */
    for ( int i = 0; i < trackInfo.size() && trackInfo[i].distance < trackDistance; ++i )
    {
      TrackInfo &ti( trackInfo[i] );

	  /* current not assigned, but merged is */
      if ( currentMap[ti.currentIndex] < 0 && mergedMap[ti.mergedIndex] >= 0 )
      { 
	int currentMergedIndex = mergedMap[ti.mergedIndex]; /* current assigned merged trackable */
	
	Trackable<Type> &currentMergedTrackable  = *current[currentMergedIndex];
	if ( !currentMergedTrackable.isActivated ) /* merged is not activated */
	{
	  Trackable<Type> &currentTrackable = *current[ti.currentIndex];
	  
	      /* if current is older than merged (which is not activated yet), then assign is instead of merged */
	  if ( currentTrackable.isActivated && currentTrackable.firstTime < currentMergedTrackable.firstTime )
	  {
	    int mergedCurrentIndex         = currentMap[ti.mergedIndex];
	    currentMap[ti.currentIndex]    = ti.mergedIndex;
	    currentMap[currentMergedIndex] = -1;
	    mergedMap[mergedCurrentIndex]  = ti.currentIndex;
//	    printf( "swap\n" );
	  }
	}
      }
    }

	/* create trackables assigned in merged to current */
    for ( int currentIndex = ((int)current.size())-1; currentIndex >= 0; --currentIndex )
    {
      if ( currentMap[currentIndex] >= 0 )
      {
	Trackable<Type> &currentTrackable = *current[currentIndex];

	int mergedIndex = currentMap[currentIndex];

	currentTrackable.mixWith( *merged[mergedIndex], trackFilterWeight );
	currentTrackable.lastTime  = now;

	currentTrackable.user1     = merged[mergedIndex]->user1;
	currentTrackable.user2     = merged[mergedIndex]->user2;
	currentTrackable.splitProb = merged[mergedIndex]->splitProb;
      }
    }

	/* calculate motion */
    for ( int i = ((int)current.size())-1; i >= 0; --i )
    {
      Trackable<Type> &currentTrackable( *current[i] );

      const float minTime = 1.0/80.0;

      if ( time > minTime )
      {
	if ( currentMap[i] >= 0 ) // mix new motionvector with old
        {
	  const float alpha = 0.25 * (1.0 - trackFilterWeight);

	  Vector3D motionVector( (currentTrackable.Pos[0]-lastPos[i].x) / time, (currentTrackable.Pos[1]-lastPos[i].y) / time );

	  currentTrackable.motionVector[0] = alpha *  motionVector.x + (1-alpha) * currentTrackable.motionVector[0];
	  currentTrackable.motionVector[1] = alpha *  motionVector.y + (1-alpha) * currentTrackable.motionVector[1];
	  if ( !isnan(currentTrackable.Pos[2]) )
	    currentTrackable.motionVector[2] = alpha * motionVector.z + (1-alpha) * currentTrackable.motionVector[2];
	  
	  limitSpeed( currentTrackable, 1.0 ); // limit speed to 1m/s
	}
	else if ( predictWeight > 0.0 ) /* if unused move by MotionVector */
        {
	  double alpha = 0.0;
	  if ( keepTime > 0 ) // slow down during keep time
	    alpha = (1.0 - (now - currentTrackable.lastTime) / (double) keepTime) * predictWeight;
	  
	  currentTrackable.Pos[0] += alpha * currentTrackable.motionVector[0];
	  currentTrackable.Pos[1] += alpha * currentTrackable.motionVector[1];
	  if ( !isnan(currentTrackable.Pos[2]) )
	    currentTrackable.Pos[2] += alpha * currentTrackable.motionVector[2];
	}
      }
      else
      {
	currentTrackable.motionVector[0] = 0;
	currentTrackable.motionVector[1] = 0;
	currentTrackable.motionVector[2] = 0;
      }
    }
    
	/* remove unused current trackables if they are old */

    for ( int i = ((int)current.size())-1; i >= 0; --i )
    { Trackable<Type> &currentTrackable( *current[i] );

      currentTrackable.setTouched( currentMap[i] >= 0 );
      
      if ( currentMap[i] < 0 )
      { 
	uint64_t tdiff = now - currentTrackable.lastTime;
	if ( tdiff >= keepTime ) /* drop if to old */
	{ if ( !isInPortal( currentTrackable ) )
	    putLatentId( i, latentDistance, time, timestamp );
	  
	  currentTrackable.erasable = true;
	  
//	  current.erase( current.begin()+i );
	}
      }
    }
    
	/* add new trackable with no correspondence */
    for ( int i = ((int)merged.size())-1; i >= 0; --i )
    { 
      if ( mergedMap[i] < 0 && !merged[i]->erasable )
      {
	Trackable<Type> &mergedTrackable = *merged[i];
	
	mergedTrackable.init( timestamp, false );
	mergedTrackable.setTouched( true );

	    /* extend time until activation when something activated is closeby */
	for ( int t = 0; t < trackInfo.size() && trackInfo[t].distance < uniteDistance; ++t )
        { TrackInfo &ti( trackInfo[t] );
	  if ( i == ti.mergedIndex && current[ti.currentIndex]->isActivated )
          { mergedTrackable.firstTime = now;
	    break;
	  }
	}
	current.push_back( merged[i] );
      }
    }
    
	/* remove unused current trackables if they are old */

    for ( int i = ((int)current.size())-1; i >= 0; --i )
      if ( current[i]->erasable )
	current.erase( current.begin()+i );

    latest.resize( 0 );


	/* smoothing */
    double sms = trackSmoothing;
    double oms = 1.0 - sms;
    
    double smsSize = 1.0 - (1.0 - trackSmoothing) * 0.6;
    double omsSize = 1.0 - smsSize;
    
    for ( int i = 0; i < current.size(); ++i )
    { Trackable<Type> &currentTrackable( *current[i] );

      Vector3D p( currentTrackable.p[0] - currentTrackable.Pos[0],
		  currentTrackable.p[1] - currentTrackable.Pos[1],
		  isnan(currentTrackable.Pos[2]) ? NAN : currentTrackable.p[2] - currentTrackable.Pos[2] );
      
      float distance = length( &p.x );
      if ( isValidSpeed( time_diff, distance ) )
      {
	currentTrackable.p[0] = sms * currentTrackable.p[0] + oms * currentTrackable.Pos[0];
	currentTrackable.p[1] = sms * currentTrackable.p[1] + oms * currentTrackable.Pos[1];
	if ( !isnan(currentTrackable.Pos[2]) )
	  currentTrackable.p[2] = sms * currentTrackable.p[2] + oms * currentTrackable.Pos[2];

	currentTrackable.size = smsSize * currentTrackable.size + omsSize * currentTrackable.Size;
      }
      else
      {
	currentTrackable.p[0] = currentTrackable.Pos[0];
	currentTrackable.p[1] = currentTrackable.Pos[1];
	currentTrackable.p[2] = currentTrackable.Pos[2];
	currentTrackable.size = currentTrackable.Size;
      }
    }
    
    	/* transfer trackables with activity */
    for ( int i = 0; i < current.size(); ++i )
    { 
      Trackable<Type> &currentTrackable = *current[i];
      bool erased = false;
      
      if ( !currentTrackable.isActivated )
      { if ( now - currentTrackable.lastTime > minActiveTime * minActiveFraction ) /* drop if not active */
	{ current.erase( current.begin()+i );
	  i -= 1;
	  erased = true;
	}
	else if ( currentTrackable.isTouched() && now - currentTrackable.firstTime > minActiveTime ) /* activate if old enough */
	{
	  std::string latentId;
	  uint64_t ts;
	  
	  if ( !isInPortal( currentTrackable ) )
	    latentId = getLatentId( i, latentDistance, time, ts );
	    
	  if ( !latentId.empty() )
	  { 
#if 0
	    time_t t = timestamp / 1000;
	    struct tm timeinfo = *localtime( &t );

	    const int maxLen = 2000;
	    char buffer[maxLen+1];
	    strftime( buffer, maxLen, "%c", &timeinfo );

	    printf( "[%ld.%03ld] %s: reassigning latentId: -> %s after %g sec\n", (long)(timestamp/1000), (long)(timestamp%1000), buffer, latentId.c_str(), (timestamp-ts)/1000.0 );
#endif
	    currentTrackable.setId( latentId, starttime );
	  }
	  else
	    currentTrackable.id( starttime );
	  
	  currentTrackable.isActivated = true;
	  currentTrackable.firstImmobilePos[0] = currentTrackable.Pos[0];
	  currentTrackable.firstImmobilePos[1] = currentTrackable.Pos[1];
	  currentTrackable.firstImmobilePos[2] = currentTrackable.Pos[2];
	}
      }

      if ( !erased && currentTrackable.isActivated )
      {    /* remove masked Trackables */
	if ( trackableMask == NULL )
	  latest.push_back( current[i] );
	else
	{
	  int maskBits = trackableMask( currentTrackable );

	  if ( !(maskBits&Trackable<Type>::Occluded) )
	  {
	    currentTrackable.touchPrivate( maskBits & Trackable<Type>::Private, timestamp, privateTimeout );

	    currentTrackable.setPortal( maskBits & Trackable<Type>::Portal );

	    currentTrackable.checkImmobile( timestamp, immobileTimeout, immobileDistance );

	    latest.push_back( current[i] );
	  }
	}
      }
    }
    
    this->finish( timestamp );
    this->current->cleanup( latentLifeTime, timestamp, time_diff ); // keep related for 10 sec

#ifdef TRACKABLE_OBSERVER_H
    if ( observer != NULL )
    {    
      obsvObjects.clear();
      obsvObjects.timestamp = timestamp;
      obsvObjects.uuid	    = uuid;
      obsvObjects.frame_id  = this->frame_count;
      
      int validCount = 0;
      for ( int i = 0; i < current.size(); ++i )
      { 
	Trackable<Type> &trackable = *current[i];
	if ( trackable.isActivated )
	{ 
	  int maskBits = 0;

	  if ( trackableMask == NULL || !((maskBits=trackableMask(trackable))&Trackable<Type>::Occluded) )
	  {
	    ObsvObject obsvObject;
	    trackable.getObservInfo( obsvObject );

	    obsvObject.id        	= std::atoi( trackable.id().c_str() );
	    obsvObject.uuid      	= trackable.uuid;
	    obsvObject.timestamp 	= timestamp;
	    obsvObject.flags     	= trackable.flags;
	    obsvObjects.emplace(std::make_pair(obsvObject.id,obsvObject));

	    for ( LatentIds::iterator iter = trackable.latentIds.begin(); iter != trackable.latentIds.end(); ++iter )
	    {
	      ObsvObject latentObject( obsvObject );
	      latentObject.setLatent( true );

	      const std::string &id( iter->first );
	      latentObject.id   = std::atoi( id.c_str() );
	      latentObject.uuid = iter->second.uuid;

	      obsvObjects.emplace(std::make_pair(latentObject.id,latentObject));
	    }
	  }
	}
      }
      obsvObjects.validCount = validCount;

      observer->observe( obsvObjects );
      obsvObjects.update();
    }
#endif
  }
  

  void	clear()
  {
    Trackables<Type> &current( *this->current );

#ifdef TRACKABLE_OBSERVER_H
    if ( observer != NULL )
    { obsvObjects.clear();
      observer->observe( obsvObjects, true );
      obsvObjects.update();
    }
#endif
    current.resize( 0 );
  }

#ifdef TRACKABLE_OBSERVER_H
  void	addObserver( TrackableObserver *observer )
  {
    if ( this->observer == NULL )
      this->observer = new TrackableMultiObserver();

    this->observer->addObserver( observer );
  }

  TrackableObserver *getObserver( const char *name )
  {
    if ( this->observer == NULL )
      return NULL;

    for ( int i = ((int)observer->observer.size())-1; i >= 0; --i )
      if ( observer->observer[i]->name == name )
	return observer->observer[i];

    return NULL;
  }
  
  void removeObserver( const char *name, bool deleteIt=false )
  {
    for ( int i = ((int)observer->observer.size())-1; i >= 0; --i )
      if ( observer->observer[i]->name == name )
	observer->removeObserver( i, deleteIt );
  }
    
#endif
};

/***************************************************************************
*** 
*** TrackableReader
***
****************************************************************************/

template<typename Type> class TrackableReader
{
public:
  typedef std::shared_ptr<TrackableReader> Ptr;

  bool 						verbose;
  typename TrackableMultiStage<Type>::Ptr  	stage;
  int						worldMarkerId;
  
  TrackableReader()
    : verbose( true ),
      stage  ( new TrackableMultiStage<Type>() ),
      worldMarkerId( -1 )   
  {}

  TrackableStage<Type> &getStage( const char *stageId, bool createIfMissing=false )
  { 
    return stage->getStage( stageId, createIfMissing );
  }
  
  virtual bool parseArg( int &i, const char *argv[], int &argc ) 
  { return this->stage->parseArg( i, argv, argc );
  }
  
  virtual bool parseBuffer( const char *buffer, size_t size, std::string &stageId ) = 0;
};
  
  
template<typename Type> class TrackableJsonReader : public TrackableReader<Type>
{
public:
  virtual bool parseJson( TrackableStage<Type> &stage,
#if USE_CAMERA
			  imCamera *camera,
#endif
			  rapidjson::Value &json ) = 0;

  virtual bool parseBuffer( const char *buffer, size_t size, std::string &stageId )
  {
    rapidjson::Document document;
    if ( document.Parse( buffer ).HasParseError() ||
	 !document.IsObject() )
    {
      std::cout << "error parsing document\n";
      return false;
    }

//    rapidjson::dump( document );
	  
#if USE_CAMERA
    if ( document.HasMember("camera") )
    {
      imCamera camera, *cam;
      
      if ( camera.fromJson( document["camera"] ) )
      {
	if ( !camera.identifier().empty() && (cam=this->stage->cameras.getByIdentifier( camera.identifier().c_str())) == NULL )
        { cam  = this->stage->cameras.getNew();
	  *cam = camera;
	  stageId = camera.identifier();

	  this->getStage( camera.identifier().c_str(), true );
	}

	if ( this->worldMarkerId >= 0 )
	  this->stage->cameras.setWorldMarker( this->worldMarkerId );
      }
    }
#endif
    TrackableStage<Type> &stage( this->getStage( stageId.c_str() ) );
    
#if USE_CAMERA
    imCamera *camera = this->stage->cameras.getByIdentifier( stageId.c_str() );
    this->parseJson( stage, camera, document );
#else
    this->parseJson( stage, document );
#endif

    return true;
  }
};


};


#endif // TRACKABLE_H

