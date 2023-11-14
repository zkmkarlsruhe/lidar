#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

APPRISE=apprise.mydomain.de
USER=$(whoami)

# if config is undefined, set it to conf
if [ "$conf" == "" ] ; then
    conf=conf
fi

# set log dir in case we want to write to it
if [ -d "/var/log/$conf" ] ; then
    logDir="/var/log/$conf"
elif [ -d "conf/$conf" ] ; then
    logDir="conf/$conf"
else
    logDir="$conf"
fi

# cd to config dir
cd ../lidarconfig-$conf

# Notification type
# start           on start sensors
# stop            on stop  sensors
# run             on run lidar server
# device          on sensor device failure state change

# on start and stop
if [ "$type" == "start" ] || [ "$type" == "stop" ] || [ "$type" == "run" ]; then

    if [ "$runMode" == "production" ] ; then
	curl --silent --output /dev/null -k -X POST -d "{\"body\":\"type=$type\n\nconf=$conf\n\nmessage='$message'\n\nrunMode=$runMode\", \"title\":\"[lidar $conf] ($type,$runMode) $message\"}" -H "Content-Type: application/json" https://$APPRISE/notify/$USER 2>&1 /dev/null
    fi

# on device
elif [ "$type" == "device" ]; then

    curl --silent --output /dev/null -k -X POST -d "{\"body\":\"type=$type\n\nconf=$conf\n\ndevice=$deviceName\n\nsensorIN=$sensorIN\n\nreason='$reason'\n\nurl=$url\n\nrunMode=$runMode\", \"title\":\"[lidar $conf] ($type,$runMode) $deviceName $reason\"}" -H "Content-Type: application/json" https://$APPRISE/notify/$USER 2>&1 /dev/null

fi

