// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef _TRACK_BASE_H_
#define _TRACK_BASE_H_

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#include <set>

#include "filterTool.h"
#include "BlobMarkerUnionTrackable.h"

/***************************************************************************
*** 
*** DECLARATIONS
***
****************************************************************************/

using namespace pv;

/***************************************************************************
*** 
*** TrackableRegion
***
****************************************************************************/

enum RegionShape
{
  RegionShapeRect     = 0,
  RegionShapeEllipse  = 1
};

enum RegionEdge
{
  RegionEdgeNone      = 0,
  RegionEdgeLeft      = 1,
  RegionEdgeRight     = 2,
  RegionEdgeTop       = 3,
  RegionEdgeBottom    = 4
};


class TrackableRegion
{
public:
  KeyValueMap descr;
  
  std::string name;
  float x, y, width, height;
  int	count;
  int	edge;
  int	shape;
  std::string	usedByObserver;
  std::string   layersStr;
  std::string	tagsStr;
  
  std::set<std::string>	layers;
  std::set<std::string>	tags;

  TrackableRegion( float x=0.0, float y=0.0, float width=6.0, float height=6.0, const char *name=NULL )
    : x     ( x ),
      y     ( y ),
      width ( width ),
      height( height ),
      name  ( name == NULL ? "" : name ),
      count ( -1 ),
      edge  ( RegionEdgeNone ),
      shape ( RegionShapeRect )
    {}

  TrackableRegion( const char *name )
    : x     (  0.0 ),
      y     (  0.0 ),
      width (  6.0 ),
      height(  6.0 ),
      name  ( name == NULL ? "" : name ),
      count ( -1 ),
      edge  ( RegionEdgeNone ),
      shape ( RegionShapeRect )
    {}
  
  inline float x1() const { return x - width*0.5;  }
  inline float y1() const { return y - height*0.5; }
  inline float x2() const { return x + width*0.5;  }
  inline float y2() const { return y + height*0.5; }
  
  inline bool hasLayer( const std::string &layer )
  { return (layer.empty() && layers.size() == 0) || layers.find( layer ) != layers.end();
  }

  inline bool hasTag( const std::string &tag )
  { return tags.find( tag ) != tags.end();
  }

  inline bool contains( float x, float y, float size=0.0f ) const
  {
    if ( shape == RegionShapeRect )
    { size *= 0.5;
      return x+size >= this->x1() &&
             x-size <= this->x2() &&
             y+size >= this->y1() &&
             y-size <= this->y2();
    }
    
    x -= this->x;
    y -= this->y;    
    y *= this->width / this->height;

    return sqrt( x*x + y*y ) <= 0.5 * this->width;
  }

  void setCommaList( std::set<std::string> &set, const char *str );
  void setTags  ( const char *str );
  void setLayers( const char *str );

  bool setKeyValueMap ( KeyValueMap &descr );
  bool fromKeyValueMap( KeyValueMap &descr );
  bool toKeyValueMap  ( KeyValueMap &descr );

  static RegionEdge regionEdgeByString( const char *edgeName );
  const std::string regionEdge_str( RegionEdge defaultRegionEdge );

  static RegionShape regionShapeByString( const char *shapeName );
  static const std::string regionShape_str( RegionShape defaultRegionShape );
};
 
/***************************************************************************
*** 
*** TrackableRegions
***
****************************************************************************/

class TrackableRegions : public std::vector<TrackableRegion>
{
public:
  
  std::string name;
  std::set<std::string> tags;
  std::set<std::string> layers;
  

  TrackableRegions()
    : name  ( "region" )
    {}

  TrackableRegion   *get( const char *name, bool create=false );
  TrackableRegion   &add( float x, float y, float width, float height, const char *name=NULL );
  TrackableRegion   &add( const char *name=NULL );
  void			set      ( const char *name, float x, float y, float width, float height );
  void			setEdge  ( const char *name, int edge );
  void			setEdge  ( const char *name, const char *edge );
  void			setShape ( const char *name, int shape );
  void			setShape ( const char *name, const char *shape );
  void 			setTags  ( const char *name, const char *tags );
  void 			setLayers( const char *name, const char *layers );
  void			remove   ( const char *name );
  void			rename   ( const char *name, const char *newName );
  
  std::set<TrackableRegion *> getByNameOrTag( const char *name );
  std::set<TrackableRegion *> getByLayer    ( const char *layer );

  bool writeToFile ( const char *fileName );
  bool readFromFile( const char *fileName );

  std::set<std::string> collectTags();
  std::set<std::string> collectLayers();
  
};
 
/***************************************************************************
*** 
*** TrackGlobal
***
****************************************************************************/

class TrackGlobal
{
public:

    enum CheckPointMode {

      NoCheckPoint           = 0,
      ReadCheckPoint         = (1<<0),
      WriteCheckPoint        = (1<<1),
      CreateCheckPoint       = (1<<2),
      WriteCreateCheckPoint  = (WriteCheckPoint|CreateCheckPoint)
  
    };

    static KeyValueMap			defaults;
    static KeyValueMapDB		observers;
    static TrackableRegions		regions;
    static std::string			defaultsFileName;
    static std::string			observerFileName;
    static std::string			regionsFileName;
    static std::vector<std::string> 	regionEdgeLabel;
    
    static bool WriteKeyValues( KeyValueMap &map, const char *fileName );
    static bool ReadKeyValues( KeyValueMap &map, const char *fileName, bool reportError=true );
    static bool WriteKeyValueMapDB( KeyValueMapDB &map, const char *fileName, const char *key="name", const char *mapName="map" );
    static bool ReadKeyValueMapDB( KeyValueMapDB &map, const char *fileName, const char *key="name", const char *mapName="map" );

    static bool writeDefaults();
    static bool readDefaults();
    static bool getDefault( const char *key, std::string &value );
    static bool getDefault( const char *key, int &value );
    static bool	getDefault( const char *key, bool &value );

    static bool setDefault( const char *key, const char *value );
    static bool removeDefault( const char *key );
    
    static bool writeObservers();
    static bool readObservers();
    static void setObserverValue( const char *name, const char *key, const char *value );
    static void removeObserverValue( const char *name, const char *key );
    static void removeObserver( const char *name );
    static void renameObserver( const char *name, const char*newName );

    static bool	loadRegions();
    static bool	saveRegions();

    static int  verbose();
    static void setVerbose( int level );

    static bool setDefaults( int &argc, const char **&argv );
    static bool parseDefaults( int &argc, const char **&argv );
    static bool parseArg( int &i, const char *argv[], int &argc );

    static std::string configFileName( std::string fileName );

    static std::string configDir;

    static void (*error)		( const char *format, ... );
    static void (*warning)		( const char *format, ... );
    static void (*log)  		( const char *format, ... );
    static void (*info)			( const char *format, ... );
    static void (*notification)  	( const char *tags, const char *format, ... );

    static void setErrorFileName     ( const char *fileName );
    static void setLogFileName       ( const char *fileName );
    static void setNotificationScript( const char *scriptFileName );
    
    static void printConfig	     ();

    static void setReadCheckPoint    ( const char *checkPoint );
    static std::string  getConfigFileName( const char *fileName, const char *suffix=NULL, const char *path=NULL, CheckPointMode checkPointMode=NoCheckPoint, uint64_t timestamp=0 );
    static void catchSigPipe();

};

/***************************************************************************
*** 
*** TrackBase
***
****************************************************************************/

class PackedPlayer;

class TrackBase : public TrackGlobal
{
public:

    enum UniteMethod
    {
      UniteStages  = 0,
      UniteBlobs   = 1,
      UniteObjects = 2

    };

    TrackableMultiStage<BlobMarkerUnion>::Ptr     m_Stage;
    UniteMethod					  uniteMethod;

    float					  imageSpaceResolution;
    float					  logDistance;
    
    std::string 				  logFilter;

    TrackBase();
    
    virtual void	reset();

    virtual void 	markUsedRegions();
    
    virtual bool	addObserver       ( KeyValueMap &descr );
    virtual bool	addObserver       ( TrackableObserver *observer );
    virtual void	finishObserver    ();
    virtual bool	updateObserverRegion( const char *name );
    virtual void	setObserverRegion ( TrackableObserver *observer, const char *regionName );
    virtual void 	setObserverParam  ( TrackableObserver *observer, KeyValueMap &descr );
    virtual void	registerObserverCreator( const char *type, TrackableObserverCreator creator );

    static int64_t  	packedPlayerCurrentTime();
    static uint64_t 	packedPlayerTimeStamp();
    static float    	packedPlayerPlayPos();
    static void     	setPackedPlayerPlayPos( float playPos );
    static void     	setPackedPlayerSyncTime( uint64_t timestamp=0 );
    static void     	setPackedPlayerPaused( bool paused );
    static bool     	packedPlayerIsPaused();
    static bool     	packedPlayerAtEnd();

    static PackedPlayer *packedPlayer();
    static void		 setPackedPlayer( PackedPlayer *packedPlayer );
           void		 packedPlayerTrack( uint64_t timestamp=0, bool waitForFrame=false );
           void		 trackObjects     ( ObsvObjects &objects );

           void		 observe 	  ( PackedTrackable::Header      &header );
           void		 observe 	  ( PackedTrackable::BinaryFrame &frame );
	   
};




#endif

