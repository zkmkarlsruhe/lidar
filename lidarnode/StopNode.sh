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

if [ -f config.txt ] ; then
    source config.txt
else
    source config_default.txt
fi


lidarroot="$(realpath -s $(dirname $0)/..)"
lidartool="$lidarroot/lidartool"

pids=$(echo $(pgrep -x lidarTool) $(pgrep -x lidarTool.sh))
[ -z "$pids" ] || (kill -9 $pids ; sleep 3)

$lidartool/hardware/lidarPower.sh off
$lidartool/hardware/setStatusIndicator.sh stopped

