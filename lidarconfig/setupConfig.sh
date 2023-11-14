#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

lidartool=../lidartool
lidaradmin=../lidaradmin
lidarconfig=../lidarconfig

pushd() { builtin pushd $1 > /dev/null; }
popd()  { builtin popd > /dev/null; }

if [ -f config.txt ] ; then 

    source config.txt

    lidartool="$lidarroot/lidartool"
    lidaradmin="$lidarroot/lidaradmin"
    lidarconfig="$lidarroot/lidarconfig"

    mkdir -p $conf

    pushd $lidartool
    if [ ! -L conf/$conf ] ; then
	echo "creating conf dir in $(pwd)"
	echo 'ln -s' "$confDir/$conf" conf/.
	ln -s "$confDir/$conf" conf/.
    fi
    popd

fi

install() {

    if ! test -h $1 ; then
	echo 'ln -s' $lidaradmin/$1 .
	ln -s $lidaradmin/$1 .
    fi
}

install manageNodes.sh
install manageSensors.sh
install StopServer.sh
install lidarAdmin
install lidarAdmin.sh
install StartAdmin.sh
install StopAdmin.sh
install html

if ! test -h editConfig.sh ; then
    echo 'ln -s' $lidarconfig/editConfig.sh .
    ln -s $lidarconfig/editConfig.sh .
fi

if [ ! -f StartServer.sh ] ; then 
    echo cp $lidaradmin/StartServer.sh .
    cp $lidaradmin/StartServer.sh .
fi

if [ ! -f modelMap.txt ] ; then 
    echo cp $lidaradmin/modelMap.txt .
    cp $lidaradmin/modelMap.txt .
fi


