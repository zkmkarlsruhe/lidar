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


#include "packedPlayer.h"

#include "lidarTrack.h"
#include "TrackBase.cpp"

#include <set>

#define cimg_use_jpeg
#define cimg_use_png
#include "CImg/CImg.h"

typedef cimg_library::CImg<unsigned char> rpImg;

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

static uint64_t				firstTimeStamp     = 0;
static uint64_t				lastTimeStamp      = 0;
static int				numFrames          = 0;
static uint64_t				frameTimeSum       = 0;
static uint64_t				maxFrameTime       = 0;
static uint64_t				minFrameTime       = 0;
static int				numPrivates        = 0;
static int				numImmobiles       = 0;
static int				numDrops           = 0;
static int				numKeeps           = 0;
static int				numStarts          = 0;
static int				numStops           = 0;

static uint64_t				startStopPauseTime = 120000;
static int64_t				dropLifeSpan       = 0;
static int64_t				privateTimeout     = 5000;
static double		                immobileTimeout    = 60*60*1000;
static double                           immobileDistance   = 1;
static bool				dropPrivate        = false;
static bool				dropImmobile       = false;
static bool				timeRangeValid     = false;
static int				validTimeRangeHourBegin = 0;
static int				validTimeRangeMinBegin  = 0;
static int				validTimeRangeHourEnd   = 24;
static int				validTimeRangeMinEnd    = 0;

static bool				g_Info             = false;
static bool				g_Unite            = false;
static TrackBase     			g_Track;
static int				g_Pass = 0;
static int				g_NumPasses = 1;
static std::string			g_Regions;
static std::string			g_UniteTime( "no" );

class TrackInfo
{
public:
  
  int		id;
  uint64_t	timestamp_enter;
  uint64_t	timestamp_touched;
  
  TrackInfo( int id )
    : id( id )
  {}
    
};


class TrackInfoMap : public std::map<int,TrackInfo> 
{
public:
  
};

typedef std::map<UUID,TrackInfoMap> TrackInfoMapDict;



class UUIDMap : public std::set<int> 
{
public:

};

typedef std::map<UUID,UUIDMap> UUIDMapDict;


static TrackInfoMap			*g_InfoMap = NULL;
static TrackInfoMapDict		 	 g_InfoMaps;
static UUIDMap	 			*g_DropMap = NULL;
static UUIDMapDict 			 g_DropMaps;
static UUIDMap	 			*g_PrivateMap = NULL;
static UUIDMapDict 			 g_PrivateMaps;
static UUIDMap	 			*g_ImmobileMap = NULL;
static UUIDMapDict 			 g_ImmobileMaps;
static UUID	 			 g_CurrentUUID;

static std::string			 g_InstallDir( "./" );
static std::string			 g_RealInstallDir( "./" );

/***************************************************************************
*** 
*** Helper
***
****************************************************************************/

static bool
testConfigDir( std::string &dir )
{
  std::string testDir( dir );
  rtrim( testDir, "/" );

  if ( !fileExists( testDir.c_str() ) )
  { 
    if ( g_InstallDir.empty() )
      return false;
    
    testDir = g_InstallDir + testDir;

    if ( !fileExists( testDir.c_str() ) )
      return false;
  }
  
  testDir += "/";
  
  TrackGlobal::configDir = testDir;

  return true;
}


static bool
testConf( std::string &conf )
{
  if ( conf.empty() )
    return false;

  std::string confDir( conf );
  
  if ( testConfigDir( confDir ) )
    return true;

  confDir = "conf/";
  confDir += conf;

  return testConfigDir( confDir );
}


static void
readConfigDir()
{
  std::string fileName( "config.txt" );
  std::string confDir, conf;

  const char *env = std::getenv( "LIDARCONF" );
  if ( env != NULL && env[0] != '\0' )
    conf = env;
  
  if ( conf.empty() )
  {
    fileName = g_InstallDir + "configDir.txt";

    std::ifstream stream( fileName );
    if ( stream.is_open() )
    {
      if ( g_Verbose )
	TrackGlobal::info( "reading config dir file '%s'", fileName.c_str() );

      std::string dir;
      stream >> dir;
      if ( testConf( dir ) )
	return;
    }
  }
  
  testConf( conf );
}

static void
setInstallDir( const char *executable )
{
  g_InstallDir = filePath( executable );
  if ( g_InstallDir.empty() )
    g_InstallDir = "./";

#ifdef PATH_MAX
  const int path_max = PATH_MAX;
#else
  int path_max = pathconf(executable, _PC_PATH_MAX);
  if (path_max <= 0)
    path_max = 4096;
#endif
  
  char buf[path_max];
  char *res = realpath(executable, buf);
  if ( res != NULL )
    g_RealInstallDir = filePath( buf );
}


static KeyValueMap g_EnvVar;

static bool
replaceEnvVar( std::string &str )
{
  if ( g_EnvVar.size() == 0 )
  { extern char **environ;
    for ( int i = 0; environ[i] != NULL; ++i )
    { std::string envVar( environ[i] );
      char *val = (char*)strchr(envVar.c_str(), '=' );
      if ( val != NULL )
      { 
	std::string value( &val[1] );
	val[0] = '\0';
	
	std::string key( "$(" );
	key += envVar.c_str();
	key += ")";

	if ( g_EnvVar.count(key) )
	  g_EnvVar[key] = value;
	else
	  g_EnvVar.emplace( std::make_pair(key,value) );

//	printf( "%s = %s\n", key.c_str(), value.c_str() );
      }
    }
  }
  
  bool result = false;
  
  for ( KeyValueMap::iterator iter( g_EnvVar.begin() ); iter != g_EnvVar.end(); iter++ )
    if ( replace( str, iter->first, iter->second ) )
      result = true;
  
  return result;
}
  
static void
replaceEnvVar( KeyValueMap &map )
{ 
  for ( KeyValueMap::iterator iter( map.begin() ); iter != map.end(); iter++ )
    replaceEnvVar( iter->second );
}

static void
setFilter( KeyValueMap &descr, const char *filter )
{
  std::string f( filter );
  if ( !g_Regions.empty() )
  {
//    printf( "Set\n" );
    
    descr.set( "regions", g_Regions.c_str() );
    f += ",region";
  }
  
  descr.set( "filter", f.c_str() );
}

static void
parseFilter( TrackableObserver *observer, const char *filter )
{
  std::string f( filter );
  if ( !g_Regions.empty() )
  {
    KeyValueMap descr;
    descr.set( "regions", g_Regions.c_str() );
    g_Track.setObserverParam( observer, descr );
    f += ",region";
  }
  observer->obsvFilter.parseFilter( f.c_str() );
}

/***************************************************************************
*** 
*** Blueprints
***
****************************************************************************/

static std::string	blueprintsFileName;
static KeyValueMap	blueprints;
static Matrix3H		bpMatrix,  bpMatrixInv;
static std::string	bluePrintExtend( "10" );
static float            bluePrintExtendPixels = 0;
static float            bluePrintExtendX = 10;
static float            bluePrintExtendY = 10;
static float		bluePrintPPM     = 1;
static std::string	trackOcclusionMapFileName;
static rpImg		trackOcclusionMapImg;
static bool		g_UseOcclusionMap = false;


static bool
readBlueprints()
{ return TrackGlobal::ReadKeyValues( blueprints, blueprintsFileName.c_str() );
}

static int
trackableMask( const ObsvObject &object )
{
  rpImg &oImg( trackOcclusionMapImg );
  int ow = oImg.width();
  int oh = oImg.height();

  Vector3D coord( object.x, object.y, 0.0 );
  Vector3D coordMap = coord - bpMatrix.w;

  int ox =   bluePrintPPM * coordMap.x + ow/2;
  int oy =  -bluePrintPPM * coordMap.y + oh/2;

  int maskBits = 0;
    
  if ( ox >= 0 && ox < ow && oy >= 0 && oy < oh )
  {
    if ( oImg( ox, oy, 0, 3 ) < 128 )
      return maskBits;

    bool red   = (oImg( ox, oy, 0, 0 ) > 128);
    bool green = (oImg( ox, oy, 0, 1 ) > 128);
    bool blue  = (oImg( ox, oy, 0, 2 ) > 128);

    if ( red && green )
      	maskBits |= Trackable<BlobMarkerUnion>::Occluded;
    else
    {
      if ( red )
	maskBits |= Trackable<BlobMarkerUnion>::Portal;

      if ( green )
	maskBits |= Trackable<BlobMarkerUnion>::Green;
    }

    if ( oImg( ox, oy, 0, 2 ) > 128 )
      maskBits |= Trackable<BlobMarkerUnion>::Private;
  }
  
  return maskBits;
}

static bool
setBluePrints()
{
  if ( !readBlueprints() )
    return false;
  
  if ( !blueprints.get( "trackOcclusionMap", trackOcclusionMapFileName ) )
    return false;
  
  blueprints.get( "extend", bluePrintExtend );

  blueprints.get( "x", bpMatrix.w.x );
  blueprints.get( "y", bpMatrix.w.y );

  int width, height;
  try {
    rpImg img( TrackGlobal::getConfigFileName(trackOcclusionMapFileName.c_str()).c_str() );
    trackOcclusionMapImg = img; 
    width  = img.width();
    height = img.height();
  } catch(CImgException& e) {
    TrackGlobal::error( "can't read track occlusion image file %s", trackOcclusionMapFileName.c_str() );
    return false;
  }

  std::vector<std::string> pair( split(bluePrintExtend,'=',2) );
  if ( pair.size() == 2 )
  { bluePrintExtendPixels = ::atoi( pair[0].c_str() );
    bluePrintExtendX      = ::atof( pair[1].c_str() );
  }
  else
    bluePrintExtendX      = ::atof(pair[0].c_str() );

  if ( bluePrintExtendPixels != 0 )
    bluePrintExtendX *= width / (double)bluePrintExtendPixels;

  bluePrintPPM = width / bluePrintExtendX;
  
  if ( g_Verbose )
    fprintf( stderr, "using track occlusion image %s extend=%s (%dx%d)\n", trackOcclusionMapFileName.c_str(), bluePrintExtend.c_str(), width, height );

  bluePrintExtendY = (bluePrintExtendX * height) / width;

  g_UseOcclusionMap = true;

  return true;
}

/***************************************************************************
*** 
*** TrackableEvalObserver
***
****************************************************************************/

class EvalCtx : public ObsvUserData
{
public:
  
  std::vector<uint64_t>	   m_AvgCounts;
  std::vector<uint64_t>	   m_NumAvgCounts;
  std::vector<uint64_t>	   m_MinCounts;
  std::vector<uint64_t>	   m_MaxCounts;
  std::vector<std::pair<int,uint64_t>> m_LifeSpans;
  std::vector<uint64_t>	   m_AvgLifeSpans;
  std::vector<uint64_t>	   m_NumAvgLifeSpans;
  uint64_t		   m_NumSamples;
  int64_t		   dropLifeSpan;
  
  int			   window, minCol, maxCol;
  
  EvalCtx( int window, int minCol, int maxCol, int64_t dropLifeSpan )
    : ObsvUserData(),
      m_NumSamples( 0 ),
      window( window ),
      minCol( minCol ),
      maxCol( maxCol ),
      dropLifeSpan( dropLifeSpan )
  {}
  
  ~EvalCtx() {}
  
  void eval( ObsvObjects &objects )
  {
    time_t t = objects.timestamp / 1000;
    struct tm timeinfo = *localtime( &t );

    int sec    = timeinfo.tm_sec;
    int min    = timeinfo.tm_min;
    int hour   = timeinfo.tm_hour;
      
    int numWindows = (24*60) / window;
    int col    = (hour*60+min)/window;
      
    if ( m_AvgCounts.size() != numWindows )
    {
      m_AvgCounts.resize      ( numWindows, 0 );
      m_NumAvgCounts.resize   ( numWindows, 0 );
      m_AvgLifeSpans.resize   ( numWindows, 0 );
      m_NumAvgLifeSpans.resize( numWindows, 0 );
      m_MinCounts.resize      ( numWindows, 1000000 );
      m_MaxCounts.resize      ( numWindows, 0 );
    }
      
    m_AvgCounts[col]    += objects.validCount;
    m_NumAvgCounts[col] += 1;
      
    if ( objects.validCount > m_MaxCounts[col] )
      m_MaxCounts[col] = objects.validCount;

    if ( objects.validCount < m_MinCounts[col] )
      m_MinCounts[col] = objects.validCount;

    for ( auto &iter: objects )
      if ( iter.second.status == ObsvObject::Leave )
      { 
	ObsvObject &object( iter.second );

	uint64_t timestamp_enter   = object.timestamp_enter;
	uint64_t timestamp_touched = object.timestamp_touched;

	uint64_t lifespan = timestamp_touched - timestamp_enter;
        if ( lifespan > dropLifeSpan )
        {
	  m_LifeSpans.push_back( std::make_pair(col,lifespan) );
 
	  m_AvgLifeSpans[col]    += lifespan;
	  m_NumAvgLifeSpans[col] += 1;
	}
      }

    m_NumSamples += 1;
  }

  void stop()
  {
    for ( int col = 0; col < m_MinCounts.size(); ++col )
      if ( m_MinCounts[col] > m_MaxCounts[col] )
	m_MinCounts[col] = m_MaxCounts[col];
  }

  void writeTimes( FILE *file, const char *key, std::vector<uint64_t> samples )
  {
    fprintf( file, "  \"%s\": [", key );
    bool first = true;    
    for ( int col = minCol; col < maxCol; ++col )
    { if ( first )
	first = false;
      else
	fprintf( file, ", " );
      fprintf( file, " \"%02d:%02d-%02d:%02d\"", (col*window)/60, (col*window)%60, ((col+1)*window)/60, ((col+1)*window)%60 );
    }
    fprintf( file, " ]" );
  }

  void writeValues( FILE *file, const char *key, std::vector<uint64_t> samples )
  {
    fprintf( file, "    \"%s\": [", key );

    bool first = true;    
    for ( int col = minCol; col < maxCol; ++col )
    { if ( first )
	first = false;
      else
	fprintf( file, ", " );
      fprintf( file, " %ld", samples[col] );
    }
    fprintf( file, " ]" );
  }
  
  void writeAvgs( FILE *file, const char *key, std::vector<uint64_t> samples, std::vector<uint64_t> numSamples, double devider=1 )
  {
    fprintf( file, "    \"%s\": [", key );

    bool first = true;    
    for ( int col = minCol; col < maxCol; ++col )
    { if ( first )
	first = false;
      else
	fprintf( file, ", " );
      if ( numSamples[col] > 0 )
	fprintf( file, " %g", samples[col] / (double) numSamples[col] / devider );
      else
	fprintf( file, " 0.0" );
    }
    fprintf( file, " ]" );
  }
  
  void writeValues( FILE *file, const char *key, std::vector<std::pair<int,uint64_t>> samples, double devider=1 )
  {
    fprintf( file, "    \"%s\": [ ", key );

    bool first = true;    
    for ( int i = 0; i < samples.size(); ++i )
    { if ( first )
	first = false;
      else
	fprintf( file, ", " );

      fprintf( file, "%g", samples[i].second / devider );
    }
    fprintf( file, " ]" );
  }
  
  void write( FILE *file, const char *title )
  {
  }
  
};

  
class TrackableEvalObserver : public TrackableFileObserver
{
public:

  float dropLifeSpan;
  int	window;
  std::string minCol, maxCol;
  
  TrackableEvalObserver()
    : TrackableFileObserver(),
      window( 60 ),
      dropLifeSpan( 0 )
  {
    type 	   = File;
    isThreaded	   = false;
    isJson	   = false;
    name           = "eval";

//    obsvFilter.parseFilter( "timestamp=ts,action,start,stop,regions,objects,type,enter,move,leave,x,y,z,size,id,lifespan,count" );

    parseFilter( this, "timestamp=ts,action,start,stop,objects,type,enter,leave,x,y,z,size,id,uuid,lifespan,count" );
  }

  void setParam( KeyValueMap &descr )
  {
    TrackableObserver::setParam( descr );

    std::string fileName;
    if ( descr.get( "file", fileName ) )
      setFileName( fileName.c_str() );

    std::string win;
    if ( descr.get( "window", win ) )
      window = minutes( win );

    descr.get( "minCol",   	minCol  );
    descr.get( "maxCol",   	maxCol  );
    descr.get( "dropLifeSpan", 	dropLifeSpan  );
  }
  
  int minutes( const std::string &string )
  {
    int hour = 0;
    int min  = 0;
    
    std::vector<std::string> pair( split(string,':') );

    if ( pair.size() > 1 )
    { hour = std::atoi( pair[0].c_str() );
      min  = std::atoi( pair[1].c_str() );
    }
    else
      min  = std::atoi( pair[0].c_str() );

    return hour*60 + min;
  }
  
  int col( const std::string &string )
  {
    int numWindows = (24*60) / window;

    int min  = minutes( string );

    return min / window;
  }
  
  void report()
  { 
    for ( int i = rects.numRects()-1; i >= 0; --i )
    {
      ObsvObjects &objects( rects.rect(i).objects );
	
      if ( objects.userData == NULL )
      {
	int numWindows = (24*60) / window;
	int min = 0;
	int max = numWindows-2;
	
	if ( !minCol.empty() )
	  min = col( minCol );

	if ( !maxCol.empty() )
	  max = col( maxCol );

	if ( max >= numWindows )
	  max = numWindows-1;

	objects.userData = new EvalCtx( window, min, max, dropLifeSpan*1000 );
      }
      
      static_cast<EvalCtx*>(objects.userData)->eval( objects );
    }
  }

  bool stop( uint64_t timestamp=0 )
  {
    if ( timestamp == 0 )
      timestamp = getmsec();

    if ( !TrackableObserver::stop(timestamp) )
      return false;
    
    std::string fn( templateToFileName( timestamp ) );
	
    FILE *file;
    if ( fn == "-" )
      file = stdout;
    else
    {
      std::string path( filePath( fn.c_str() ) );
      if ( !path.empty() && !fileExists( path.c_str() ) )
	std::filesystem::create_directories( path.c_str() );

      file = fopen( fn.c_str(), "w" );
    }
    
    if ( file == NULL )
    { TrackGlobal::error( "TrackableEvalObserver: opening file '%s'", fn.c_str() );
      return false;
    }
    
    fprintf( file, "{\n" );

    for ( int i = 0; i < rects.numRects(); ++i )
    {
      ObsvRect &rect( rects.rect(i) );
      
      ObsvObjects &objects( rect.objects );
	
      EvalCtx &ctx( *static_cast<EvalCtx*>(objects.userData) );
      
      ctx.stop();
      
      if ( i == 0 )
      { ctx.writeTimes( file, "time", ctx.m_MaxCounts );
	fprintf( file, ",\n" );
	fprintf( file, "  \"regions\": {\n" );
      }
      
      fprintf( file, "  \"%s\": {\n", rect.name.empty() ? "all" : rect.name.c_str() );

      ctx.writeValues( file, "maxCount", ctx.m_MaxCounts );
      fprintf( file, ",\n" );
	
      ctx.writeValues( file, "minCount", ctx.m_MinCounts );
      fprintf( file, ",\n" );
	
      ctx.writeAvgs( file, "avgCount", ctx.m_AvgCounts, ctx.m_NumAvgCounts);
      fprintf( file, ",\n" );
	
      ctx.writeAvgs( file, "avgLifeSpan", ctx.m_AvgLifeSpans, ctx.m_NumAvgLifeSpans, 1000 );
      fprintf( file, ",\n" );
      
      ctx.writeValues( file, "lifeSpan", ctx.m_LifeSpans, 1000 );
      fprintf( file, "\n" );
      fprintf( file, "  }" );
      if ( i < rects.numRects()-1 )
	fprintf( file, ",\n" );
      fprintf( file, "\n" );
      
//	printf( "region(%s) %02d:%02d max: % 3ld, avg: %.2g\n", rect.name.empty() ? "all" : rect.name.c_str(), col/quant , (col%quant)*60/quant, ctx.m_MaxCounts[col], ctx.m_AvgCounts[col] / (double)ctx.m_NumSamples );
    }

    fprintf( file, "  }\n}\n" );

    if ( file != stdout )
      fclose( file );
    
    return true;
  }
  
};

  
/***************************************************************************
*** 
*** TrackableDropObserver
***
****************************************************************************/

class TrackableDropObserver : public TrackableObserver
{
public:

  TrackableDropObserver()
  : TrackableObserver()
  {
    type 	   = File;
    isThreaded	   = false;
    name           = "drop";
  }

  virtual bool observe( const ObsvObjects &other, bool force=false )
  {
    timestamp = other.timestamp;

    ObsvObjects &objects( rects.rect(0).objects );
    objects.timestamp       = other.timestamp;
 
    for ( auto &iter: objects )
      iter.second.status = ObsvObject::Invalid;

    for ( auto &iter: other )
    { const ObsvObject &object( iter.second );

      int maskBits = (g_UseOcclusionMap ? trackableMask( object ) : 0);
      if ( !(maskBits & Trackable<BlobMarkerUnion>::Occluded) )
      {
	ObsvObject *obj = objects.get( object.id );

	if ( obj == NULL )
        { auto pair( objects.emplace(object.id, object) );
	  obj = &pair.first->second;
	  obj->objects 		 = &objects;
	  obj->status            = ObsvObject::Enter;
	  obj->timestamp_enter   = timestamp;
	  obj->timestamp_touched = timestamp;
	}
	else
	  obj->status = ObsvObject::Move;

	obj->flags = object.flags;
	if ( object.isTouched() )
	  obj->timestamp_touched = timestamp;

	bool isPrivate = object.isPrivate();
	if ( g_UseOcclusionMap )
	{ obj->touchPrivate( maskBits & ObsvObject::Private, timestamp, privateTimeout );
	  isPrivate |= obj->isPrivate();
	}

	if ( isPrivate )
        {
	  obj->setPrivate( true );

	  auto iter( g_PrivateMap->find(object.id) );
	  if ( iter == g_PrivateMap->end() )
          { g_PrivateMap->emplace( object.id );
	    numPrivates += 1;
	  }
	}

	if ( dropImmobile )
	{
	  obj->checkImmobile( objects.timestamp, immobileTimeout, immobileDistance );
	
	  if ( obj->isImmobile() )
          {
	    auto iter( g_ImmobileMap->find(object.id) );
	    if ( iter == g_ImmobileMap->end() )
	    { g_ImmobileMap->emplace( object.id );
	      numImmobiles += 1;
	    }
	  }
	}

	auto iiter( g_InfoMap->find(object.id) );
        if ( iiter == g_InfoMap->end() )
        { auto pair( g_InfoMap->emplace( object.id, TrackInfo(object.id) ) );
	  TrackInfo &info( pair.first->second );
	  info.timestamp_enter   = timestamp;
	  info.timestamp_touched = timestamp;
	}
	else if ( object.isTouched() )
	  iiter->second.timestamp_touched = timestamp;
      }
    }
	
    return true;
  }

  virtual void cleanup()
  {
    ObsvObjects &objects( rects.rect(0).objects );

    for ( auto iter = g_InfoMap->begin(); iter != g_InfoMap->end(); ++iter )
    {
      TrackInfo &info( iter->second );

      bool isPrivate = false;
      if ( dropPrivate )
      { auto piter( g_PrivateMap->find(info.id) );
	if ( piter != g_PrivateMap->end() )
	  isPrivate = true;
      }

      bool isImmobile = false;
      if ( dropImmobile )
      { auto iiter( g_ImmobileMap->find(info.id) );
	if ( iiter != g_ImmobileMap->end() )
	  isImmobile = true;
      }

      int64_t lifeSpan = info.timestamp_touched - info.timestamp_enter;

      if ( lifeSpan <= dropLifeSpan || isPrivate || isImmobile )
      {
	if ( g_Verbose > 1 )
        {
	  if ( isImmobile )
	    fprintf( stderr, "dropping tid: %d  (%.3f,immobile)\n", info.id, immobileTimeout/1000.0 );
	  else if ( isPrivate && lifeSpan <= dropLifeSpan )
	    fprintf( stderr, "dropping tid: %d  (%.3f,private)\n", info.id, lifeSpan/1000.0 );
	  else if ( isPrivate )
	    fprintf( stderr, "dropping tid: %d  (private)\n", info.id );
	  else
	    fprintf( stderr, "dropping tid: %d  (%.3f)\n", info.id, lifeSpan/1000.0 );
	}

	g_DropMap->emplace( info.id );
	  
	numDrops += 1;
      }
      else
      { if ( g_Verbose > 1 )
	  fprintf( stderr, "keeping  tid: %d  (%.3f)\n", info.id, lifeSpan/1000.0 );
	numKeeps += 1;
      }
    }
  }
  
};

/***************************************************************************
*** 
*** Play
***
****************************************************************************/

static bool firstStart 		= true;
static bool isStarted  		= false;
static bool observerStarted  	= false;

static PackedTrackable::Header stopHeader( 0, PackedTrackable::StopHeader );
static std::string dateName;
  
inline bool
checkDayInRange( const PackedTrackable::Header &header )
{
  if ( !timeRangeValid )
    return true;

  time_t t = header.timestamp / 1000;
  struct tm timeinfo = *localtime( &t );

  int min    = timeinfo.tm_min;
  int hour   = timeinfo.tm_hour;
      
  return (hour > validTimeRangeHourBegin || (hour == validTimeRangeHourBegin && min >= validTimeRangeMinBegin)) &&
         (hour < validTimeRangeHourEnd   || (hour == validTimeRangeHourEnd   && min <= validTimeRangeMinEnd));
}


inline void
startDropPass( const PackedTrackable::Header &header )
{
  if ( !isStarted )
  { numStarts += 1;
    isStarted  = true;
  }
}

inline void
logStartStop( const PackedTrackable::Header &header, bool start, bool dropPass )
{
  if ( g_Verbose == 0 )
    return;
  
  std::string time( timestampString( "%c", header.timestamp ) );
  fprintf( stderr, "%s: %s %s\n", dropPass?"Drop Pass": "Calc Pass", time.c_str(), start?"start()": "stop()" );
}

static void
startNormalPass( const PackedTrackable::Header &header )
{
  if ( checkDayInRange(header) && header.timeStampValid() )
  {
    if ( g_Unite )
    { if ( !isStarted )
      { isStarted = true;
	if ( firstStart )
        { firstStart = false;
	  g_Track.m_Stage->observer->start( header.timestamp );
	  observerStarted = true;

	  logStartStop( header, true, false );
	}
      }
    }
    else if ( !(observerStarted&&isStarted) )
    {
      g_Track.m_Stage->observer->start( header.timestamp );
      observerStarted 	= true;
      isStarted 	= true;

      logStartStop( header, true, false );
    }
  }
}

inline void
start( const PackedTrackable::Header &header, bool dropPass )
{
  if ( dropPass )
    startDropPass( header );
  else
    startNormalPass( header );
}


inline void
stopDropPass( const PackedTrackable::Header &header )
{
  if ( isStarted )
  { numStops += 1;
    isStarted = false;
  }
}

static void
stopNormalPass( const PackedTrackable::Header &header, bool forceWrite=false )
{
  if ( g_Unite && !forceWrite )
  { if ( isStarted )
    { isStarted = false;
    }
  }
  else 
  {
    if ( forceWrite || (checkDayInRange( header ) && header.timeStampValid() && (isStarted||observerStarted)) )
    {
      isStarted = false;
      if ( observerStarted )
      { g_Track.m_Stage->observer->stop( header.timestamp );
	observerStarted 	= false;
	logStartStop( header, false, false );
      }
    }
  }
}

inline void
stop( const PackedTrackable::Header &header, bool dropPass, bool forceWrite=false )
{
  if ( dropPass )
    stopDropPass( header );
  else
    stopNormalPass( header, forceWrite );
}

static void
stall( const PackedTrackable::Header &header, bool dropPass, bool forceWrite=false )
{
  if ( dropPass )
  { 
  }
  else if ( checkDayInRange( header ) && header.timeStampValid() )
  { 
    g_Track.m_Stage->observer->stall( header.timestamp );
  }
}

static void
resume( const PackedTrackable::Header &header, bool dropPass )
{
  if ( dropPass )
  { 
  }
  else if ( checkDayInRange( header ) && header.timeStampValid() )
  { 
    g_Track.m_Stage->observer->resume( header.timestamp );
  }
}

static void
clearDay()
{
  dateName   = "";
  isStarted  = false;
  firstStart = true;
}

inline void
checkDay( const PackedTrackable::Header &header, bool dropPass )
{
  if ( !g_Unite || dropPass )
    return;
  
  std::string fileName( timestampString( g_UniteTime.c_str(), header.timestamp ) );
  
  if ( fileName == dateName )
    return;
  
  if ( dateName.empty() )
  { dateName = fileName;
    return;
  }

  stop( stopHeader, dropPass, true );
  
  clearDay();
}


static bool
play( const char *inFile, bool dropPass=false )
{
  TrackableDropObserver dropObserver;

  PackedPlayer *player = new PackedPlayer();

  if ( !player->open( inFile ) )
  { cerr << "Error opening file " << inFile << "\n";
    exit( 1 );
  }
  
  clearDay();
  
  bool ok = true;
  long failPos;
  
  uint64_t realTime = 0;

  while ( !player->is_eof() )
  {
    PackedTrackable::Header     header;
    PackedTrackable::HeaderType type = player->nextHeader( header );

    if ( !type )
    {
      if ( ok )
      { failPos = player->file->tell();
	ok = false;
      }
    }
    else if ( !player->is_eof() )
    {
      if ( !ok )
      { 
	if ( g_Verbose )
        { int64_t timeDiff = header.timestamp - player->lastFrame.header.timestamp;
	  std::string time1( timestampString( "%c", player->lastFrame.header.timestamp ) );
	  std::string time2( timestampString( "%c", header.timestamp ) );

	  if ( player->lastFrame.header.timeStampValid() )
	    TrackGlobal::error( "%s: failed at %lx skipped %ld bytes, %g sec -> %s\n", time1.c_str(), failPos, player->file->tell() - failPos, timeDiff/1000.0, time2.c_str() );
	}
	
	ok = true;
//      printf( "got: %016lx: %ld %d\n", player->currentFrame.header.timestamp, player->currentFrame.size(), g_Track.m_Stage->observer != NULL  );
      }

      {
	checkDay( header, dropPass );

	if ( header.isType( PackedTrackable::StartHeader ) )
	  start( header, dropPass );
	else if ( header.isType( PackedTrackable::StopHeader ) )
	  stop( header, dropPass );
	
	int64_t timeDiff = 0;
	if ( player->lastFrame.header.timeStampValid() && header.timeStampValid() )
	  timeDiff = header.timestamp - player->lastFrame.header.timestamp;
	
	if ( timeDiff < 0 || timeDiff >= 5000 ) // 5sec no data give a error message
        {
	  if ( dropPass )
          {
	    std::string time1( timestampString( "%c", player->lastFrame.header.timestamp ) );
	    std::string time2( timestampString( "%c", header.timestamp ) );
	
	    if ( player->lastFrame.header.timeStampValid() && header.timeStampValid() )
	      TrackGlobal::error( "%s skipped %g sec (%ldms) (%ld) -> (%ld) %s", time1.c_str(), timeDiff/1000.0, timeDiff, player->lastFrame.header.timestamp, header.timestamp, time2.c_str() );
	  }
	  
	  if ( timeDiff >= startStopPauseTime && !dropPass ) // start stop when it a really long time
          { 
	    if ( g_Unite )
            {
	      stall ( player->lastFrame.header,    dropPass );
	      resume( player->currentFrame.header, dropPass );
	    }
	    else
            {
	      stop ( player->lastFrame.header,    dropPass );
	      start( player->currentFrame.header, dropPass );
	    }
	  }
	}

	ObsvObjects objects;
	if ( header.isType( PackedTrackable::FrameHeader ) && player->nextFrame( objects, header ) )
        {
	  if ( objects.uuid != g_CurrentUUID )
          {
	    auto iiter( g_InfoMaps.find(objects.uuid) );
	    if ( iiter == g_InfoMaps.end() )
	    { auto pair( g_InfoMaps.emplace( objects.uuid, TrackInfoMap() ) );
	      g_InfoMap = &pair.first->second;
	    }
	    else
	      g_InfoMap = &iiter->second;

	    auto diter( g_DropMaps.find(objects.uuid) );
	    if ( diter == g_DropMaps.end() )
	    { auto pair( g_DropMaps.emplace( objects.uuid, UUIDMap() ) );
	      g_DropMap = &pair.first->second;
	    }
            else
	      g_DropMap = &diter->second;

	    auto piter( g_PrivateMaps.find(objects.uuid) );
            if ( piter == g_PrivateMaps.end() )
            { auto pair( g_PrivateMaps.emplace( objects.uuid, UUIDMap() ) );
	      g_PrivateMap = &pair.first->second;
	    }
            else
	      g_PrivateMap = &piter->second;

	    auto imiter( g_ImmobileMaps.find(objects.uuid) );
            if ( imiter == g_ImmobileMaps.end() )
            { auto pair( g_ImmobileMaps.emplace( objects.uuid, UUIDMap() ) );
	      g_ImmobileMap = &pair.first->second;
	    }
            else
	      g_ImmobileMap = &imiter->second;

	    g_CurrentUUID = objects.uuid;
	  }
	  
	  if ( dropPass )
          {
	    if ( g_Info )
            {
	      if ( firstTimeStamp == 0 )
		firstTimeStamp = objects.timestamp;
	  
	      if ( objects.timestamp > lastTimeStamp )
		lastTimeStamp = objects.timestamp;

	      if ( player->lastFrame.header.timeStampValid() && player->currentFrame.header.timeStampValid() )
              {
		int64_t timeDiff = player->currentFrame.header.timestamp - player->lastFrame.header.timestamp;

		if ( timeDiff > 0 )
                {
		  frameTimeSum += timeDiff;
		  numFrames    += 1;

		  if ( timeDiff > maxFrameTime )
		    maxFrameTime = timeDiff;
		  if ( minFrameTime == 0 || timeDiff < minFrameTime )
		    minFrameTime = timeDiff;
		}
	      }
	    }

	    dropObserver.observe( objects );
	  }
	  else
          {
	    objects.frame_id   = player->frame_id;
	
	    uint64_t size = objects.size();
	  
	    for ( auto iter = objects.begin(); iter != objects.end(); )
            { 
	      ObsvObject &object( iter->second );

	      bool dropIt = (g_DropMap->count( object.id ) > 0);
	      if ( !dropIt )
              { auto iiter( g_InfoMap->find(object.id) );
		if ( iiter != g_InfoMap->end() )
                { TrackInfo &info( iiter->second );
		  if ( info.timestamp_touched < objects.timestamp )
		    dropIt = true;
		}
	      }

	      if ( !dropIt && object.isLatent() )
              { auto iiter( g_InfoMap->find(object.id) );
		if ( iiter == g_InfoMap->end() )
		  dropIt = true;
		else
                { TrackInfo &info( iiter->second );
		  if ( info.timestamp_touched < objects.timestamp )
		    dropIt = true;
		  else
		    object.setLatent( false );
		}
	      }
	      
	      if ( !dropIt )
              {
		auto piter( g_PrivateMap->find(object.id) );
		if ( piter != g_PrivateMap->end() )
                {
		  if ( dropPrivate )
		    dropIt = true;
		  else
		    object.setPrivate( true );
		}
	      }

	      if ( !dropIt )
              {
		auto piter( g_ImmobileMap->find(object.id) );
		if ( piter != g_ImmobileMap->end() )
                {
		  if ( dropImmobile )
		    dropIt = true;
		  else
		    object.setImmobile( true );
		}
	      }

	      if ( dropIt )
		iter = objects.erase(iter);
	      else
		++iter;
	    }

	    if ( checkDayInRange( header ) )
            {
	      if ( !observerStarted )
		start( player->currentFrame.header, dropPass );
	      g_Track.m_Stage->observer->observe( objects );
	      stopHeader.timestamp = objects.timestamp;
	    }
	    
	    objects.update();
	  }
	}
      
	if ( g_Verbose && objects.frame_id % 100 == 0 )
        {
	  uint64_t t = getmsec();
	  if ( t - realTime > 1000 )
          { realTime = t;
	    std::string time1( timestampString( "%c", player->currentFrame.header.timestamp ) );
	    fprintf( stderr, "%s: %s\r", dropPass?"Drop Pass": "Calc Pass", time1.c_str() );
	  }
	}
      }
    }
  }

  if ( !dropPass )
  {
    if ( g_Unite )
    {
//      printf( "Unite Stop: %ld %d\n", stopHeader.timestamp, isStarted );
      stop( stopHeader, dropPass, true );
    }
    else
      stop( stopHeader, dropPass );
  }
  else
    dropObserver.cleanup();

  delete player;
  
  return true;
}

/***************************************************************************
*** 
*** HELP
***
****************************************************************************/

void printHelp( int argc, const char *argv[] )
{
  printf( "usage: %s [-h|-help] [+v [verboseLevel]] [+ts dateFormat] [+ps pauseSec (default=%g)] [+dropSec lifeSpanSec (default=%g)] [+dropPrivate] [+privateTimeout sec (default=%g)] [+dropImmobile] [+immobileTimeout sec (default=%g)] [+immobileDistance dist (default=%g)] [+timeRange hour:min hour:min] [+log outLogName.log|-] [+o outName.pkf] +i inName.pkf\n", argv[0], startStopPauseTime/1000.0, dropLifeSpan/1000.0, privateTimeout/1000.0, immobileTimeout/1000.0, immobileDistance );

  printf( " +v               verbose\n" );
  printf( " +i inName.pkf    packed file to process\n" );
  printf( " +ts format       format of time stamps (%%c=human readable)\n" );
}

/***************************************************************************
*** 
*** main
***
****************************************************************************/

int main( int argc, const char *argv[] )
{
  std::string   inFile;

  cimg::exception_mode(0);

  setInstallDir( argv[0] );
  
  readConfigDir();

  if ( TrackGlobal::configDir.empty() )
    TrackGlobal::configDir = g_InstallDir + "conf/";

  for ( int i = 0; i < argc; ++i )
  {
    if ( strcmp(argv[i],"+conf") == 0 )
    { 
      std::string conf( argv[++i] );
      if ( !testConf( conf ) )
      { TrackGlobal::error( "setting config: directory %s does not exist", conf );
	exit( 0 );
      }
    }
    else if ( strcmp(argv[i],"+setRegionsFile") == 0 )
    { 
      TrackBase::regionsFileName = argv[++i];
    }
    else if ( strcmp(argv[i],"+region") == 0 || strcmp(argv[i],"+regions") == 0 )
    {
      g_Regions = argv[++i];
    }
  }

  if ( TrackGlobal::configDir[TrackGlobal::configDir.length()-1] != '/' )
    TrackGlobal::configDir += "/";

  TrackGlobal::defaultsFileName = TrackGlobal::configDir + "defaults.json";

   if ( !TrackGlobal::setDefaults( argc, argv ) )
    exit( 0 );

  if ( !TrackGlobal::parseDefaults( argc, argv ) )
    exit( 0 );

  if ( TrackGlobal::configDir[TrackGlobal::configDir.length()-1] != '/' )
    TrackGlobal::configDir += "/";

  TrackGlobal::defaultsFileName = TrackGlobal::getConfigFileName( "defaults.json" );
  TrackGlobal::observerFileName = TrackGlobal::getConfigFileName( "observer.json" );
  TrackGlobal::regionsFileName  = TrackGlobal::getConfigFileName( TrackGlobal::regionsFileName.c_str() );
  blueprintsFileName  		= TrackGlobal::getConfigFileName( "blueprints.json" );

  TrackGlobal::readDefaults();
  replaceEnvVar( TrackGlobal::defaults );
  TrackGlobal::loadRegions();

/*  
    for ( int i = 0; i < TrackGlobal::regions.size(); ++i )
    {
      TrackableRegion &region( TrackGlobal::regions[i] );
	
      printf( "\n" );
      printf( "name=\"%s\"\n", region.name.c_str() );
      printf( " x=%g\n", region.x );
      printf( " y=%g\n", region.y );
      printf( " width=%g\n", region.width );
      printf( " height=%g\n", region.height );
      printf( " active=%s\n", region.active ? "true" : "false" );
      printf( " script=\"%s\"\n", region.script.c_str() );
//      printf( " type=%s\n", regionsTypeLabel[region.type].c_str() );
      printf( " edge=%s\n", g_RegionsEdgeName[region.edge].c_str() );
      printf( " shape=%s\n", g_RegionsShapeName[region.shape].c_str() );
      printf( " tags=\"%s\"\n", region.tagsStr.c_str() );
      printf( " layers=\"%s\"\n", region.layersStr.c_str() );
    }

    exit( 0 );
*/

  std::string 	timestampDate;
  
  for ( int i = 1; i < argc; ++i )
  {
    if 	( strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"-help") == 0 || strcmp(argv[i],"+h") == 0 || strcmp(argv[i],"+help") == 0 )
    {
      printHelp( argc, argv );
      exit( 0 );
    }
    if ( strcmp(argv[i],"+conf") == 0 )
    { ++i;
    }
    else if ( strcmp(argv[i],"+setRegionsFile") == 0 )
    { 
      ++i;
    }
    else if ( strcmp(argv[i],"+region") == 0 || strcmp(argv[i],"+regions") == 0 )
    {
      ++i;
    }
    else if ( strcmp(argv[i],"+v") == 0 )
    { 
      int level = 1;
      
      if ( i < argc-1 && isdigit(argv[i+1][0]) )
	level = atoi(argv[++i]);

      g_Verbose = level;
    }
    else if ( strcmp(argv[i],"+info") == 0 )
    { 
      g_Info = true;
    }
    else if ( strcmp(argv[i],"+uniteDay") == 0 )
    { 
      g_Unite = true;
      g_UniteTime = "%d";
    }
    else if ( strcmp(argv[i],"+uniteWeek") == 0 )
    { 
      g_Unite = true;
      g_UniteTime = "%V";
    }
    else if ( strcmp(argv[i],"+ts") == 0 )
    {
      timestampDate = argv[++i];
    }
    else if ( strcmp(argv[i],"+ps") == 0 )
    {
      startStopPauseTime = std::atof( argv[++i] ) * 1000;
    }
    else if ( strcmp(argv[i],"+dropSec") == 0 )
    {
      dropLifeSpan = std::atof( argv[++i] ) * 1000;
    }
    else if ( strcmp(argv[i],"+dropPrivate") == 0 )
    {
      dropPrivate = true;
    }
    else if ( strcmp(argv[i],"+dropImmobile") == 0 )
    {
      dropImmobile = true;
    }
    else if ( strcmp(argv[i],"+timeRange") == 0 )
    {
      const std::string t0( argv[++i] );
      const std::string t1( argv[++i] );
      
      std::vector<std::string> pair0( split(t0,':') );

      if ( pair0.size() > 1 )
      { validTimeRangeHourBegin = std::atoi( pair0[0].c_str() );
	validTimeRangeMinBegin  = std::atoi( pair0[1].c_str() );
      }
      else
	validTimeRangeHourBegin  = std::atoi( pair0[0].c_str() );

      std::vector<std::string> pair1( split(t1,':') );

      if ( pair1.size() > 1 )
      { validTimeRangeHourEnd = std::atoi( pair1[0].c_str() );
	validTimeRangeMinEnd  = std::atoi( pair1[1].c_str() );
      }
      else
	validTimeRangeHourEnd  = std::atoi( pair1[0].c_str() );


      timeRangeValid = true;
    }
    else if ( strcmp(argv[i],"+privateTimeout") == 0 )
    {
      privateTimeout = std::atof( argv[++i] ) * 1000;
    }
    else if ( strcmp(argv[i],"+immobileTimeout") == 0 )
    {
      immobileTimeout = std::atof( argv[++i] ) * 1000;
    }
    else if ( strcmp(argv[i],"+immobileDistance") == 0 )
    {
      immobileDistance = std::atof( argv[++i] );
    }
    else if ( strcmp(argv[i],"+occlusionMap") == 0 )
    {
      if ( !setBluePrints() )
      { TrackGlobal::error( "error setting occlusionMap" );
	exit( 2 );
      }
    }
    else if ( strcmp(argv[i],"+regions") == 0 )
    {
      ++i;
    }
    else if ( strcmp(argv[i],"+useObserver") == 0 || strcmp(argv[i],"+useObservers") == 0 )
    {
      std::string name( "all" );
      if ( strcmp(argv[i],"+useObserver") == 0 )
	name = argv[++i];

      TrackGlobal::readObservers();

      bool all = (name == "all");

      for ( KeyValueMapDB::iterator dbiter( TrackGlobal::observers.begin() ); dbiter != TrackGlobal::observers.end(); dbiter++ )
      { KeyValueMap &descr( dbiter->second );
	if ( all || name == dbiter->first )
	{ descr.set( "name", dbiter->first.c_str() );
	  g_Track.addObserver( descr );
	}
      }
    }
    else if ( strcmp(argv[i],"+observer") == 0 )
    {
      KeyValueMap descr;

      descr.set( "isThreaded", "0" );
      parseArg( i, argv, argc, descr );

      g_Track.addObserver( descr );
    }
    else if ( strcmp(argv[i],"+log") == 0 )
    {
      std::string fileName( argv[++i] );
      
      KeyValueMap descr;
      descr.set( "type", "file" );
      descr.set( "isThreaded", "0" );
      descr.set( "fullFrame",  "1" );
      descr.set( "continuous", "1" );
      descr.set( "file", fileName.c_str() );
      setFilter( descr, "timestamp=ts,action,start,stop,objects,enter,move,x,y,size,id,uuid" );

      if ( fileExists( fileName.c_str() ) )
	std::remove( fileName.c_str() ); 

      g_Track.addObserver( descr );
    }
    else if ( strcmp(argv[i],"+o") == 0 )
    {
      std::string fileName( argv[++i] );
      
      KeyValueMap descr;
      descr.set( "type", "packedfile" );
      descr.set( "isThreaded", "0" );
      descr.set( "maxFPS", "1000" );
      descr.set( "file", fileName.c_str() );

      if ( fileExists( fileName.c_str() ) )
	std::remove( fileName.c_str() ); 

      g_Track.addObserver( descr );
    }
    else if ( strcmp(argv[i],"+e") == 0 )
    {
      KeyValueMap descr;
      descr.set( "isThreaded", "0" );
      parseArg( i, argv, argc, descr );
      descr.set( "file", argv[++i] );

      TrackableObserver *observer = new TrackableEvalObserver();
      g_Track.setObserverParam( observer, descr );

      g_Track.addObserver( observer );
    }
    else if ( g_Track.m_Stage->parseArg( i, (const char **) argv, argc ) )
    {
    }
    else if ( strcmp(argv[i],"+i") == 0 )
    { 
      inFile = argv[++i];
    }
    else 
    { 
      TrackGlobal::error( "unknown option: %s", argv[i] );  
      exit( 0 );
    }
  }

  if ( inFile.empty() )
  { printHelp( argc, argv );
    exit( 1 );
  }

  if ( g_Verbose )
  {
    fprintf( stderr, "using regions file: %s\n", TrackGlobal::regionsFileName.c_str() );

    if ( timeRangeValid )
      fprintf( stderr, "using time range: %02d:%02d - %02d:%02d\n", validTimeRangeHourBegin, validTimeRangeMinBegin, validTimeRangeHourEnd, validTimeRangeMinEnd );
  }
  
  if ( !g_Info && (g_Track.m_Stage->observer == NULL || g_Track.m_Stage->observer->observer.size() == 0) )
  {
    KeyValueMap descr;

    descr.set( "type", "file" );
    descr.set( "isThreaded", "0" );
    descr.set( "fullFrame",  "1" );
    descr.set( "continuous", "1" );
    descr.set( "file", "-" );
    setFilter( descr, "timestamp=ts,action,start,stop,objects,enter,move,x,y,size,id,uuid" );

    g_Track.addObserver( descr );
  }
  
  if ( !timestampDate.empty() )
  {
    for ( int i = 0; i < g_Track.m_Stage->observer->observer.size(); ++i )
    {
      TrackableObserver *observer = g_Track.m_Stage->observer->observer[i];
      Filter::ObsvFilter &obsvFilter( observer->obsvFilter );
      
      auto iter( obsvFilter.KeyMap.find("timestamp") );
      if ( iter != obsvFilter.KeyMap.end() )
      { iter->second.append( "@" );
	iter->second.append( timestampDate );
      }
    }
  }

  play( inFile.c_str(), true );

  if ( g_Info )
  {
    printf( "{\n" );

    std::string firstTime( timestampString( "%c", firstTimeStamp ) );
    printf( "  \"First\":      %s,\n", firstTime.c_str() );

    std::string lastTime( timestampString( "%c", lastTimeStamp ) );
    printf( "  \"Last\":       %s\n", lastTime.c_str() );

    int timeDiff = (lastTimeStamp - firstTimeStamp) / 1000;
    int hour = (timeDiff / 3600);
    int min  = (timeDiff / 60) % 60;
    int sec  =  timeDiff % 60;

    printf( "  \"Duration\":    \"%02d:%02d:%02d\",\n", hour, min, sec );

    double fps = 0;
    if ( numFrames > 0 )
      fps = numFrames / (frameTimeSum / 1000.0);
    
    double minfps = 0;
    if ( maxFrameTime > 0 )
      minfps = 1.0 / (maxFrameTime / 1000.0);
    
    double maxfps = 0;
    if ( minFrameTime > 0 )
      maxfps = 1.0 / (minFrameTime / 1000.0);
    
    printf( "  \"AvgFPS\":       %g,\n", fps );
    printf( "  \"MaxFPS\":       %g,\n", maxfps );
    printf( "  \"MinFPS\":       %g,\n", minfps );
    printf( "  \"Starts\":       %d,\n", numStarts );
    printf( "  \"Stops\":        %d,\n", numStops );
    
    printf( "  \"NumIds\":       %d,\n", numKeeps + numDrops );
    printf( "  \"NumKeeps\":     %d,\n", numKeeps );
    printf( "  \"NumDrops\":     %d,\n", numDrops );
    printf( "  \"NumPrivates\":  %d,\n", numPrivates );
    printf( "  \"NumImmobiles\": %d,\n", numImmobiles );
    
    printf( "}\n" );
  }
  else
  {
    for ( g_Pass = 0; g_Pass < g_NumPasses; ++g_Pass )
      play( inFile.c_str(), false );
  }
}


