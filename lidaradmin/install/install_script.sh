#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

numProc="$1"

if [ "$numProc" == "" ]; then
    if [ $(expr $(sed -n '/^MemTotal:/ s/[^0-9]//gp' /proc/meminfo) / 1024) -lt 900 ] ; then
	numProc=1
    else
        numProc=`nproc`
    fi
fi

admindir=$(pwd)

# install dependencies

if [ $(apt show libcurl4-openssl-de 2> /dev/null | wc -l) -gt 0 ] ; then
    sudo apt-get -yq install libcurl4-openssl-dev
else
    sudo apt-get -yq install libcurl-dev
fi

sudo apt-get -yq install liblo-tools
sudo apt-get -yq install ssh-askpass

cd ../lidartool
USE_MQTT=true USE_WEBSOCKETS=true USE_OSC=true USE_LUA=true ./install/install_script.sh

cd $admindir

# compile application

make -j $numProc


