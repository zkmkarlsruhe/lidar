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

#include "lidarTrack.h"
#include "packedPlayer.h"

#include "TrackBase.cpp"

/***************************************************************************
*** 
*** DEFINES
***
****************************************************************************/

/***************************************************************************
*** 
*** GLOBALS
***
****************************************************************************/

using namespace rapidjson;

static float g_RadialDisplacement = 0.0;

/***************************************************************************
*** 
*** TrackBase
***
****************************************************************************/

/***************************************************************************
*** 
*** TrackableLogger
***
****************************************************************************/

/***************************************************************************
*** 
*** LidarTrack
***
****************************************************************************/

LidarTrack::LidarTrack()
  : TrackBase()
{
}

void
LidarTrack::exit()
{
  if ( m_Stage != NULL )
  { m_Stage = NULL;
//  delete &*m_Stage;
  }
}
void
LidarTrack::setRadialDisplacement( float displace )
{
  g_RadialDisplacement = displace;
}

void
LidarTrack::start( uint64_t timestamp, LidarDevice *device )
{
  if ( device == NULL )
  { 
    for ( int i = ((int)regions.size())-1; i >= 0; --i )
      regions[i].count = -1;

    m_Stage->start( timestamp );
  }
}

void
LidarTrack::stop( uint64_t timestamp, LidarDevice *device )
{
  if ( device != NULL )
  {
    TrackableStage<BlobMarkerUnion> &stage( m_Stage->getStage( device->baseName.c_str(), true ) );
    
    stage.swap();
    stage.swap();
  }
  else
    m_Stage->stop( timestamp );
}


void
LidarTrack::startAlwaysObserver( uint64_t timestamp )
{
  m_Stage->startAlwaysObserver( timestamp );
}

void
LidarTrack::updateOperational( std::set<std::string> &availableDevices, TrackableObserver *observer )
{
  if ( observer == NULL )
  {
    TrackableMultiObserver *observer = static_cast<TrackableMultiObserver*>( m_Stage->observer );
    if ( observer == NULL )
      return;
    
    for ( int i = ((int)observer->observer.size())-1; i >= 0; --i )
      updateOperational( availableDevices, observer->observer[i] );

    return;
  }

  std::vector<std::string> &requestedDevices( observer->operationalDevices );
  if ( requestedDevices.size() == 0 )
    return;

  for ( int i = observer->rects.numRects()-1; i >= 0; --i )
  {
    ObsvRect &rect( observer->rects.rect(i) );
    ObsvObjects &objects( rect.objects );

    float numNvailable = 0;
    
    for ( int d = ((int)requestedDevices.size())-1; d >= 0; --d )
    { if ( availableDevices.count( requestedDevices[d] ) > 0 )
	numNvailable += 1;
    }

    objects.operational = numNvailable / requestedDevices.size();
  }
}


static Trackable<BlobMarkerUnion>::Ptr
createTrackable( TrackableStage<BlobMarkerUnion> &stage, uint64_t timestamp, float x, float y, float size )
{
  Trackable<BlobMarkerUnion>::Ptr trackable = stage.newTrackable( timestamp );
  trackable->type  = BlobMarkerUnion::Blob;
      
  trackable->p[0] = x;
  trackable->p[1] = y;
  trackable->p[2] = NAN;
  trackable->size = size;

  trackable->init( timestamp );

#if USE_MARKER
  trackable->matrix[3][0] = trackable->p[0];
  trackable->matrix[3][1] = trackable->p[1];
  trackable->matrix[3][2] = trackable->p[2];
#endif

  return trackable;
}


static void
addObjectToStage( TrackableStage<BlobMarkerUnion> &stage, LidarObject &object, uint64_t timestamp )
{
  createTrackable( stage, timestamp, object.center.x, object.center.y, object.extent );
}

static void
addToStage( LidarDevice *device, TrackableStage<BlobMarkerUnion> &stage, uint64_t timestamp )
{
  if ( !device->isOpen() || device->isEnvScanning || !device->dataValid )
    return;
  
  device->lock();

  for ( int oi = device->numDetectedObjects()-1; oi >= 0; --oi )
  { 
    LidarObject &object( device->detectedObject( oi ) );
    addObjectToStage( stage, object, timestamp );
  }
      
  device->unlock();
}


void
LidarTrack::mergeStages( LidarDevices &devices, uint64_t timestamp )
{
  for ( int i = 0; i < devices.size(); ++i )
  {
    LidarDevice *device = devices[i];
    
    TrackableStage<BlobMarkerUnion> &stage( m_Stage->getStage( device->getNikName().c_str(), true ) );
    
    addToStage( device, stage, timestamp );
    
    stage.finish( timestamp );
    stage.swap();
  }
}

static void
addToObjects( LidarDevice *device, LidarObjects &objects, int user )
{
  if ( !device->isOpen() || device->isEnvScanning || !device->dataValid )
    return;
  
  device->lock();

  for ( int oi = device->numDetectedObjects()-1; oi >= 0; --oi )
  { 
    LidarObject &object( device->detectedObject( oi ) );

    objects.push_back( LidarObject(object) );
    LidarObject &obj( objects.back() );
    obj.oid  = 0;
    obj.user = user;
      
    if ( g_RadialDisplacement != 0.0 )
    { Vector3D offset( obj.center - device->matrix.w );
      offset.normalize();
      obj.normal = offset;

      offset *= g_RadialDisplacement - obj.closest;
      obj.center += offset;
    }
  }
      
  device->unlock();
}

struct TrackInfo
{
  double distance;
  int    currentIndex;
  int    mergedIndex;
};

static bool
compareTrackInfo( const TrackInfo& t1, const TrackInfo& t2 )
{ return t1.distance < t2.distance; }


static float
confidence( LidarObject &object )
{
  const float addConf = 0.4;
  const float mixConf = 1.0 - addConf;

  return  addConf * (object.personSized + object.curvature) + mixConf * (object.personSized * object.curvature);
}

static void
adjustBoundingBox( const Vector3D &p, Vector2D &min, Vector2D &max )
{
  if ( p.x < min.x ) min.x = p.x;
  if ( p.x > max.x ) max.x = p.x;
  
  if ( p.y < min.y ) min.y = p.y;
  if ( p.y > max.y ) max.y = p.y;
}


static float
objTimeOffset( const LidarObject &obj0, const LidarObject &obj1 )
{
  int64_t time_diff = abs( (int)(obj0.timeStamp - obj1.timeStamp) );

  if ( time_diff < 250 )
  { const float maxSpeed = 4.0; // m/sec
    return maxSpeed * time_diff / 1000.0;
  }
      
  return 0.0;
}

static float
maxTimeOffset( const std::vector<LidarObject*> &objs )
{
  float timeOffset = 0.0;

  for ( int i = ((int)objs.size())-1; i > 0; --i )
  { for ( int j = i-1; j >= 0; --j )
    { float offset = objTimeOffset( *objs[i], *objs[j] );
      if ( offset > timeOffset )
	timeOffset = offset;
    }
  }
  
  return timeOffset;
}

static void addObjectsToMerged( TrackableStage<BlobMarkerUnion> &stage, std::vector<LidarObject*> &objs, Trackables<BlobMarkerUnion> &merged, float objectMaxSize, uint64_t timestamp );


static bool
isHullObjs( const std::vector<LidarObject*> &objs, Vector3D &center )
{
  for ( int i = ((int)objs.size())-1; i >= 0; --i )
  { 
    LidarObject &obj( *objs[i] );
    
    std::vector<Vector2D> &curvePoints( obj.curvePoints );
    
    for ( int i = 1; i < curvePoints.size(); ++i )
    {
      float a1 = (curvePoints[i-1] - curvePoints[i]).angle( (*(Vector2D*)&obj.center)-curvePoints[i] );
      float a2 = (curvePoints[i-1] - curvePoints[i]).angle( (*(Vector2D*)&center)    -curvePoints[i] );
   
      if ( signbit(a1) != signbit(a2) )
	return false;
    }
  }
  
  return true;
}


static float
objsMeanSquare( const std::vector<LidarObject*> &objs, Vector3D &center, float confidenceWeight=0.2f )
{
  for ( int i = ((int)objs.size())-1; i >= 0; --i )
    center += objs[i]->center;
   
  center /= objs.size();
  
  float sum = 0;
  const float   cw =  confidenceWeight;
  const float omcw = 1.0 - cw;
  
  for ( int i = ((int)objs.size())-1; i >= 0; --i )
  { float dist = center.distance( objs[i]->center );

    dist *= omcw + objs[i]->confidence * cw;
    dist += 1;
    sum  += dist * dist;
  }
  
  float meanSquare = sum;

  return meanSquare;
}


static float
splitObjectsToMerged( TrackableStage<BlobMarkerUnion> &stage, std::vector<LidarObject*> &objs, Trackables<BlobMarkerUnion> &merged, float objectMaxSize, float objectSize, uint64_t timestamp )
{
  float dmax = 0.0;
  int oi1, oi2;
  for ( int o1 = ((int)objs.size())-1; o1 > 0; --o1 )
  { for ( int o2 = o1-1; o2 >= 0; --o2 )
    { double d = objs[o1]->center.distance( objs[o2]->center );
      if ( d > dmax )
      { oi1  = o1;
	oi2  = o2;
	dmax = d;
      }
    }
  }
   
  std::vector<LidarObject*> objs1, objs2;

  for ( int i = ((int)objs.size())-1; i >= 0; --i )
  {
    double d1 = objs[oi1]->center.distance( objs[i]->center );
    double d2 = objs[oi2]->center.distance( objs[i]->center );
	
    if ( d1 < d2 )
      objs1.push_back( objs[i] );
    else
      objs2.push_back( objs[i] );
  }

  bool isHull1 = false;
  bool isHull2 = false;
  
  Vector3D center, center1, center2;
    
  float ms  = objsMeanSquare( objs,  center ) / objs.size();
  float ms1 = objsMeanSquare( objs1, center1 );
  float ms2 = objsMeanSquare( objs2, center2 );
  float msa = (ms1 + ms2) / (objs1.size() + objs2.size());
  
  float msf = msa / ms;

  if ( msf >= 1 )
  { isHull1 = isHullObjs( objs1, center2 );  
    if ( !isHull1 )
      isHull2 = isHullObjs( objs2, center1 );
  }

  if ( msf < 1 || isHull1 || isHull2 )
  {
    addObjectsToMerged( stage, objs1, merged, objectMaxSize, timestamp );
    addObjectsToMerged( stage, objs2, merged, objectMaxSize, timestamp );

    return 1.0;
  }

  return 1.0 / msf;
}

static void
addObjectsToMerged( TrackableStage<BlobMarkerUnion> &stage, std::vector<LidarObject*> &objs, Trackables<BlobMarkerUnion> &merged, float objectMaxSize, uint64_t timestamp )
{
	/* compute center and size */
      
  Vector2D min(  10000,  10000 );
  Vector2D max( -10000, -10000 );

  for ( int o = ((int)objs.size())-1; o >= 0; --o )
  { adjustBoundingBox( objs[o]->center,      min, max );
    adjustBoundingBox( objs[o]->lowerCoord,  min, max );
    adjustBoundingBox( objs[o]->higherCoord, min, max );
  }

  Vector2D center = (min+max) * 0.5;
  double size = 0.0;
      
  for ( int o = ((int)objs.size())-1; o >= 0; --o )
  { size = std::max( size, center.distance( *((Vector2D*)&(objs[o]->center)) ) );
    size = std::max( size, center.distance( *((Vector2D*)&(objs[o]->lowerCoord)) ) );
    size = std::max( size, center.distance( *((Vector2D*)&(objs[o]->higherCoord)) ) );
  }

  const float objSize = 2.0 * size;

  float splitProb = 0.0;
  
//	printf( "c<onfDistance: %g\n", confDistance );

  if ( objectMaxSize > 0.0 && objs.size() > 1 ) // if to big, split into two
  {
//    printf( "split Trackable\n" );

    float timeDiff = maxTimeOffset( objs );
    float maxSize  = objectMaxSize + timeDiff;
    if ( objSize > maxSize )
    { splitProb = splitObjectsToMerged( stage, objs, merged, maxSize, objSize, timestamp );
      if ( splitProb == 1.0 )
	return;
    }
  }

  merged.push_back( createTrackable(stage, timestamp, center.x, center.y, objSize ) );
  Trackable<BlobMarkerUnion>::Ptr trackable = merged.back();
  trackable->user2     = objs[0]->user;
  trackable->splitProb = splitProb;
}


void
LidarTrack::mergeObjects( LidarDevices &devices, uint64_t timestamp )
{
  LidarObjects objects;

  for ( int i = 0; i < devices.size(); ++i )
    addToObjects( devices[i], objects, i );
    
  TrackableStage<BlobMarkerUnion>   &stage( m_Stage->getStage( "single", true ) );

  int  mixedIndex[objects.size()];
  std::vector<LidarObject*> mergedObjects[objects.size()];

  const float maxPersonSize = 0.7;
  const float minPersonSize = 0.3;
  const float medPersonSize = 0.5 * (maxPersonSize + minPersonSize);
  const float personRange   = maxPersonSize - minPersonSize;

  for ( int i = ((int)objects.size())-1; i >= 0; --i )
  { mixedIndex[i] = -1;

    LidarObject &obj( objects[i] );

    double diff = fabsf( medPersonSize-obj.extent );
    diff /= 0.5 * personRange;
    if ( diff > 1.0 )
      diff = 1.0;
    else
      diff *= diff;
    
    obj.personSized = 1.0 - diff;
    obj.confidence  = confidence( obj );

    mergedObjects[i].push_back( &obj );
  }

/****				            ****/
/****		merge  close objects 	    ****/
/****				            ****/

  const double uniteDistance   = m_Stage->uniteDistance;
  const double confWeight      = 0.8;
  const double splitWeight     = 1.0;
  
      /* calculate min distances between objects */

  std::vector<TrackInfo> trackInfo;
  for ( int i = ((int)objects.size())-1; i > 0; --i )
  { 
    LidarObject &obj( objects[i] );
    
    double obj0Weight = 1.0 - obj.confidence;
    
    for ( int j = i-1; j >= 0; --j )
    { 
      LidarObject &obj1( objects[j] );
      double d  = obj.center.distance( obj1.center );
      
      double obj1Weight = 1.0 - obj1.confidence;
    
      double weight = 1.0 + 0.5 * (obj0Weight + obj1Weight) * confWeight;

      if ( obj.isSplit || obj1.isSplit )
      {
	if ( obj.user == obj1.user )
	  d = 10000.0;
	else
        {
	  weight += 0.5 * ((obj0Weight + obj1Weight) + 0.5*(obj.isSplit&&obj1.isSplit)) * splitWeight;
	}
      }

//      printf( "weight: %g  %g\n", weight, uniteDistance/weight );
      
      d *= weight;

      float timeOffset  = objTimeOffset( obj, obj1 );
      
      if ( d <= uniteDistance + timeOffset )
	trackInfo.push_back( TrackInfo({d, i, j}) );
    }
  }
      /* sort by distances */
  sort( trackInfo.begin(), trackInfo.end(), compareTrackInfo );

      /* correlate objects with min distance */
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

	  /* if they are not the same object */
      if ( mergedIndex != currentIndex )
      {
	LidarObject *cobj( &objects[currentIndex] );
	    /* remember objects and mix info */
	mergedObjects[mergedIndex].push_back( cobj );

	mixedIndex[currentIndex] = mergedIndex;
      }
    }
  }

/****				            ****/
/****  create trackables for merged object  ****/
/****				            ****/

  float objectMaxSize = m_Stage->objectMaxSize;

  Trackables<BlobMarkerUnion> &merged( *new Trackables<BlobMarkerUnion>() );

  for ( int i = ((int)objects.size())-1; i >= 0; --i )
  {
    if ( mixedIndex[i] < 0 && mergedObjects[i].size() > 0 )	/* add used trackables */
      addObjectsToMerged( stage, mergedObjects[i], merged, objectMaxSize, timestamp );
  }

  stage.finish( timestamp );
  stage.latest = Trackables<BlobMarkerUnion>::Ptr( &merged );

  Trackables<BlobMarkerUnion> &current( *new Trackables<BlobMarkerUnion>() );

  for ( int i = ((int)objects.size())-1; i >= 0; --i )
  {
    LidarObject &object( objects[i] );
    

    current.push_back( Trackable<BlobMarkerUnion>::Ptr( stage.createTrackable()) );
    Trackable<BlobMarkerUnion>::Ptr trackable = current.back();
//    trackable->touchTime( timestamp );

    trackable->type	  = BlobMarkerUnion::Blob;
      
    trackable->p[0]  	  = object.center.x;
    trackable->p[1]  	  = object.center.y;
    trackable->p[2]  	  = NAN;
    trackable->size  	  = object.extent;
    trackable->init( timestamp );

    trackable->user2 	  = object.user;
    trackable->user3 	  = object.curvature;
    trackable->user4      = object.personSized;
    trackable->user5      = object.extent;
    trackable->confidence = confidence( object );
  }

  stage.lockCurrent();
  stage.current = Trackables<BlobMarkerUnion>::Ptr( &current );
  stage.unlockCurrent();
  
  m_Stage->unite( timestamp );
}

void
LidarTrack::track( LidarDevices &devices, uint64_t timestamp )
{
  if ( timestamp == 0 )
    timestamp = getmsec();

  if ( packedPlayer() != NULL )
  {
    packedPlayerTrack( timestamp, false );
  }
  else
  {
    if ( uniteMethod == UniteObjects )
    { m_Stage->uniteInSingleStage = false;
      mergeObjects( devices, timestamp );
    }
    else
    {
      m_Stage->uniteInSingleStage = (uniteMethod == UniteBlobs);
      mergeStages( devices, timestamp );
      m_Stage->unite( timestamp );
    }
  }
}


