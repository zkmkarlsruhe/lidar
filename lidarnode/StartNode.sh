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

if [ "$1" == "update" ] ; then
    shift
    ./install_node.sh repair
    ./install_node.sh update

    ./registerNode.sh +config
    ( ./registerNode.sh ) &
    sleep 5;
fi

if [ -f config.txt ] ; then
    source config.txt
else
    source config_default.txt
fi

lidar="$(realpath -s $(dirname $0)/..)"
lidartool="$lidar/lidartool"
export LIDARCONF=
export LIDARCONFDIR="$lidar/lidarnode"

cd $DIR

isRunning=$($lidartool/lidarTool.sh isRunning)

if $isRunning ; then
    exit 0
fi

if [ -f LidarNodeId.txt ] ; then
	lidarNodeId=$(cat LidarNodeId.txt)
else
	lidarNodeId=000
fi

if [ -f LidarDevices.txt ] ; then 
    lidarDevices=$(cat LidarDevices.txt)
fi

if [ "$lidarDevices" == "" ] ; then 
    if [ -f LidarDeviceType.txt ] ; then 
	lidarDeviceType="$(cat LidarDeviceType.txt):"
    fi

    lidarDevices="+d ${lidarDeviceType}0 +virtual $server:$portBase$lidarNodeId"
fi

if [ -f LidarParam.txt ] ; then 
    lidarParam=$(cat LidarParam.txt)
fi

cd $lidartool

if $(./hardware/setStatusIndicator.sh isSupported) ; then
    useStatusIndicator="+useStatusIndicator"
fi

./hardware/lidarPower.sh on
./hardware/setStatusIndicator.sh running

cmd="./lidarTool.sh $@ $lidarDevices $lidarParam $useStatusIndicator $loop $usb_power_ctl"

echo $cmd
$cmd
