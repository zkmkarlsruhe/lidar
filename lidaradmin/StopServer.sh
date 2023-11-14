#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

DIR="$(realpath $(dirname $0))"
cd $DIR

source config.txt

lidartool="$lidarroot/lidartool"

id=
hub=
server=true

cmdArgs=

while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
	server)
        id="+id server"
	shift # past argument
	;;
	hub)
        id="+id hub"
	server=false
	shift # past argument
	;;
	testHUB)
	id="+id testHUB"
	server=false
	shift # past argument
	;;
	stats)
	id="+id stats"
	server=false
	shift # past argument
	;;
	+id)
        id="+id $2"
	server=false
	shift # past argument
	shift # past value
	;;
	*)
	cmdArgs="$cmdArgs $1"
	shift # past argument
	;;
    esac
done

if $($lidartool/lidarTool.sh isRunning $conf $id) ; then

    if $server; then

	wget http://localhost:$webPort/stop -q -O /dev/null &>> /dev/null >> /dev/null

	( ./manageNodes.sh stop ) &
	
    fi

    $lidartool/lidarTool.sh kill $conf $id
    sleep 3

fi
