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

#include "webAPI.h"

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

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  WebAPI *webAPI = (WebAPI *) userp;
  return webAPI->writeMemory( contents, size, nmemb );
}

static size_t
ReadMemoryCallback(void *dest, size_t size, size_t nmemb, void *userp)
{
  WebAPI *webAPI = (WebAPI *) userp;
  return webAPI->readMemory( dest, size, nmemb );
}

/***************************************************************************
*** 
*** WebAPI
***
****************************************************************************/

static void
runThread( WebAPI *webAPI )
{
  while( !webAPI->exitThread )
    webAPI->threadFunction();
}


void
WebAPI::stopThread()
{ if ( m_Thread == NULL )
    return;

  exitThread = true;
  m_Thread->join();
  delete m_Thread;
  m_Thread = NULL;
}

WebAPI::WebAPI( bool verbose )
  : 
    exitThread ( false ),
    m_Thread   ( NULL ),
    m_Sema     (),
    m_Mutex    (),
    m_URL      (),
    m_Ready    ( true ),
    m_Verbose  ( verbose ),
    m_HasResponded( false ),
    curl       ( curl_easy_init() ),
    errorFP    ( fopen("/dev/null", "wb") )
{
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
  curl_easy_setopt(curl, CURLOPT_STDERR, errorFP);
  errbuf[0] = 0;

  curl_easy_setopt(curl, CURLOPT_READFUNCTION, ReadMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_READDATA, (void *)this);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback); 
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)this); 

//  m_Thread = new std::thread( runThread, this );  
}

WebAPI::~WebAPI()
{
  stopThread();

  curl_easy_cleanup( curl );

  fclose( errorFP );
}

void
WebAPI::setThreaded( bool set )
{
  if ( set )
  { if ( m_Thread == NULL )
      m_Thread = new std::thread( runThread, this );
  }
  else
    stopThread();
}


void
WebAPI::setVerbose( bool verbose )
{
  lock();
  m_Verbose = verbose;
  unlock();
}

void
WebAPI::addHeader( const char *header )
{
  m_Headers.push_back( std::string(header) );
}

void
WebAPI::threadFunction()
{
  acquire();

  if ( m_MethodIsPost )
    postCurl();
  else
    getCurl();

  lock();
  m_Ready  = true;
  unlock();
}

bool
WebAPI::isReady()
{
  lock();
  bool ready = m_Ready;
  unlock();

  return ready;
}

bool
WebAPI::hasReturnData()
{
  lock();
  bool ready = m_Ready;
  int  size = m_ReturnData.size();
  unlock();

  if ( !ready )
    return false;
  
  return size > 0;
}

bool
WebAPI::hasResponded()
{
  lock();
  bool ready = m_Ready;
  bool hasResponded = m_HasResponded;
  unlock();

  if ( !ready )
    return false;
  
  return hasResponded;
}


std::vector<uint8_t> &
WebAPI::postData()
{
  return m_PostData;
}

std::vector<uint8_t> &
WebAPI::returnData()
{
  return m_ReturnData;
}

std::string
WebAPI::returnDataStr()
{
  std::string result;
  result.reserve( m_ReturnData.size() + 1 );

  memcpy( &result[0], &m_ReturnData[0], m_ReturnData.size() );
  
  result[m_ReturnData.size()] = '\0';

  return result;
}


void
WebAPI::clearReturnData()
{
  m_ReturnData.resize( 0 );
  m_HasResponded = false;
}


size_t
WebAPI::readMemory( void *dest, size_t size, size_t nmemb )
{
  size_t buffer_size = size*nmemb;
 
  if( m_PostData.size() > 0 ) 
  {
    /* copy as much as possible from the source to the destination */ 
  
    size_t copy_this_much = m_PostData.size();
    if ( copy_this_much > buffer_size )
      copy_this_much = buffer_size;

    memcpy( dest, &m_PostData[0], copy_this_much );
    
    size_t remain = m_PostData.size() - copy_this_much;
    if ( remain > 0 )
    { uint8_t tmp[remain];
      memcpy( &tmp[0], &m_PostData[copy_this_much], remain );
      memcpy( &m_PostData[0], &tmp[0], remain );
    }

    m_PostData.resize( remain );
	      
    return copy_this_much; /* we copied this many bytes */ 
  }
 
  return 0; /* no more data left to deliver */ 
}

size_t
WebAPI::writeMemory(void *contents, size_t size, size_t nmemb )
{
  size_t realsize = size * nmemb;
  
  m_ReturnData.resize( m_ReturnData.size() + realsize );
  memcpy( &m_ReturnData[m_ReturnData.size()-realsize], contents, realsize );

  return realsize;
}


bool
WebAPI::post( const char *url )
{
  if ( m_Thread == NULL )
  { m_URL = url;
    postCurl();
  }
  else
  {
    lock();

    m_MethodIsPost = true;
  
    m_URL = url;
  
    m_Ready = false;

    unlock();

    release();
  }
  
  return true;
}

bool
WebAPI::get( const char *url )
{
  if ( m_Thread == NULL )
  { m_URL = url;
    getCurl();
  }
  else
  {
    lock();
  
    m_MethodIsPost = false;
  
    m_URL = url;
  
    m_Ready = false;

    unlock();

    release();
  }
  
  return true;
}

bool
WebAPI::post( const char *data, int size, const char *url )
{
  m_PostData.resize( size );
  
  memcpy( &m_PostData[0], data, size );

  return post( url );
}

bool
WebAPI::getCurl()
{
  CURLcode res;
  errbuf[0] = 0;

  lock();
  m_HasResponded = false;
  m_ReturnData.resize( 0 );
  unlock();

  curl_easy_setopt(curl, CURLOPT_VERBOSE, m_Verbose);
  curl_easy_setopt(curl, CURLOPT_URL, m_URL.c_str() );
  curl_easy_setopt(curl, CURLOPT_POST, 0L);

  struct curl_slist *headers = NULL;
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    if ( m_Verbose )
      fprintf(stderr, "curl_easy_perform(%s) failed: %s\n", m_URL.c_str(), curl_easy_strerror(res));
    return false;
  }

  lock();
  m_HasResponded = true;
  unlock();

  return true;
}

bool
WebAPI::postCurl()
{
  const char *token = "c0955d6990d248019fd4aaa34a241b6c";
  CURLcode res;

  errbuf[0] = 0;

  lock();
  m_HasResponded = false;
  m_ReturnData.resize( 0 );
  unlock();

  curl_easy_setopt(curl, CURLOPT_VERBOSE, m_Verbose);
  curl_easy_setopt(curl, CURLOPT_URL, m_URL.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)m_PostData.size());

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
  headers = curl_slist_append(headers, "Expect:");

  for ( int i = 0; i < m_Headers.size(); ++i )
    headers = curl_slist_append(headers, m_Headers[i].c_str() );

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    if ( m_Verbose )
      fprintf(stderr, "curl_easy_perform(%s) failed: %s\n", m_URL.c_str(), curl_easy_strerror(res));
    return false;
  }

  lock();
  m_HasResponded = true;
  unlock();

  return true;
}

