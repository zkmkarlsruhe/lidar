#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

cmd=$(realpath -s "$0")
if [ "$1" == "update" ] && [ "$2" != "" ] ; then
	cmd=$(realpath -s "$2")
fi

nodeId=

link=$(ip -o -br link | awk '$2=="UP" {print $1}')
if [ "$link" == "" ] ; then
	link=eth0
fi
IPaddr=$(ip a l $link | awk '/inet / {print $2}' | cut -d/ -f1)
HWaddr=$(ip a l $link | awk '/ether/ {print $2}')

invalidHW() {
   if [ $(echo "$1" | tr -cd ':' | wc -c) != "5" ] ; then
       true
   fi
   
   if [ $(echo "$1" | wc -c ) != "18" ] ; then
       true
   fi

   false
}

invalidIP() {

   if [ $(echo "$1" | tr -cd '.' | wc -c) != "3" ] ; then
       true
   fi
   
   if [ $(echo "$1" | tr -cd ' ' | wc -c) != "0" ] ; then
       true
   fi

   false
}

if invalidIP "$IPaddr" ; then
    IPaddr="0.0.0.0"
fi

if invalidHW "$HWaddr" ; then
    HWaddr="00:00:00:00:00:00"
fi

confDir="$(dirname "$cmd")"
cd $confDir

if [ "$1" == "update" ] && [ "$(whoami)" == "root" ] ; then
    if [ "$3" != "" ] ; then
	conf="$3"
    fi
else
    if [ -f config.txt ] ; then
    	source config.txt
    elif [ -f config_default.txt ] ; then
    	source config_default.txt
    fi
fi

lidarroot=$HOME/lidar

SETUPDIR="$(realpath -s $(dirname $0))"
cd $SETUPDIR

if [ ! -f user.txt ] ; then
    if [ "$(whoami)" == "root" ] ; then
        echo run first time as normal user !!!
        exit 0
    fi
    echo "$(whoami)" > user.txt
    echo please run: sudo $0 install
    exit 0
fi
USER=$(cat user.txt)

MODEL="$(../lidartool/hardware/raspiModel.sh)"
if [ "$MODEL" == "" ] ; then 
    MODEL="Unknown"
fi

if [ "$1" == "entry" ] ; then

    if [ -f LidarNodeId.txt ] ; then
	nodeId="$(cat LidarNodeId.txt)"
    fi
    if [ "$nodeId" == "" ] || [ "$(echo $nodeId | grep ':')" != "" ] ; then
	nodeId="000"
    fi

    sensorIN="_"
    if [ -f "../lidartool/SensorIN.txt" ] ; then
	sensorIN="$(cat ../lidartool/SensorIN.txt)"
    fi

    poweringEnabled="pwDis"
    if [ -f ../lidartool/hardware/LidarPower.enable ] ; then
	if [ "$(cat ../lidartool/hardware/LidarPower.enable)" == "true" ] ; then
	    poweringEnabled="pwEn"
	fi
    else
	poweringEnabled="pwUndef"
    fi
    entry="$HWaddr $IPaddr $nodeId $USER $MODEL $sensorIN $poweringEnabled"

    echo $entry

    exit 0
fi

if [ "$1" == "repair" ] ; then

    if [ "$useServerGit" == "true" ] ; then
    	find .git/objects -type f -empty -delete
    	git prune
    	git fetch --all --prune
    	git reset HEAD --hard
    	git pull
    fi
    exit 0

fi

if [ "$1" == "install" ] ; then
    
    if [ "$(whoami)" != "root" ] ; then
        sudo $cmd $@
        exit 0
    fi

    isPi=false
    
    if [ "$MODEL" == "RockPiS" ] || [ "$(echo $MODEL | cut -c1-5)" == "RasPi" ] ; then
      isPi=true
    fi
    
    if [ "$(grep shutdown /etc/sudoers)" == "" ] ; then
        echo >> /etc/sudoers
        echo $USER ALL=NOPASSWD: /sbin/shutdown >> /etc/sudoers
        echo $USER ALL=NOPASSWD: /sbin/reboot >> /etc/sudoers
        echo $USER ALL=NOPASSWD: $SETUPDIR/install_node.sh >> /etc/sudoers
    fi

    if [ "$(grep $SETUPDIR /etc/rc.local)" == "" ]; then
    
    	if [ "$(head -c 6 /etc/rc.local)" != "#!/bin" ] ; then
    		sed -i '1s;^;\#!/bin/sh -e\n;' /etc/rc.local
    	fi
        sed -i "s#will \"exit 0\"#will exit \"0\"#g" /etc/rc.local
        sed -i "s#exit 0#\(sleep 25; su - $USER -c '$SETUPDIR/StartNode.sh update \> /dev/null \>\& /dev/null' \) \&\n\nexit 0#g" /etc/rc.local
    fi

    if $isPi ; then
    	cp install/99-usbStickPlugged.rules /etc/udev/rules.d/.
    	cp install/usbStickPlugged.service /etc/systemd/system/.

    	sed -i "s#pathToConfigDir#`pwd`#g" /etc/systemd/system/usbStickPlugged.service
    	sed -i "s#USER#$USER#g" /etc/systemd/system/usbStickPlugged.service

    	service udev reload
    	service udev restart

    	sudo systemctl start  usbStickPlugged.service
    	sudo systemctl enable usbStickPlugged.service
    fi
    
    exit 0
fi

op="$1"

if [ "$op" != "" ] && [ "$op" != "update" ] ; then
    nodeId="$1"
    if [ "$(whoami)" != "root" ] ; then
        echo "$nodeId" > LidarNodeId.txt
        op="update"
    fi
fi

if [ "$op" == "update" ] ; then

    if [ "$(whoami)" != "root" ] ; then
        sudo $cmd $@ $cmd $conf
        exit 0
    else
    	cd $confDir
    	
   	if [ -f LidarNodeId.txt ] ; then
            nodeId="$(cat LidarNodeId.txt)"
	fi
        if [ "$nodeId" != "" ] ; then
            host=lidarpi$nodeId
        else
            host=$(nslookup $IPaddr | grep "name =" | awk -F= '{print $2}' | tr -d ' ' | sed 's/\(.*\)./\1 /')
        fi
        if [ "$host" == "" ] && [ "$conf" != "" ] ; then
            host=lidar$conf
        fi
        if [ "$host" == "" ] ; then
            host=lidarpi
        fi
        hostname="$(echo $host | cut -d "." -f 1)" 
        
        echo $hostname > /etc/hostname
        sed -i '$ d' /etc/hosts
        echo 127.0.0.1 $host $hostname >> /etc/hosts
        exit 0
    fi
fi

exit 0
