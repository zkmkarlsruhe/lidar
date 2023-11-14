#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
	mqtt)
        USE_MQTT=true
	shift # past argument
	;;
	websockets)
        USE_WEBSOCKETS=true
	shift # past argument
	;;
	osc)
        USE_OSC=true
	shift # past argument
	;;
	lua)
        USE_LUA=true
	shift # past argument
	;;
	all)
        USE_MQTT=true
        USE_WEBSOCKETS=true
        USE_OSC=true
        USE_LUA=true
	shift # past argument
	;;
	*)
        numProc="$1"
	shift # past argument
	;;
    esac
done

if [ "$numProc" == "" ]; then
    if [ $(expr $(sed -n '/^MemTotal:/ s/[^0-9]//gp' /proc/meminfo) / 1024) -lt 900 ] ; then
	numProc=1
    else
        numProc=`nproc`
    fi
fi

#install dependencies

sudo apt-get -yq install build-essential git cmake automake libtool m4 libx11-dev libjpeg-dev libpng-dev libudev-dev wget libcurl4-openssl-dev curl dnsutils net-tools uuid-dev pkg-config jq bc

# install optional

if [ "$USE_MQTT" ] ; then
	sudo apt-get -yq install libmosquitto-dev
fi

if [ "$USE_WEBSOCKETS" ] ; then
        sudo apt-get -yq install libwebsockets-dev
fi

if [ "$USE_OSC" ] ; then
        sudo apt-get -yq install liblo-dev liblo-tools
fi

if [ "$USE_LUA" ] ; then
        sudo apt-get -yq install lua5.3 liblua5.3-dev luarocks lua-socket
fi

#sudo apt-get -yq install libmosquitto-dev libwebsockets-dev liblo-dev liblo-tools lua5.3 liblua5.3-dev luarocks lua-socket 

# compile application

make -j $numProc

# install udev rules

sudo cp install/90-lidar.rules /etc/udev/rules.d/.

sudo sed -i "s#pathToInstallDir#`pwd`#g" /etc/udev/rules.d/90-lidar.rules

# pi platform specific install

./hardware/install_pi.sh

#reload udev

sudo service udev reload
sudo service udev restart

