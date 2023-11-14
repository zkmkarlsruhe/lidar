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

configFile="LidarPower.conf"
enableFile="LidarPower.enable"

onLevel=1
offLevel=0
enabled=true

model="$(./raspiModel.sh)"

if test -f "$configFile" ; then
    entry=$(cat $configFile)

    GPIO=$(echo $entry | awk '{print $1}')
    onLevel=$(echo $entry | awk '{print $2}')
    offLevel=$(echo $entry | awk '{print $3}')
fi

if test -f "$enableFile" ; then
    enabled=$(cat $enableFile)
fi

operation=

help() {

    echo "usage: $0 [-h] [+gpio GPIO] [+onHi] [+onLo] on|off|isSupported|isEnabled|isOn|isOff|init|exit|enable|disable"
    echo ""
    echo "  GPIO     = $GPIO"
    echo "  onLevel  = $onLevel"
    echo "  offLevel = $offLevel"

    echo ""
    echo "set persistent GPIO value with:"
    echo "  $0 +gpio GPIO [+onHi|+onLo] init"

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
	+gpio)
        GPIO=$2
	shift # past argument
	shift # past value
	;;
	+onHi)
        onLevel=1
        offLevel=0
	shift # past argument
	;;
	+onLo)
        onLevel=0
        offLevel=1
	shift # past argument
	;;
	on|off|init|exit|enable|disable|isSupported|isEnabled|isOn|isOff)
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

gpioDir="/sys/class/gpio/gpio$GPIO"

USER="$(whoami)"

if [ "$operation" == "isEnabled" ] ; then
    echo $enabled
    exit 0
fi

if [ "$operation" == "enable" ] ; then
    echo true > "$enableFile"
    exit 0
fi

if [ "$operation" == "disable" ] ; then
    echo false > "$enableFile"
    exit 0
fi

if [ "$operation" == "isSupported" ] ; then
    
    if [ "$USER" != "root" ] ; then
	if ! $enabled ; then
	    echo false
	elif $(sudo -n $DIR/$(basename $0) "$operation" 2> /dev/null) ; then
	   echo true
	else
    	   echo false
    	fi
    elif [ "$GPIO" != "" ] && [ -d "$gpioDir" ] ; then
	echo true
    else
	echo false
    fi
    exit 0
fi

if [ "$USER" != "root" ] ; then

    if $($DIR/$(basename $0) isSupported) ; then
    	sudo $DIR/$(basename $0) "$operation"
    fi
    exit 0
fi

if [ "$operation" == "init" ] ; then

    if [ "$GPIO" == "" ] && [ "$model" == "RockPiS" ] ; then
	GPIO=69
    fi

echo "init $GPIO $onLevel $offLevel" >> /tmp/boot.txt

    echo $GPIO $onLevel $offLevel > $configFile

    if [ ! -d "$gpioDir" ] ; then
	echo $GPIO > /sys/class/gpio/export
	echo out > $gpioDir/direction
	
	echo "creating $gpioDir" >> /tmp/boot.txt
    fi

    echo $onLevel > $gpioDir/value

    exit 0
fi

if [ "$GPIO" == "" ] ; then
    echo powering usupported
    exit 0
fi

if [ "$operation" == "isOn" ] ; then
    if [ "$(cat $gpioDir/value)" == "$onLevel" ] ; then
	echo true
    else
	echo false
    fi
    exit 0
fi

if [ "$operation" == "isOff" ] ; then
    if [ "$(cat $gpioDir/value)" == "$offLevel" ] ; then
	echo true
    else
	echo false
    fi
    exit 0
fi

if [ "$USER" != "root" ] ; then
    sudo $DIR/$(basename $0) "$operation"
    exit 0
fi

if [ "$operation" == "exit" ] ; then

    rm -f $configFile
    echo $GPIO > /sys/class/gpio/unexport

    exit 0
fi

if [ "$operation" == "on" ] ; then

    if [ "$(cat $gpioDir/value)" != "$onLevel" ] ; then
	echo $onLevel > $gpioDir/value
	sleep 2
    fi

elif [ "$operation" == "off" ] ; then

    if [ "$(cat $gpioDir/value)" != "$offLevel" ] ; then
	echo $offLevel > $gpioDir/value
    fi
else
    echo usupported operation $operation
    exit 0
fi

exit 0
