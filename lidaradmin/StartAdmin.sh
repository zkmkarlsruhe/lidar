#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

loop="+loop"

DIR="$(realpath -s $(dirname $0))"
cd $DIR
source config.txt


if [ "$adminPort" != "" ] ; then
    adminPortParam="+adminport $adminPort"
fi

if [ "$webPort" != "" ] ; then
    webPortParam="+webport $webPort"
fi

if [ "$hubPort" != "" ] ; then
    hubPortParam="+hubport $((webPort+1))"
fi

if [ "$configPort" != "" ] ; then
    configPortParam="+configport $configPort"
fi

if [ "$configServer" == "true" ]; then
    configServerParam="+configServer"
fi

if [ "$dataDir" != "" ] ; then
    fileSystem="+fileSystem $dataDir"
fi

if [ -f SpaceFailureReportScript.sh ] ; then	
    spaceFailParam="+spaceFailureReportScript SpaceFailureReportScript.sh"
fi

ARGS=( "$@" )

while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
	+loop)
	loop="$1"
	shift # past argument
	;;
	-loop)
	shift # past argument
	;;
	+v)
	verbose="+v"
	shift # past argument
	;;
	restart)
	pids=$(pgrep -xa $EXECUTABLE|grep "+conf $conf")
	[ -z "$pids" ] || (kill -9 $pids ; sleep 3)
	shift # past argument
	;;
	*) 
	ARGS+=( "$1" )
	shift # past argument
	;;
    esac
done

pids=$(pgrep -xa lidarAdmin|grep "+conf $conf")
if [ "$pids" != "" ] ; then
    exit 0
fi

cmd="./lidarAdmin.sh $verbose +conf $conf $configServerParam $configPortParam $adminPortParam $webPortParam $hubPortParam ${ARGS[@]} $fileSystem $spaceFailParam $loop"

if [ "$verbose" != "" ] ; then
    echo $cmd
fi

$cmd
