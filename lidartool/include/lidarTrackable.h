// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
// Bernd Lintermann <bernd.lintermann@zkm.de>
//
// BSD Simplified License.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE" in this distribution.
//

#ifndef LIDAR_TRACKABLE_H
#define LIDAR_TRACKABLE_H

#include "keyValueMap.h"

#include "TrackableObserver.h"
#include "TrackableImageObserver.h"

#if USE_LIBLO
#include "TrackableOSCObserver.h"
#endif

#if USE_WEBSOCKETS
#include "TrackableWebSocketObserver.h"
#endif

#if USE_MOSQUITTO
#include "TrackableMQTTObserver.h"
#endif

#if USE_LUA
#include "TrackableLuaObserver.h"
#endif

#if USE_INFLUXDB
#include "TrackableInfluxDBObserver.h"
#endif


#include "Trackable.h"


#endif // LIDAR_TRACKABLE_H
