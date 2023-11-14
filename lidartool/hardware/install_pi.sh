#!/bin/bash

#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

cmd=$(realpath -s "$0")
lidarDir=$(dirname $cmd)
lidarHWDir=$lidarDir

cd $lidarDir

MODEL="$($lidarHWDir/raspiModel.sh)"
if [ "$MODEL" == "" ] ; then
    MODEL="Unknown"
fi

isPi=false
    
if [ "$MODEL" == "RockPiS" ] || [ "$(echo $MODEL | cut -c1-5)" == "RasPi" ] ; then
    isPi=true
fi
    
if ! $isPi ; then
    exit 0
fi

if [ "$PiUser" == "" ] ; then
    if [ "$(whoami)" == "root" ] ; then
        echo run as normal user !!!
    else
        sudo PiUser=$(whoami) $cmd
    fi
    exit 0
fi

if [ "$(grep lidarPower.sh /etc/sudoers)" == "" ] ; then
    echo >> /etc/sudoers
    echo $PiUser ALL=NOPASSWD: $lidarHWDir/lidarPower.sh >> /etc/sudoers
    if [ "$MODEL" == "RockPiS" ] ; then
        echo $PiUser ALL=NOPASSWD: $lidarHWDir/setStatusIndicator.sh >> /etc/sudoers
    fi
fi

if [ "$(grep $lidarHWDir /etc/rc.local)" == "" ]; then
    
    if [ "$(head -c 6 /etc/rc.local)" != "#!/bin" ] ; then
        sed -i '1s;^;#!/bin/sh -e\n;' /etc/rc.local
    fi
    sed -i "s#will \"exit 0\"#will exit \"0\"#g" /etc/rc.local
    if [ "$MODEL" == "RockPiS" ] ; then
        sed -i "s#exit 0#$lidarHWDir/lidarPower.sh +gpio 69 +onHi init\nexit 0#g" /etc/rc.local
	$lidarHWDir/lidarPower.sh +gpio 69 +onHi init
    fi
fi

if [ ! -e /etc/init.d/lidarStop.sh ] ; then
    cp $lidarHWDir/lidarStop.sh /etc/init.d/.
    ln -s /etc/init.d/lidarStop.sh /etc/rc6.d/K99_lidarStop
    sed -i "s#lidarDir#$lidarHWDir#g" /etc/init.d/lidarStop.sh
fi
    
exit 0

