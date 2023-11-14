// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef _LIDAR_TRACK_H_
#define _LIDAR_TRACK_H_

/***************************************************************************
*** 
*** INCLUDES
***
****************************************************************************/

#include "helper.h"

#include "lidarTrackable.h"
#include "lidarKit.h"
#include "TrackBase.h"

/***************************************************************************
*** 
*** DECLARATIONS
***
****************************************************************************/

/***************************************************************************
*** 
*** LidarTrack
***
****************************************************************************/

class LidarTrack : public TrackBase
{
public:

    std::string					  outFormat;

    void	mergeStages ( LidarDevices &devices, uint64_t timestamp );
    void	mergeObjects( LidarDevices &devices, uint64_t timestamp );

    LidarTrack();

    void	track( LidarDevices &devices, uint64_t timestamp=0 );
    void        updateOperational( std::set<std::string> &availableDevices, TrackableObserver *observer=NULL );

    void	start( uint64_t timestamp=0, LidarDevice *device=NULL );
    void	stop ( uint64_t timestamp=0, LidarDevice *device=NULL );
    void	startAlwaysObserver( uint64_t timestamp=0 );   

    void	exit();

    static void	setRadialDisplacement( float displace );
    
};


#endif

