// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef PACKED_TRACKABLE_H
#define PACKED_TRACKABLE_H

#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <unistd.h>
#include <vector>
#include <filesystem>
#include "UUID.h"
#include "helper.h"

#include <string.h>
#include <assert.h>

namespace PackedTrackable {
 
  inline uint64_t getmsec() 
  { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  }

  enum HeaderType
  {
    TypeBits      = (0xff),

    Unknown       = 0,
    FrameHeader   = 1,
    StartHeader   = 2,
    StopHeader    = 3,

    VersionBits   = (0xff00),

    Version1      = (0<<8),
    Version2      = (1<<8)
  };
  
  typedef HeaderType HeaderVersion;

  struct Header
  {
    uint32_t	zero;
    uint16_t	flags;
    union 	{ uint16_t size; };
    uint64_t	timestamp;

    Header( uint64_t tstamp=0, uint16_t flags=FrameHeader )
      : zero     ( 0 ),
	flags    ( flags | Version2 ),
	size     ( 0 ),
	timestamp( (tstamp&0xffffffff) == 0 ? (tstamp|1) : tstamp )
      {}

    inline bool	isType( HeaderType type ) const
    { return (flags&TypeBits) == type; }
    
    inline bool	isVersion( HeaderVersion version ) const
    { return (flags&VersionBits) == version; }
    
    inline HeaderVersion version() const
    { return (HeaderVersion) (flags & VersionBits); } 
    
    inline bool	timeStampValid() const
    { return timestamp > 1; }
    
  };
  
  struct Binary
  {
    union 	{ 
      struct 
      {
	uint16_t	tid;
	int16_t		x;
	int16_t		y;
	uint16_t	size;
	uint16_t	flags;
	uint16_t	one;
      } v1;
      
     struct 
      {
	uint32_t	tid;
	int16_t		x;
	int16_t		y;
	uint16_t	size;
	uint16_t	flags;
      } v2;
      
      
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

    Binary()
    {}
    
    Binary( uint32_t tid, float x, float y, float size, uint16_t flags )
      : v2 { tid, (int16_t)(x * 100), (int16_t)(y * 100), (uint16_t)(size * 100), flags }
    {
      if ( this->v2.x    == 0 ) this->v2.x    = 1;
      if ( this->v2.y    == 0 ) this->v2.y    = 1;
      if ( this->v2.size == 0 ) this->v2.size = 1;
//      printf( ": %04x %04x %04x %04x\n" , this->v2.tid, this->v2.x, this->v2.y, this->v2.size );
    }
    
    void getV1( uint16_t &tid, float &x, float &y, float &size, uint16_t &flags )
    {
      tid   = this->v1.tid;
      x     = this->v1.x    / 100.0;
      y     = this->v1.y    / 100.0;
      size  = this->v1.size / 100.0;
      flags = this->v1.flags;
    }

    void getV2( uint32_t &tid, float &x, float &y, float &size, uint16_t &flags )
    {
      tid   = this->v2.tid;
      x     = this->v2.x    / 100.0;
      y     = this->v2.y    / 100.0;
      size  = this->v2.size / 100.0;
      flags = this->v2.flags;
//      printf( ": %04x %04x %04x %04x\n" , this->v2.tid, this->v2.x, this->v2.y, this->v2.size );
//      printf( ": %04x %g %g %g\n" , tid, x, y, size );
    }

   };
  
  class BinaryFrame : public std::vector<Binary>
  {
    public:

    Header		header;
    UUID		uuid;

    BinaryFrame() : header( 0 )
    {}

    BinaryFrame( uint64_t tstamp, const UUID &uuid ) : header( tstamp, FrameHeader ), uuid( uuid )
    { if ( header.timestamp == 0 )
      { header.timestamp = getmsec();
	if ( (header.timestamp&0xffffffff) == 0 )
	  header.timestamp |= 1;
      }
    }

    void add( uint16_t tid, float x, float y, float size, uint16_t flags )
    { push_back( Binary( tid, x, y, size, flags ) );
    }
  };

  class Stream
  {
    public:

      Stream() 
      {}

      virtual bool	write( const unsigned char *buffer, int size ) = 0;
      virtual int	read ( unsigned char *buffer, int size ) = 0;
      
      bool flush( const unsigned char *buffer, int size )
      { return write( buffer, size );
      }
      
      bool put( const struct Binary &binary )
      { return flush( (const unsigned char *)&binary, sizeof(binary) );
      }

      bool put( uint16_t tid, float x, float y, float size, uint16_t flags )
      { struct Binary binary( tid, x, y, size, flags );
	return put( binary );
      }

      bool put( const UUID &uuid )
      { return flush( (const unsigned char *)&uuid, sizeof(uuid) );
      }

      bool put( Header &header )
      { 
	return flush( (const unsigned char *)&header, sizeof(header) );
      }

      bool put( BinaryFrame &frame )
      { 
	frame.header.size = frame.size();
	if ( !write( (const unsigned char *)&frame.header, sizeof(frame.header) ) )
	  return false;
	
	if ( !write( (const unsigned char *)&frame.uuid, sizeof(frame.uuid) ) )
	  return false;
	
	if ( frame.size() == 0 )
	  return true;
	
	return flush( (const unsigned char *)&frame[0], frame.size()*sizeof(frame[0]) );
      }

      bool get( Header &header )
      {
	int readsize = read( (unsigned char *)&header, sizeof(header) );
	return readsize == sizeof(header) && header.zero == 0;
      }

      bool get( UUID &uuid )
      {
	int readsize = read( (unsigned char *)&uuid, sizeof(uuid) );
	return readsize == sizeof(uuid);
      }

      bool get( Binary &binary )
      { int readsize = read( (unsigned char *)&binary, sizeof(binary) );
	return readsize == sizeof(binary);
      }

      bool get( BinaryFrame &frame, bool skipHeader=false )
      { 
	if ( !skipHeader )
	{ if ( !get( frame.header ) || !frame.header.isType(FrameHeader) )
	    return false;
	}

	if ( !get( frame.uuid ) )
	  return false;

	Binary binary;
	for ( int i = 0; i < frame.header.size; ++i )
	{ if ( !get( binary ) )
	    return false;
	  frame.push_back( binary );
	}

	return true;
      }
	
  };
  
  class IFile : public Stream
  {
    public:
      FILE *file;
      std::vector<int8_t> buffer;
      bool     is_buffered;
      int64_t  bufferPos;
      
      uint64_t begin_time;
      uint64_t start_time;
      uint64_t current_time;
      long     file_size;
  
      IFile( const char *fileName=NULL, uint64_t reftimestamp=0, bool buffered=true ) : Stream(), file( NULL ), is_buffered(buffered), bufferPos( -1 ), current_time( 0 ), file_size( 0 )
      { if ( fileName != NULL )
	{ open( fileName, reftimestamp );
	}
      }

      ~IFile()
      { close();
      }

      bool is_open() const
      { return is_buffered ? bufferPos >= 0 : file != NULL;
      }
      
      bool is_eof() const
      { return is_buffered ? (bufferPos < 0 || bufferPos >= file_size) : (file == NULL || feof( file ));
      }
      
      float playPos() const
      { return file_size == 0 ? 0 : tell() / (float)file_size; }

      uint64_t currentTime() const
      { return current_time; }

      uint64_t timeStamp() const
      { return begin_time + current_time; }

      bool reopen()
      {
	if ( !is_buffered )
	  return false;
	
	current_time = 0;
	
	seek( 0 );

	return true;
      }
      
      bool openBuffer( const char *buffer, uint64_t size, uint64_t reftimestamp=0 )
      { close();

	if ( size == 0 )
	  return false;
	
	is_buffered = true;
	
	if ( reftimestamp == 0 )
	  start_time = getmsec();
	else
	  start_time = reftimestamp;

	begin_time = 0;

	file_size = size;
	this->buffer.resize( file_size );

	memcpy( &this->buffer[0], buffer, file_size );
	bufferPos = 0;
    
	bool success = true;
	
	Header header;    
	while ( begin_time == 0 && (success=get( header )) )
	  begin_time = header.timestamp;

	seek( 0 );

	return true;
      }
      
      bool open( const char *fileName, uint64_t reftimestamp=0 )
      { close();
	file = fopen( fileName, "rb" );
	if ( file == NULL )
	  return false;
    
	if ( reftimestamp == 0 )
	  start_time = getmsec();
	else
	  start_time = reftimestamp;

	begin_time = 0;

	bool success = true;
	
	fseek( file, 0L, SEEK_END );
	file_size = ftell( file );
	fseek( file, 0L, SEEK_SET );

	if ( is_buffered )
	{ buffer.resize( file_size );
	  success = (fread( (char *) &buffer[0], 1, file_size, file ) == file_size);
	  fclose( file );
	  file      = NULL;
	  if ( !success )
	    return false;
	  bufferPos = 0;
	}
    
	Header header;    
	while ( begin_time == 0 && (success=get( header )) )
	  begin_time = header.timestamp;

	seek( 0 );

	return success;
      }
      
      void close()
      { 
	file_size = 0;
	bufferPos = -1;
	buffer.resize( 0 );
	
	if ( file == NULL )
	  return;
	
	fclose( file );
	file = NULL;
      }

      long tell() const
      { 
	if ( !is_open() )
	  return 0;
	return is_buffered ? bufferPos : ftell( file );
      }

      void seek( long pos )
      { 
	if ( !is_open() )
	  return;

	if ( is_buffered )
	  bufferPos = pos;
	else
	  fseek( file, pos, SEEK_SET );
      }

      virtual int read( unsigned char *buffer, int size )
      { 
	if ( !is_open() )
	  return -1;
	
	if ( bufferPos+size > file_size )
	  size = file_size - bufferPos;
	
	if ( size <= 0 )
	  return -1;
	
	if ( !is_buffered )
	  return fread( (char *) buffer, 1, size, file );
	
	memcpy( buffer, &this->buffer[bufferPos], size );

	bufferPos += size;
	
	return size;
      }

      virtual bool write( const unsigned char *buffer, int size )
      { return false;
      }

      uint64_t sync()
      {
	if ( !is_open() )
	  return 0;

	uint64_t timestamp = 0;

	do 
        {
	  Header header;    
	  long pos = tell();

	  int readsize = read( (unsigned char *)&header, sizeof(header) );
	  if ( readsize != sizeof(header) )
	    return 0;

	  if ( header.zero == 0 && 
	       (header.isType( PackedTrackable::FrameHeader ) ||
		header.isType( PackedTrackable::StartHeader )) )
	  { timestamp = header.timestamp;
	    seek( pos );
	    break;
	  }

	  seek( pos + sizeof(header.zero) );
	  if ( is_eof() )
	    return 0;

	} while( true );
    
	current_time = timestamp - begin_time;
    
	return current_time;
      }
  
      uint64_t play( float time )
      {
	long pos = time * file_size;
	pos -= pos % 4;
	seek( pos );

	return sync();
      }
  
      uint64_t sync( uint64_t play_time )
      {
	double ltime = 0.0;
	double rtime = 1.0;
	long lastPos = -1;

	while ( ltime < rtime )
        {
	  double time = 0.5 * (rtime+ltime);

	  uint64_t t = play( time );
	  if ( t == 0 )
	    return t;
      
	  long pos = tell();
	  if ( pos == lastPos )
	    return t;
      
	  lastPos = pos;

	  if ( t > play_time )
	    rtime = time;
	  else if ( t < play_time )
	    ltime = time;
	  else
	    return t;
	}

	return current_time;
      }
  
  };
  
  class OFile : public Stream
  {
    public:
      FILE *file;

      OFile( const char *fileName=NULL ) : Stream(), file( NULL )
      { if ( fileName != NULL )
	  open( fileName );
      }

      ~OFile()
      { close();
      }

      bool is_open()
      { return file != NULL;
      }
      
      bool is_eof()
      { return file == NULL || feof( file );
      }
      
      bool open( const char *fileName )
      { close();
	
	std::string path( filePath( fileName ) );
	if ( !path.empty() && !fileExists( path.c_str() ) )
	  std::filesystem::create_directories( path.c_str() );

	file = fopen( fileName, "ab" );
	return file != NULL;
      }
      
      void close()
      { 
	if ( file == NULL )
	  return;
	
	fclose( file );
	file = NULL;
      }

      virtual int read( unsigned char *buffer, int size )
      { return false;
      }

      virtual bool write( const unsigned char *buffer, int size )
      { 
	if ( file == NULL )
	  return false;

	if ( fwrite( (char *) buffer, 1, size, file ) != size )
	  return false;

	fflush( file );

	return true;
      }
      
  };
  
  

} // namespace PackedTrackable

#endif // PACKED_TRACKABLE_H
