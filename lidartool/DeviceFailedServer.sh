#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

conf="$3"

if [ "$conf" == "" ] ; then
    conf=$(cat configDir.txt)
fi

logDir="$conf"
if [ -d "/var/log/$conf" ] ; then
        logDir="/var/log/$conf"
fi

echo `date` $1 \"$2\" >> $logDir/deviceFailures.log

if [ -f $conf/DeviceFailedNotification.sh ] ; then
    ./$conf/DeviceFailedNotification.sh "$1" "$2" "$conf"
fi

if [ "$2" == "ok" ] ; then
    exit 0
fi

nodeId=$(./lidarTool +listNikNames | grep $1 | cut -d ' ' -f3- | cut -d ':' -f2 | cut -c 3-)

if [ "$nodeId" != "" ] ; then

    cd ../lidarconfig-$conf

    runMode="setup"
    if [ -f LidarRunMode.txt ] ; then 
	runMode=$(cat LidarRunMode.txt)
    fi

    if [ "$runMode" == "production" ] ; then
	./manageNodes.sh node $nodeId reboot
    fi
fi


