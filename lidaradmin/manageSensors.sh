#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

DIR="$(realpath -s $(dirname $0))"

source config.txt

lidartool="$lidarroot/lidartool"
lidaradmin="$lidarroot/lidaradmin"
lidarconfig="$lidarroot/lidarconfig"

source modelMap.txt

simulationMode=
testMode=

dconf="+conf $conf"

if [ "$sensorDB" == "" ] ; then
    sensorDB="sensorDB.txt"
fi

pushd() { builtin pushd $1 > /dev/null; }
popd()  { builtin popd  $1 > /dev/null; }

printHelp() {
    echo "usage: $(basename $0) [+t] [+s] [+q] [enable|disable name|nodeId|update]"
    echo "  +t              test only but do not execute"
    echo "  +s              create sensor info for simulation mode"
    echo "  +q              quiet"
    echo "  enable|disable  enables or disables a sensor with given name or nodeId"
    echo "  update          updates the sensor data base when the $sensorDB changed"
    exit 0
}

setNikName() {
    
    cmd="./lidarTool.sh $dconf +setNikName ${group}_${model}${groupId}"
    if [ "$simulationMode" == "+simulationMode" ] ; then
	cmd+=" ${modelMap[$model]}:$nodeId"
    else
	if [ "$active" == "-" ] ; then
	    cmd+=" -"
	else
	    cmd+=" virtual:$portBase$nodeId"
	fi
    fi

    cmd+=" $simulationMode"
    
    gcmd="./lidarTool.sh $dconf +assignDeviceToGroup ${group} ${group}_${model}${groupId}"

    if [ "$quiet" == "" ] ; then
	echo $cmd
	echo $gcmd
    fi

    if [ "$testMode" == "" ]; then
	$cmd
	$gcmd
    fi
}

setNikNames() {

    mkdir -p "conf/$conf"

    numSensors=${#sensors[@]}

    for ((i=0; i < $numSensors; i++))
    do
      entry="${sensors[$i]}"
      
      group=$(echo $entry | awk '{print $1}')

      if [ "$group" != "" ] ; then
	  model=$(echo $entry | awk '{print $2}')
	  groupId=$(echo $entry | awk '{print $3}')
	  nodeId=$(echo $entry | awk '{print $4}')
	  active=$(echo $entry | awk '{print $5}')
	  mac=$(echo $entry | awk '{print $6}' | tr "[:upper:]" "[:lower:]")

	  setNikName
      fi

    done
}

readSensors() {

    sensors=()
    
    if [ -f "$sensorDB" ] ; then

	readarray sensorArray < $sensorDB

	numSensors=${#sensorArray[@]}

	for ((i=0; i < $numSensors; i++))
	  do
	  entry="${sensorArray[$i]}"
      
	  first=$(echo $entry | awk '{print $1}')

	  if [ "$first" != "" ] && [ "$(echo $first | cut -c 1)" != "#" ] ; then
	      sensors+=("$entry")
	  fi
      
	done
    fi
}

activate() {

    readSensors

    numSensors=${#sensors[@]}

    for ((i=0; i < $numSensors; i++))
    do
      entry="${sensors[$i]}"
      
      group=$(echo $entry | awk '{print $1}')

      if [ "$group" != "" ] ; then
	  model=$(echo $entry | awk '{print $2}')
	  groupId=$(echo $entry | awk '{print $3}')
	  nodeId=$(echo $entry | awk '{print $4}')
	  active=$(echo $entry | awk '{print $5}')
	  mac=$(echo $entry | awk '{print $6}' | tr "[:upper:]" "[:lower:]")
	  name=${group}_${model}${groupId} 
	  devType=${modelMap[$model]} 

	  if [ "$1" == "$nodeId" ] || [ "$1" == "$name" ] ; then
	      if [ "$2" != "$active" ] ; then
		  sed -i "/.* $nodeId .*/ s/$active/$2/" $sensorDB
		  if $useServerGit ; then
		      git commit $sensorDB -m "en-/disabled node $nodeId" >> /dev/null
		      git push >> /dev/null
		  fi
		  update="update"
	      fi
	  fi
      fi

    done
}


queryMAC() {

    readSensors

    numSensors=${#sensors[@]}

    for ((i=0; i < $numSensors; i++))
    do
      entry="${sensors[$i]}"
      
      group=$(echo $entry | awk '{print $1}')

      if [ "$group" != "" ] ; then
	  model=$(echo $entry | awk '{print $2}')
	  groupId=$(echo $entry | awk '{print $3}')
	  nodeId=$(echo $entry | awk '{print $4}')
	  active=$(echo $entry | awk '{print $5}')
	  mac=$(echo $entry | awk '{print $6}' | tr "[:upper:]" "[:lower:]")
	  devType=${modelMap[$model]} 

	  if [ "$1" == "$mac" ] ; then
	      echo ${group}_${model}${groupId} $nodeId $devType $active
	  fi
      fi

    done
}


update=

while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
	-h|--help|help)
        printHelp
	shift # past argument
	;;
	enable)
        nodeId=$2
        activate $nodeId '+'
	shift # past argument
	shift # past value
	;;
	disable)
        nodeId=$2
        activate $nodeId '-'
	shift # past argument
	shift # past value
	;;
	update)
        update=$1
	shift # past argument
	;;
	list)
        cat $sensorDB
	exit 0
	shift # past argument
	;;
	hasSensors)
        readSensors
    
	if [ ${#sensors[@]} -gt 0 ] ; then
	    echo 1
	else
	    echo 0
	fi
	exit 0
        ;;
	+mac)
        queryMAC "$2"
	shift # past argument
	shift # past value
	;;
	+s)
        simulationMode="+simulationMode"
	shift # past argument
	;;
	+t)
        testMode="testMode"
	shift # past argument
	;;
	+q)
        quiet="$1"
	shift # past argument
	;;
	*) 
	shift # past argument
	;;
    esac
done


if [ "$update" != "" ] ; then

    readSensors

    pushd $lidartool
    
    cmd="./lidarTool.sh $dconf +clearNikNames $simulationMode"
    if [ "$quiet" == "" ] ; then
	echo $cmd
    fi

    if [ "$testMode" == "" ]; then
	$cmd
    fi

    cmd="./lidarTool.sh $dconf +clearGroups"
    if [ "$quiet" == "" ] ; then
	echo $cmd
    fi

    if [ "$testMode" == "" ]; then
	$cmd
    fi

    setNikNames

    popd

fi


