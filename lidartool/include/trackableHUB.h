// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef TRACKABLE_HUB_H
#define TRACKABLE_HUB_H

#include <map>
#include "UUID.h"

#include "helper.h"
#include "keyValueMap.h"

#include "lidarTrackable.h"
#include "packedPlayer.h"
#include "TrackableObserver.h"

/***************************************************************************
***
*** TrackableHUB
***
****************************************************************************/

class TrackableHUB 
{
public:
  
    bool m_IsConnected;
    uint64_t m_DiscardTime;

    uint64_t lastConnectionTime;

    std::string		m_Host;
    int			m_Port;
    


    TrackableHUB();

    void    setEndpoint( const char *host="localhost", int port=5000 );

    bool    decodeFrame ( PackedTrackable::BinaryFrame &frame );
    bool    decodePacket( unsigned char *in, size_t len );

    bool    update      ();

    static bool 		 parseArg( int &i, const char *argv[], int &argc );
    static TrackableHUB 	*instance();
    static void			 setVerbose( int level );
    static void		 	 observe   ( PackedTrackable::Header      &header );
    static void		 	 observe   ( PackedTrackable::BinaryFrame &frame );
	   

};


#endif // TRACKABLE_HUB_H
