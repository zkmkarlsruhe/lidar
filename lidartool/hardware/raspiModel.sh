#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//


# for Raspberry 2, 3, 4 and NanoPi Neo only

Model="$(cat /proc/cpuinfo | grep Model)"

if [ -f /etc/Model ] ; then
        Model="$(cat /etc/Model)"
elif [ "$(echo \"$Model\" | grep 'Raspberry Pi 2')" != "" ]; then
	Model="RasPi2"
elif [ "$(echo \"$Model\" | grep 'Raspberry Pi 3')" != "" ]; then
        Model="RasPi3"
elif [ "$(echo \"$Model\" | grep 'Raspberry Pi 4')" != "" ]; then
        Model="RasPi4"
elif [ "$(echo \"$Model\" | grep 'Raspberry Pi Model')" != "" ]; then
	Model="RasPi1"
else
	if [ -f /etc/friendlyelec-release ] ; then
		Model="$(source /etc/friendlyelec-release ; echo $BOARD_NAME)"
	else
		Model=""
	fi
fi

if [ "$Model" != "" ] ; then
    echo $Model
fi

