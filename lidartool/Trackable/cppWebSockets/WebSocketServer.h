/** --------------------------------------------------------------------------
 *  WebSocketServer.h
 *
 *  Base class that WebSocket implementations must inherit from.  Handles the
 *  client connections and calls the child class callbacks for connection
 *  events like onConnect, onMessage, and onDisconnect.
 *
 *  Author    : Jason Kruse <jason@jasonkruse.com> or @mnisjk
 *  Copyright : 2014
 *  License   : BSD (see LICENSE)
 *  --------------------------------------------------------------------------
 **/

#ifndef _WEBSOCKETSERVER_H
#define _WEBSOCKETSERVER_H
#include <stdint.h>
#include <map>
#include <set>
#include <string>
#include <list>
#include <stdio.h>
#include <ctime>
#include <sys/time.h>
#include <iostream>
#include <sstream>
#include "libwebsockets.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/// WebSocketServer
/// ---------
class WebSocketServer
{
public:
    // Represents a client connection
    struct Connection
    {
        std::vector<uint8_t>       readBuffer; 
        list<std::vector<uint8_t>> buffer;     // Ordered list of pending messages to flush out when socket is writable
        map<string,string> keyValueMap;
        time_t             createTime;
    };

    // Manages connections. Unfortunately this is public because static callback for
    // libwebsockets is defined outside the instance and needs access to it.
    map<int,Connection*> connections;
    set<int>             connectionsToRemove;

    bool		 _binary;
    

    // Constructor / Destructor
    WebSocketServer( int port, const string certPath = "", const string& keyPath = "", bool binary=false );
    ~WebSocketServer( );

    void 	wait	 ( uint64_t timeout = 50     );
    void 	send	 ( int 	    socketID, const char *data, int length 	);
    void 	broadcast( const char *data, int length   		);

    // Key => value storage for each connection
    string getValue( int socketID, const string& name );
    void   setValue( int socketID, const string& name, const string& value );
    int    getNumberOfConnections( );

    // Overridden by children
    virtual void onConnect   ( int socketID                        ) = 0;
    virtual void onMessage   ( int socketID, const string& data    ) = 0;
    virtual void onDisconnect( int socketID                        ) = 0;
    virtual void onError     ( int socketID, const string& message ) = 0;


    // Wrappers, so we can take care of some maintenance
    void onConnectWrapper(    int socketID, const char *remoteIP );
    void onDisconnectWrapper( int socketID );
    void onErrorWrapper( int socketID, const string& message );

protected:
    // Nothing, yet.

private:
    int                  _port;
    string               _keyPath;
    string               _certPath;
    struct lws_context  *_context;

    void _removeConnection( int socketID );
    void _deleteRemovedConnections();
};

// WebSocketServer.h
#endif
