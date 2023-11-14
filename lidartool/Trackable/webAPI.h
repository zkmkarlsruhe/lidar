// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef CURL_TOOL_H 
#define CURL_TOOL_H

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#include "string.h"
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <curl/curl.h>

#include <condition_variable>

/***************************************************************************
*** 
*** semaphore
***
****************************************************************************/

class semaphore {
private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;

public:
    semaphore (int count_ = 0)
        : count(count_) {}

    inline void release()
    {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        cv.notify_one();
    }

    inline void acquire()
    {
        std::unique_lock<std::mutex> lock(mtx);

        while(count == 0){
            cv.wait(lock);
        }
        count--;
    }

};

/***************************************************************************
*** 
*** DECLARATIONS
***
****************************************************************************/

/***************************************************************************
*** 
*** WebAPI
***
****************************************************************************/

class WebAPI
{
public:
    std::thread		*m_Thread;
    semaphore		 m_Sema;
    std::mutex	  	 m_Mutex;
    bool		 exitThread;
public:
    
    std::vector<uint8_t> m_PostData;
    std::vector<uint8_t> m_ReturnData;
    bool		 m_HasResponded;
    std::string	         m_URL;
    bool	    	 m_Ready;
    bool		 m_Verbose;
    bool		 m_MethodIsPost;
    bool   		 postCurl();
    bool   		 getCurl();

    std::vector<std::string> m_Headers;
    CURL                *curl;
    char                 errbuf[CURL_ERROR_SIZE];
    FILE                *errorFP;
public:
  
    void   		 threadFunction();
    void 		 stopThread();

    size_t 		 readMemory ( void *dest,     size_t size, size_t nmemb );
    size_t 		 writeMemory( void *contents, size_t size, size_t nmemb );
   
    void		 lock()    { if ( m_Thread != NULL ) m_Mutex.lock();   }
    void		 unlock()  { if ( m_Thread != NULL ) m_Mutex.unlock(); }
    void		 acquire() { if ( m_Thread != NULL ) m_Sema.acquire(); }
    void		 release() { if ( m_Thread != NULL ) m_Sema.release(); }


public:

    WebAPI( bool verbose=false );
    ~WebAPI();

    void		 setThreaded( bool set );

    bool 		 post( const char *url );
    bool 		 get ( const char *url );
    bool   		 post( const char *data, int size, const char *url );

    std::vector<uint8_t> &postData();
    std::vector<uint8_t> &returnData();
    std::string		  returnDataStr();

    bool   		 hasReturnData();
    bool   		 isReady();
    bool   		 hasResponded();
    void   		 clearReturnData();
    
    void		 addHeader( const char *header );

    void		 setVerbose( bool verbose );

};

 
#endif // CURL_TOOL_H

