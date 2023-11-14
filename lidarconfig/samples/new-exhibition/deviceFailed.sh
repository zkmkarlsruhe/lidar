#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

# if config is undefined, we are wrong here
if [ "$conf" == "" ] ; then
    echo ERROR node conf defined
    exit 0
fi

if [ "$runMode" == "" ] || [ "$runMode" == "unknown" ] ; then 
    echo no runMode defined
    exit 0
fi

# set log dir, we want to log to a file
if [ -d "/var/log/$conf" ] ; then
    logDir="/var/log/$conf"
elif [ -d "conf/$conf" ] ; then
    logDir="conf/$conf"
else
    logDir="$conf"
fi

echo "$(date): $deviceName \"$reason\"" >> $logDir/deviceFailures.log

if [ "$reason" == "ok" ] ; then
    exit 0
fi

echo conf=$conf deviceName=$deviceName >> /tmp/log

nodeId=$(./lidarTool +conf $conf +listNikNames | grep $deviceName | cut -d ' ' -f3- | cut -d ':' -f2 | cut -c 3-)

if [ "$nodeId" != "" ] ; then

    cd ../lidarconfig-$conf

    if [ "$runMode" == "production" ] ; then
	if $verbose ; then
	    echo ./manageNodes.sh node $nodeId reboot
	fi
	./manageNodes.sh node $nodeId reboot 2>&1 /dev/null
    fi
fi


