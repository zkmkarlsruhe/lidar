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

source config.txt

lidartool="$lidarroot/lidartool"

export LIDARCONF=$conf
export LIDARCONFDIR="$lidarroot/lidarconfig-$conf/$conf"

loop="+loop"

simulation=
recordPacked=
obstacle=
verbose=
observer=()

if [ "$fps" == "" ] ; then
    fps="30"
fi

if $useNodes && [ "$useGroups" == "" ] && [ -f "$conf/groups.json" ] && [ $(jq length "$conf/groups.json") -gt 0 ] ; then
    useGroups="+useGroups"
fi

if [ "$recordPackedDir" != "" ] && [ -d "$recordPackedDir" ] ; then
    recordPackedParam="+observer @type=packedfile @name=recordPacked @file=$recordPackedDir/packed/packed_%daily.pkf @maxFPS=$fps"
fi

if [ -f "$conf/deviceFailed.sh" ] ; then
    deviceFailedParam="+failureReportScript [conf]/deviceFailed.sh"
fi

if [ -f "$conf/notification.sh" ] ; then
    notificationParam="+notificationScript [conf]/notification.sh"
fi

logDir="[conf]"
if [ -d "/var/log/$conf" ] ; then
    logDir="/var/log/$conf"
fi

reportParam="+logFile $logDir/run.log +errorLogFile [conf]/error.log $notificationParam"

simulationParam="+simulationMode $simulationParam"

runMode="setup"
if [ -f LidarRunMode.txt ] ; then 
    runMode=$(cat LidarRunMode.txt)
fi

if [ "$hubPort" == "" ] ; then
    hubMode="noHub"
else
    hubMode="hub"
fi
if [ -f LidarHubMode.txt ] ; then 
    hubMode=$(cat LidarHubMode.txt)
fi


# read observer declaration from local directory
if [ -f observer.txt ] ; then 
    source observer.txt
fi
# read observer declaration from conf directory
if [ -f "$conf/observer.txt" ] ; then 
    source "$conf/observer.txt"
fi

# read observer declaration from default observer json
if [ -f "$conf/observer.json" ] ; then 
    observer+=("+useObservers")
fi

serverObserver=( "${observer[@]}" )

if [ "$useGroups" == "" ] ; then # read devices if no groups are defined

    if [ -f LidarSensors.txt ] ; then
	readarray sensorArray < "LidarSensors.txt"

	numSensors=${#sensorArray[@]}
	lidarSensors=

	for ((i=0; i < $numSensors; i++))
	  do
	  entry="${sensorArray[$i]}"
	  first=$(echo $entry | awk '{print $1}')

	  if [ "$first" != "" ] && [ "$(echo $first | cut -c 1)" != "#" ] ; then
	      if  [ "$(echo $entry | cut -c 1-2)" != "+d" ] ; then
		  entry="+d $entry"
	      fi
	      lidarSensors="$lidarSensors $entry"
	  fi
      
	done
    fi

    if [ "$lidarSensors" == "" ] ; then # no devices ? define default device with given type
	if [ -f LidarDeviceType.txt ] ; then 
	    lidarDeviceType="$(cat LidarDeviceType.txt):"
	fi

	lidarSensors="+d ${lidarDeviceType}0"
    fi
fi

server=true
testHUB=false
noSensors=false

if [ "$hubPort" != "" ] ; then
    idParam="+id server"
    serverHUBParam="+hub :$hubPort $idParam"
    clientHUBParam="+hub localhost:$hubPort:$webPort"
    clientHUBid="+id hub"
    hubWebPort=$((webPort+1))
    hubObserver=( "${observer[@]}" )
    serverObserver=()
fi

trackParam="+useSimulationRange +radialDisplacement 0.26 track.uniteDistance 0.75 track.trackDistance 1 track.trackMotionPredict 1.5 track.trackFilterWeight 0.5 track.trackSmoothing 0.8 track.minActiveTime 1.5 track.keepTime 2 track.latentDistance 1.7 track.latentLifeTime 20 lidar.object.maxDistance 0.16 lidar.object.maxExtent 0.8 track.objectMaxSize 0.9"

cmdArgs=()
while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
	kill|rerun)
        target=
	if [ "$2" == "hub" ] || [ "$2" == "server" ]  || [ "$2" == "hub" ] ; then
	    target=$2
	fi
        ./StopServer.sh $target
	if [ "$1" == "kill" ] ; then
	    exit 0
	fi
	shift # past argument
	;;
	+setup)
        runMode="setup"
	shift # past argument
	;;
	+simulation)
        runMode="simulation"
	shift # past argument
	;;
	*)
	cmdArgs+=( "$1" )
	shift # past argument
	;;
    esac
done

set -- "${cmdArgs[@]}"

if [ "$runMode" == "simulation" ] ; then
    simulation="$simulationParam"
    obstacle="$obstacleParam"
    loop=
elif [ "$runMode" == "setup" ] ; then
    loop=
else
    recordPacked="$recordPackedParam"
fi

printHelp() {
    echo "usage: $(basename $0) [+v] [kill|rerun] [server|hub|testHUB] [lidarPlay fileTemplate] [packedPlay packedFile.pkf] [+s|+so] arg1 arg2 ..."
    echo "  +v             verbose"
    echo "  +s             simulation"
    echo "  +so            simulation with obstacle"
    echo "  +recordPacked  activate recording of packed data"
    echo "  +setup         setup mode without observers"
    echo "  +simulation    simulation mode"
    exit 0
}


args=()
while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
	+h|-h|-help|+help|help)
        printHelp
	shift # past argument
	;;
	+s)
        simulation="$simulationParam"
	shift # past argument
	;;
	-s)
        simulation=
	shift # past argument
	;;
	+o)
        obstacle="$obstacleParam"
	shift # past argument
	;;
	-o)
        obstacle=
	shift # past argument
	;;
	+setup)
        runMode="setup"
	shift # past argument
	;;
	+simulation)
        runMode="simulation"
	shift # past argument
	;;
	+so)
        simulation="$simulationParam"
        obstacle="$obstacleParam"
	shift # past argument
	;;
	+recordPacked)
        recordPacked="$recordPackedParam"
	shift # past argument
	;;
	-recordPacked)
        recordPacked=
	shift # past argument
	;;
	-sensors)
        noSensors=true
	shift # past argument
	;;
	+loop)
	loop="+loop"
	shift # past argument
	;;
	-loop)
	loop=
	shift # past argument
	;;
	+v)
        verbose=$1
	shift # past argument
	;;
	-v)
        verbose=
	shift # past argument
	;;
	lidarPlay)
        lidarPlay="+lidarPlay $2 +playExitAtEnd"
	shift # past argument
	shift # past value
	;;
	packedPlay)
        packedPlay="+packedPlay $2"
	shift # past argument
	shift # past value
	;;
	+webPort)
	webPort="$2"
	shift # past argument
	shift # past value
	;;
	+id)
	idParam="$1 $2"
	shift # past argument
	shift # past value
	;;
	server|-hub)
	hubPort=
	hubMode="noHub"
	shift # past argument
	;;
	hub)
	server=false
	hubMode="hub"
	shift # past argument
	;;
	testHUB)
	testHUB=true
	hubObserver=()
	hubWebPort=$((hubWebPort+1))
	clientHUBid="+id testHUB"
	idParam="$clientHUBid"
	shift # past argument
	;;
	*)
	args=( "${args[@]}" "$1" )
	shift # past argument
	;;
    esac
done

if [ "$hubMode" == "noHub" ] ; then
    hubPort=
fi

if ! $server ; then
    idParam="$clientHUBid"
fi

if [ "$verbose" != "" ] ; then
    echo "Run Mode: $runMode"
    
    if [ "$hubMode" != "noHub" ] ; then
	echo "Hub Mode: $hubMode"
    fi
    if $noSensors && [ "$verbose" != "" ] ; then
	echo "Do not open Sensors"
    fi
fi

if [ -f "$conf/blueprints.json" ] ; then
    useBlueprints="+useBlueprints"
fi

if [ -f "$conf/regions.json" ] ; then
    useRegions="+useRegions"
fi

defaultParam="$useBlueprints $useRegions +track +fps $fps"

cd $lidartool

if [ "$hubPort" != "" ] ; then

    if ! $($lidartool/lidarTool.sh isRunning $conf $clientHUBid) ; then
	hubParam="$defaultParam $clientHUBParam"
	args+=( "${hubObserver[@]}" )
	cmd="./lidarTool.sh +conf $conf $clientHUBid +webport $hubWebPort $hubParam"

	if [ "$verbose" != "" ] ; then
	    echo $cmd "${args[@]}" $loop
	fi
    
	if $testHUB ; then
	    $cmd "${args[@]}" $verbose
	else
	    $cmd "${args[@]}" $loop $verbose &
	fi
    fi
    if $testHUB ; then
	exit 0
    fi
fi

if ! $server ; then
    exit 0
fi

if $($lidartool/lidarTool.sh isRunning $conf $idParam) ; then
    exit 0
fi

if [ "$simulation" == "" ] && [ "$lidarPlay" == "" ] && [ "$packedPlay" == "" ] ; then

    cd $DIR
    ./manageNodes.sh start
    sleep 3
    cd $lidartool

fi

serverParam=($lidarSensors $defaultParam $reportParam $deviceFailedParam $trackParam $serverHUBParam "${serverObserver[@]}" $useGroups $simulation $obstacle $recordPacked $lidarPlay $packedPlay)

if $noSensors ; then
    serverParam+=( "-openOnStart" )
fi

cmd="./lidarTool.sh +conf $conf $idParam +webport $webPort +runMode $runMode $loop"

args=( "${serverParam[@]}" "${args[@]}" )

if [ "$verbose" != "" ] ; then
    echo $cmd "${args[@]}"
fi

$cmd "${args[@]}" $verbose

