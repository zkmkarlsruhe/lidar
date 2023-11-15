#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

DIR="$(realpath -s $(dirname $0))"
CMD="$(basename $0)"
cd $DIR

mountPoint=/tmp/usbstick

update=false

if [ -f $mountPoint/setup.sh ] ; then

    if [ "$1" == "mount" ] ; then
        su $2 ./$CMD
        umount $mountPoint
        if [ "$(../lidartool/raspiModel.sh)" == "RockPiS" ] ; then
            echo timer > /sys/class/leds/rockpis\:blue\:user/trigger
        fi
    else
        $mountPoint/setup.sh $DIR
        ./install_node.sh update
        ./registerNode.sh +try
    fi

elif [ -f $mountPoint/config.txt ] ; then
    if [ "$1" == "mount" ] ; then
        if [ "$(diff $mountPoint/config.txt config.txt)" != "" ] ; then
            su $2 ./$CMD
            reboot
        else
            umount $mountPoint
            if [ "$(../lidartool/raspiModel.sh)" == "RockPiS" ] ; then
                echo timer > /sys/class/leds/rockpis\:blue\:user/trigger
            fi
        fi
    else
        ./install_node.sh repair
        cp $mountPoint/config.txt .
        ./install_node.sh update
        ./registerNode.sh +try
    fi
fi

exit 0


