// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef LIDAR_FILE_DRIVER_H
#define LIDAR_FILE_DRIVER_H

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#include "helper.h"
#include "lidarVirtDriver.h"

/***************************************************************************
*** 
*** DEFINES
***
****************************************************************************/

/***************************************************************************
*** 
*** LidarFileDriver
***
****************************************************************************/

class LidarFileStream
{
public:

  enum HeaderType
  {
    NodesHeaderV1 = 0xfefe,
    NodesHeaderV2 = 0xfefd,
    
  };
  
  
  class Header
  {
    public:
    
    uint64_t timestamp;
    uint16_t size;
    uint16_t type;
    
    Header( uint64_t timestamp=0, uint16_t size=0 ) : timestamp( timestamp), size( size ), type( (uint16_t) NodesHeaderV1 )
    {}
    
  };
  

  FILE *file;

  LidarFileStream() : file( NULL )
  {}

  ~LidarFileStream()
  { close();
  }

  bool is_open()
  { return file != NULL;
  }
      
  bool is_eof()
  { return file == NULL || feof( file );
  }
      
  void close()
  { 
    if ( file == NULL )
      return;
	
    fclose( file );
    file = NULL;
  }

  long tell() const
  { return file == NULL ? 0 : ftell( file );
  }

  void seek( long pos ) 
  { if ( file == NULL )
      return;
    fseek( file, pos, SEEK_SET );
  }

  int read( unsigned char *buffer, int size )
  { return fread( (char *) buffer, 1, size, file );
  }

  bool write( const unsigned char *buffer, int size )
  { 
    if ( file == NULL )
      return false;

    if ( fwrite( (char *) buffer, 1, size, file ) != size )
      return false;

    return true;
  }
  
  bool put( const LidarRawSampleBuffer &nodes, uint64_t timestamp=0 )
  { 
    if ( timestamp == 0 )
      timestamp = getmsec();

    Header header( timestamp, nodes.size() );

    if ( !write( (const unsigned char *)&header, sizeof(header) ) )
      return false;
	
    if ( header.size == 0 )
      return true;
	
    return write( (const unsigned char *)&nodes[0], nodes.size()*sizeof(nodes[0]) );
  }
	
  void flush()
  { if ( file == NULL )
      return;
    fflush( file );
  }
  
};
  
class LidarInFile : public LidarFileStream
{
public:
  uint64_t begin_time;
  uint64_t start_time;
  uint64_t current_time;
  long     file_size;
  
  LidarInFile( const char *fileName=NULL, uint64_t reftimestamp=0) : LidarFileStream(), file_size( 0 )
  { if ( fileName != NULL )
    { open( fileName, reftimestamp );
      if ( is_open() )
      { fseek( file, 0L, SEEK_END );
	file_size = ftell( file );
	fseek( file, 0L, SEEK_SET );
      }
    }
    
  }

  ~LidarInFile()
  { close();
  }

  std::string applyDateToString( const char *string, uint64_t timestamp=0 )
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

//    printf( "templateToFileName %s %s %ld\n", buffer, logFileTemplate.c_str(), timestamp );

    return std::string( buffer );
  }

  float playPos() const
  { return file_size == 0 ? 0 : tell() / (float)file_size; }

  uint64_t currentTime() const
  { return current_time; }

  uint64_t timeStamp() const
  { return begin_time + current_time; }

  bool open( const char *fileName, uint64_t reftimestamp=0  )
  { close();
    file = fopen( fileName, "rb" );
    if ( file == NULL )
      return false;
    
    if ( reftimestamp == 0 )
      start_time = getmsec();
    else
      start_time = reftimestamp;

    begin_time = 0;

    Header header;    
    if ( get( header ) )
      begin_time = header.timestamp;
    
    seek( 0 );
    
    return true;
  }

  uint64_t sync()
  {
    if ( file == NULL )
      return 0;

    uint64_t timestamp = 0;

    do 
    {
      Header header;    
      long pos = tell();

      int readsize = read( (unsigned char *)&header, sizeof(header) );
      if ( readsize != sizeof(header) )
	return 0;

      if ( header.type == NodesHeaderV1 || header.type == NodesHeaderV2 )
      { if ( header.size > 0 )
        { seek( pos + sizeof(header) + header.size * sizeof(LidarRawSample) );
	  if ( is_eof() )
	    return 0;
	  
	  Header nextHeader;    
	  int readsize = read( (unsigned char *)&nextHeader, sizeof(nextHeader) );
	  if ( readsize != sizeof(nextHeader) )
	    return 0;
	  if ( nextHeader.type != NodesHeaderV1 && nextHeader.type != NodesHeaderV2 )
	    return 0;
	}
	timestamp = header.timestamp;
	seek( pos );
	break;
      }	
      else
      { seek( pos + sizeof(header.type) );
	if ( is_eof() )
	  return 0;
      }	
    } while( true );
    
    current_time = timestamp - begin_time;
    
    return current_time;
  }
  
  uint64_t play( float time )
  {
    long pos = time * file_size;
    pos -= pos % 2;
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
  
  bool get( Header &header )
  {
    int readsize = read( (unsigned char *)&header, sizeof(header) );
    
    if ( readsize != sizeof(header) || (header.type != NodesHeaderV1 && header.type != NodesHeaderV2) )
      return false;

    return true;
  }

  bool get( LidarRawSampleBuffer &nodes, Header &header )
  { 
    if ( !get( header ) || (header.type != NodesHeaderV1 && header.type != NodesHeaderV2) )
      return false;

    nodes.resize( header.size );

    if ( header.size > 0 )
    {
      int readsize = read( (unsigned char *)&nodes[0], header.size * sizeof(nodes[0]) );
      if ( readsize != header.size * sizeof(nodes[0]) )
	return false;
    }
	
    return true;
  }

  bool grabScanData( LidarRawSampleBuffer &nodes, uint64_t timestamp=0 )
  {
    if ( is_eof() )
      return false;

    if ( timestamp == 0 )
      timestamp = getmsec();

    current_time = timestamp - start_time;

//    printf( "grabScanData(%ld): %ld %ld %ld\n", timestamp, start_time, begin_time, current_time );

    while ( 1 )
    {
      long pos = tell();
      Header header;
      if ( !get( nodes, header ) )
      { usleep( 100000 );
	return false;
      }
      
      if ( header.timestamp >= begin_time )
      {
	int64_t record_time = header.timestamp - begin_time;

//	printf( "%ld: (%ld.%ld)  rec: %ld.%ld\n", (uint64_t) this, current_time/1000, current_time%1000,  record_time/1000, record_time%1000 );
      
	if ( record_time >= current_time )
        {
	  int64_t time_diff = record_time - current_time;

	  if ( time_diff < 750 )
          { if ( time_diff < 1 )
	      time_diff = 1;
	    usleep( 1000*time_diff-100 );
 
	    current_time = getmsec() - start_time;

//	    std::string date( applyDateToString( "%c", header.timestamp ) );
//	    printf( "%ld: begin: %s %03ld\n", (uint64_t) this, date.c_str(), header.timestamp%1000 );

//	    printf( "%ld: (%ld.%ld)  rec: %ld.%ld finish\n", (uint64_t) this, current_time/1000, current_time%1000,  record_time/1000, record_time%1000 );
	    return true;
	  }
//	  printf( "%ld: seek: time_diff: %ld\n", (uint64_t) this, time_diff );
	  seek( pos );
	  usleep( 10000 );
	  return false;
	}
      }
    }

    return false;
  }
  
};


class LidarOutFile : public LidarFileStream
{
public:

  LidarOutFile( const char *fileName=NULL ) : LidarFileStream()
  { if ( fileName != NULL )
      open( fileName );
  }

  bool open( const char *fileName )
  { close();
    file = fopen( fileName, "wb" );
    return file != NULL;
  }
      
};

#endif
