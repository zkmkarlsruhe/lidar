#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

DIR="$(realpath -s $(dirname $0))"
cd $DIR

model="$(./raspiModel.sh)"

operation=

help() {

    echo "usage: $0 [-h] running|stopped|lidarOn|lidarOff|isSupported|failure"

    echo ""
}

if [ "$1" == "" ] ; then
    help
    exit 0
fi

while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
	+h|+help|-h|-help|--help)
        help
        exit 0
        shift # past value
	;;
	running|stopped|lidarOn|lidarOff|isSupported|failure)
        operation="$1"
	shift # past argument
	;;
	*) 
        help
        exit 0
	shift # past argument
	;;
    esac
done

if [ "$operation" == "" ] ; then
    help
    exit 0
fi

USER="$(whoami)"

if [ "$operation" == "isSupported" ] ; then
    if [ "$model" == "RockPiS" ] ; then
    	if [ "$USER" != "root" ] ; then
    	    if $(sudo -n ./$(basename $0) "$operation" 2> /dev/null) ; then
    	    	echo true
    	    else
    	    	echo false
    	    fi
	else
	    echo true
	fi
    else
	echo false
    fi
    exit 0
fi

if [ "$USER" != "root" ] ; then
    if $(./$(basename $0) isSupported) ; then
    	sudo ./$(basename $0) "$operation"
    fi
    exit 0
fi

if [ "$operation" == "running" ] ; then
    echo default-on > /sys/class/leds/rockpis\:blue\:user/trigger
elif [ "$operation" == "lidarOn" ] ; then
    echo none > /sys/class/leds/rockpis\:blue\:user/trigger
elif [ "$operation" == "lidarOff" ] ; then
    echo default-on > /sys/class/leds/rockpis\:blue\:user/trigger
elif [ "$operation" == "stopped" ] ; then
    echo timer > /sys/class/leds/rockpis\:blue\:user/trigger
elif [ "$operation" == "failure" ] ; then
    echo heartbeat > /sys/class/leds/rockpis\:blue\:user/trigger
else
    help
fi


