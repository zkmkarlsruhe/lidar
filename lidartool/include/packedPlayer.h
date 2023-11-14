// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef _PACKED_PLAYER_H_
#define _PACKED_PLAYER_H_

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#include <map>
#include "UUID.h"

#include "helper.h"
#include "keyValueMap.h"

#include "lidarTrackable.h"
#include "TrackableObserver.h"

/***************************************************************************
*** 
*** DECLARATIONS
***
****************************************************************************/

/***************************************************************************
*** 
*** PackedPlayer
***
****************************************************************************/

class PackedPlayer
{
public:
  PackedTrackable::IFile	*file;

  uint64_t 			 frame_id;
  PackedTrackable::BinaryFrame 	 lastFrame;
  PackedTrackable::BinaryFrame 	 currentFrame;

  PackedPlayer( const char *fileName=NULL, bool buffered=false )
  : file( NULL ),
    frame_id( 0 )
  {
    if ( fileName != NULL )
      open( fileName, buffered );
  }
  
  ~PackedPlayer()
  { close();
  }

  void start()
  { lastFrame.header.flags    	= 0;
    currentFrame.header.flags 	= 0;
    frame_id			= 0;
  }
  
  bool open( const char *fileName, bool buffered=false )
  { 
    if ( file != NULL )
      delete file;
    
    file = new PackedTrackable::IFile();
    file->is_buffered = buffered;
    
    start();

    return file->open( fileName );
  }
      
  void close()
  { 
    if ( file == NULL )
      return;
    
    delete file;
    file = NULL;
  }
  
  bool is_eof()
  { return file == NULL || file->is_eof();
  }
      
  float playPos() const
  { return file == NULL ? -1 : file->playPos(); }

  uint64_t currentTime() const
  { return file == NULL ? 0 : file->currentTime(); }

  uint64_t timeStamp() const
  { return file == NULL ? 0 : file->timeStamp(); }

  uint64_t play( float time )
  { return file == NULL ? 0 : file->play( time ); }

  PackedTrackable::HeaderType nextHeader( PackedTrackable::Header &header )
  {
    if ( file == NULL )
      return PackedTrackable::Unknown;

    long pos = file->tell();
      
    if ( !file->get( header ) )
    { if ( header.zero != 0 )
	file->seek( pos + sizeof(header.zero) );
      return PackedTrackable::Unknown;
    }

    return (PackedTrackable::HeaderType) header.flags;
  }
  
    
  static bool decodeFrame( ObsvObjects &objects, PackedTrackable::BinaryFrame &frame )
  {
    objects.clear();

    objects.timestamp = frame.header.timestamp;
    objects.uuid      = frame.uuid;

    for ( int i = 0; i < frame.size(); ++i )
    { 
      ObsvObject obsvObject;
      uint16_t flags;
      float x, y, size;
      
      if ( frame.header.isVersion( PackedTrackable::Version2 ) )
      { uint32_t tid;
	frame[i].getV2( tid, x, y, size, flags );
	obsvObject.id = tid;
      }
      else
      { uint16_t tid;
	frame[i].getV1( tid, x, y, size, flags );
	obsvObject.id = tid;
      }

      obsvObject.timestamp = objects.timestamp;
      obsvObject.x 	   = x;
      obsvObject.y    	   = y;
      obsvObject.size 	   = size;
      obsvObject.uuid      = UUID( objects.uuid, obsvObject.id );
      obsvObject.flags     = (flags&(PackedTrackable::Binary::Touched|PackedTrackable::Binary::Private|PackedTrackable::Binary::Latent|PackedTrackable::Binary::Immobile));
      
//      printf( "%d: %d %s %g %g %g\n" , i, (int)obsvObject.id, obsvObject.uuid.str().c_str(), obsvObject.x, obsvObject.y, obsvObject.size );
      objects.emplace(std::make_pair(obsvObject.id,obsvObject));
    }

//    printf( "got: %016lx: %ld %d\n", frame.header.timestamp, frame.size(), g_Track.m_Stage->observer != NULL  );

    objects.validCount = objects.size();

    return true;
  }
  
  bool nextFrame( PackedTrackable::BinaryFrame &frame, PackedTrackable::Header &header )
  {
    frame.clear();

    if ( file == NULL )
      return false;

    frame.header = header;
    if ( !file->get( frame, true ) )
      return false;

    lastFrame    = currentFrame;
    currentFrame = frame;

    return true;
  }
  

  bool nextFrame( ObsvObjects &objects, PackedTrackable::Header &header )
  {
    objects.clear();

    PackedTrackable::BinaryFrame frame;
    
    if ( !nextFrame( frame, header ) )
      return false;

    objects.frame_id   = ++frame_id;
    objects.timestamp  = frame.header.timestamp;

    return decodeFrame( objects, frame );
  }
  

  bool nextFrame( ObsvObjects &objects )
  {
    PackedTrackable::Header     header;
    PackedTrackable::HeaderType type = nextHeader( header );

    while ( !is_eof() && !header.isType( PackedTrackable::FrameHeader ) )
      type = nextHeader( header );

    if ( !header.isType( PackedTrackable::FrameHeader ) )
      return false;
    
    return nextFrame( objects, header );
  }
  

  PackedTrackable::HeaderType grabFrame( PackedTrackable::BinaryFrame &frame, uint64_t timestamp=0 )
  {
    if ( file == NULL )
      return PackedTrackable::Unknown;

    if ( is_eof() )
      return PackedTrackable::Unknown;

    if ( timestamp == 0 )
      timestamp = getmsec();

    file->current_time = timestamp - file->start_time;

    frame.clear();

    PackedTrackable::Header header;

    while ( 1 )
    {
      uint64_t pos = file->tell();
      
      if ( !file->get( header ) )
      { 
	file->sync();
	pos = file->tell();
	if ( !file->get( header ) )
	  return PackedTrackable::Unknown;
      }
       
      if ( !header.isType( PackedTrackable::FrameHeader ) )
	return (PackedTrackable::HeaderType) header.flags;
    
      bool result = nextFrame( frame, header );
      if ( !result )
      { usleep( 100000 );
	return PackedTrackable::Unknown;
      }
      
      if ( frame.header.timestamp >= file->begin_time )
      {
	uint64_t record_time = frame.header.timestamp - file->begin_time;

	if ( record_time >= file->current_time )
        {
	  uint64_t time_diff = record_time - file->current_time;

	  if ( time_diff < 750 )
          { if ( time_diff < 1 )
	      time_diff = 1;
	    usleep( 1000*time_diff-100 );
 
	    file->current_time = getmsec() - file->start_time;

	    return PackedTrackable::FrameHeader;
	  }

	  file->seek( pos );
	  usleep( 10000 );
	  return PackedTrackable::Unknown;
	}
      }
    }

    return PackedTrackable::Unknown;
  }
 
    
  PackedTrackable::HeaderType grabFrame( ObsvObjects &objects, uint64_t timestamp=0 )
  {
    objects.clear();

    PackedTrackable::BinaryFrame frame;
    PackedTrackable::HeaderType type( grabFrame( frame, timestamp ) );
    if ( type != PackedTrackable::FrameHeader )
      return type;
  
    objects.frame_id  = ++frame_id;

    decodeFrame( objects, frame );

    return type;
  }	
     
};


/***************************************************************************
*** 
*** PackedThreadedPlayer
***
****************************************************************************/

class PackedThreadedPlayer
{
  public:

  std::atomic<float>     m_PlayPos;
  std::atomic<int64_t>   m_CurrentTime;
  std::atomic<uint64_t>  m_TimeStamp;
  std::atomic<uint64_t>	 m_TimeStampRef;

  std::atomic<bool>      m_Paused;

  PackedPlayer	    	*m_Player;
  std::mutex		 m_PlayerMutex;
  ObsvObjects 		*m_Objects;
  std::mutex		 m_ObjectMutex;


  std::thread	    	*m_Thread;
  std::atomic<bool>      m_ExitThread;

  PackedThreadedPlayer( const char *fileName, bool buffered=false )
  : m_PlayPos( -1.0 ),
    m_CurrentTime( -1 ),
    m_TimeStamp( 0 ),
    m_TimeStampRef( 0 ),
    m_Paused ( false ),
    m_Player ( NULL ),
    m_Objects( NULL ),
    m_PlayerMutex(),
    m_ObjectMutex(),
    m_Thread ( NULL ),
    m_ExitThread( false )
  {
    if ( fileName != NULL && fileName[0] != '\0' )
    {
      PackedPlayer *player = new PackedPlayer();
      if ( !player->open( fileName, buffered ) )
	std::cerr << "Error opening file " << fileName << "\n";
      else
	setPlayer( player );
    }
  }

  PackedPlayer *packedPlayer() const
  { return m_Player;
  }
  
  void setPlayer( PackedPlayer *player )
  { m_Player 	= player;
    m_TimeStamp = 1;
  }

  void lockPlayer()
  { m_PlayerMutex.lock(); }
    
  void unlockPlayer()
  { m_PlayerMutex.unlock(); }
    
  float playPos() const
  { return m_PlayPos; }

  int64_t currentTime() const
  { return m_CurrentTime;
  }

  uint64_t timeStamp() const
  {
    if ( m_TimeStamp == 0 )
      return 0;

    if ( m_Paused )
      return m_TimeStamp;
  
    return m_TimeStamp + getmsec() - m_TimeStampRef;
  }

  bool isPaused() const
  { return m_Paused; }

  bool packedPlayerAtEnd()
  { return m_Player != NULL && m_Player->is_eof();
  }

  void setPaused( bool paused )
  { 
    if ( paused == m_Paused )
      return;
    
    m_Paused = paused;
    if ( !paused )
      setPlayPos( m_PlayPos );
  }

  void setPlayPos( float playPos )
  {
    m_PlayPos = playPos;
  
    uint64_t now = getmsec();
  
    uint64_t begin_time = 0;

    m_CurrentTime   		= m_Player->play( playPos );
    m_Player->file->start_time 	= now - m_CurrentTime;
    m_PlayPos       		= m_Player->playPos();
    m_TimeStamp     		= m_Player->timeStamp();
    m_TimeStampRef  		= getmsec();
  }

  void setSyncTime( uint64_t timestamp=0 )
  {
    m_PlayPos = 0.0;
  }

  void syncToPlayer()
  {
    m_CurrentTime  = m_Player->currentTime();
    m_TimeStamp    = m_Player->timeStamp();
    m_TimeStampRef = getmsec();
    m_PlayPos      = m_Player->playPos();
  }	

  void run()
  {
    while ( !m_ExitThread )
    {
      ObsvObjects *objects = new ObsvObjects();
      
      m_PlayerMutex.lock();

      PackedTrackable::HeaderType type = m_Player->grabFrame( *objects );

      if ( type != PackedTrackable::Unknown )
	syncToPlayer();
      
      m_PlayerMutex.unlock();

      if ( type == PackedTrackable::FrameHeader )
      {
	m_ObjectMutex.lock();

	if ( m_Objects != NULL )
	  delete m_Objects;
	m_Objects = objects;

	m_ObjectMutex.unlock();
	usleep( 10000 );
      }
      else
	delete objects;
    }
  }

  static void runThread( PackedThreadedPlayer *player )
  {
    player->run();
  }

  ObsvObjects *grabFrame( bool waitForFrame=false )
  {
    if ( m_Player == NULL )
      return NULL;

    if ( m_Thread == NULL )
      m_Thread = new std::thread( runThread, this );  

    m_ObjectMutex.lock();

    if ( waitForFrame && m_Objects == NULL )
    { uint64_t now = getmsec();  
      while ( m_Objects == NULL )
      { m_ObjectMutex.unlock();
	if ( getmsec() - now > 500000 )
	  return NULL;
	usleep( 1000 );
	m_ObjectMutex.lock();
      }
    }
  
    if ( m_Objects == NULL )
    { m_ObjectMutex.unlock();
      return NULL;
    }
  
    ObsvObjects *objects = m_Objects;
    m_Objects = NULL;
    m_ObjectMutex.unlock();
    
    return objects;
  }

};

#endif

