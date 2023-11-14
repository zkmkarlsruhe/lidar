#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//



function setDefaults()
{
    [ -z ${useNodes+x} ] && useNodes=false
    [ -z ${server+x} ] && server="$(hostname -I)"
    [ -z ${webPort+x} ] && webPort=8080
    [ -z ${hubPort+x} ] && hubPort=' #5000'
    [ -z ${adminPort+x} ] && adminPort=8000
    [ -z ${portBase+x} ] && portBase=40
    [ -z ${useServerGit+x} ] && useServerGit=false
    [ -z ${blueprintImageFile+x} ] && blueprintImageFile=''
    [ -z ${blueprintExtent+x} ] && blueprintExtent=''
    [ -z ${blueprintSimulationFile+x} ] && blueprintSimulationFile=''
    [ -z ${blueprintOcclusionFile+x} ] && blueprintOcclusionFile=''
    [ -z ${blueprintObstacleImageFile+x} ] && blueprintObstacleImageFile=''
    [ -z ${blueprintObstacleExtent+x} ] && blueprintObstacleExtent=''
    [ -z ${dataDir+x} ] && dataDir=' #/data'
    [ -z ${recordPackedDir+x} ] && recordPackedDir=' #/data/packed'
    [ -z ${desktopColor+x} ] && desktopColor=505050

    if [ "$conf" == "" ] ; then

	if [ -f config.txt ] ; then 

	    dir=$(pwd)
	    source config.txt
	    cd $dir
	fi
    else
	[ -z ${desktopLabel+x} ] && desktopLabel=$conf-LIDAR
    fi
}

function printValues() {
    echo useNodes=$UseNodes
    echo server=$server
    echo webPort=$webPort
    echo hubPort=$hubPort
    echo adminPort=$adminPort
    echo portBase=$portBase
    echo useServerGit=$useServerGit
    echo blueprintExtent=$blueprintExtent
    echo blueprintImageFile=$blueprintImageFile
    echo blueprintOcclusionFile=$blueprintOcclusionFile
    echo blueprintSimulationFile=$blueprintSimulationFile
    echo blueprintObstacleExtent=$blueprintObstacleExtent
    echo blueprintObstacleImageFile=$blueprintObstacleImageFile
    echo dataDir=$recordPackedDir
    echo recordPackedDir=$dataDir
    echo desktopColor=$desktopColor
    echo desktopLabel=$desktopLabel
}

function printHelp() {

    setDefaults

    echo "usage: $0 [-h|--help] [create name|path/to/initDir] [init path/to/initDir] [list] [createRegion regionName] [+conf name] [param=value]"
    echo 
    echo "allows to create or edit a lidar configuration file"
    echo "- name is the name of the configuration (e.g. myconf)"
    echo 
    echo "$0 create myconf"
    echo "- will create a directory $(realpath -s $(dirname $0)/..)/lidarconfig-myconf with default files"
    echo 
    echo "$0 create myconf init path/to/initDir"
    echo "- will create a directory $(realpath -s $(dirname $0)/..)/lidarconfig-myconf and initialize it with the initDir content"
    echo 
    echo "$0 create path/to/osc"
    echo "- will create a directory $(realpath -s $(dirname $0)/..)/lidarconfig-osc and initialize it with the path/to/osc content"
    echo 
    echo "$0 +conf myconf server=192.168.1.100"
    echo "- will set the server ip in config.txt in directory $(realpath -s $(dirname $0)/..)/lidarconfig-myconf"
    echo "- special parameter is obstaclePersons=1 or obstaclePersons=2 which sets up the obstacle image"
    echo 
    echo "$0 createRegion regionName"
    echo "- will create a region with name regionName and default geometry. It can be edited in the web ui"
    echo 
    echo "when creating, parameters can also set via environment variables:"
    echo "server=192.168.1.100 $0 create myconf"
    echo "- will create a default config in directory $(realpath -s $(dirname $0)/..)/lidarconfig-myconf and set the server ip in config.txt"
    echo
    echo "linkConfDir=/tmp/altconf $0 create myconf"
    echo "- will create a default config in directory $(realpath -s $(dirname $0)/..)/lidarconfig-myconf and link the conf dir to /tmp/altconf"
    echo
    echo "$0 list"
    echo "- will list the variables defined in the local config.txt"
    echo 

    exit 0
}

nonint=false
create=false
list=false
updateBlueprint=false

params=()

while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
	+h|-h|-help|--help|+help|help)
        printHelp
	exit 0
	shift # past argument
	;;
	create)
        conf=$2
	if [ -d "$conf" ] ; then
	    initDir="$(pwd)/$2"
	    conf=$(basename "$initDir")
	fi
        nonint=true
	shift # past argument
	shift # past value
	;;
	init)
	initDir="$(pwd)/$2"
	if [ ! -d "$initDir" ] ; then
	    echo "error: directory \"$initDir\" does not exist !!!" 1>&2
	    exit 1
	fi
	shift # past argument
	shift # past value
	;;
	list)
	list=true
	shift # past argument
	;;
	createRegion|setRegion)
        regionName=$2
        nonint=true
	shift # past argument
	shift # past value
	break;
	;;
	updateBlueprint)
        nonint=true
	updateBlueprint=true
	shift # past argument
	;;
	+conf)
        conf=$2
	shift # past argument
	shift # past value
	;;
	nonint)
        nonint=true
	shift # past argument
	;;
	*)
	var="$(echo $1 | sed 's/=.*//')"
	if [ "$var" != "$1" ] ; then
	    params+=("$1")
	    nonint=true
	else
	    printHelp
	    exit 1
	fi
	shift # past argument
	;;
    esac
done

setDefaults
if [ "$conf" == "" ] ; then
    printHelp
    exit 1
fi

if $list ; then
    printValues
    exit 0
fi

confDir="$(realpath -s $(dirname $0)/..)/lidarconfig-$conf"

function createConfig () {

    lidarroot="$(realpath -s $(dirname $0)/..)"

    echo "creating config in $confDir"
    create=true

    if [ "$linkConfDir" != "" ] ; then
	mkdir -p "$linkConfDir"
	if [ ! -L "$confDir" ] ; then
	    ln -s "$linkConfDir" "$confDir"
	fi
    else
	mkdir -p "$confDir"
    fi

    cd "$confDir"

    mkdir -p "$conf"
    
    echo "conf=$conf" > config.txt
    echo '' >> config.txt

    echo "lidarroot=\"$lidarroot\""  >> config.txt

cat >> config.txt << EOL

useNodes=$useNodes
server=$server
portBase=$portBase
useServerGit=$useServerGit

webPort=$webPort
hubPort=$hubPort
adminPort=$adminPort

blueprintExtent=$blueprintExtent
blueprintImageFile=$blueprintImageFile
blueprintOcclusionFile=$blueprintOcclusionFile
blueprintSimulationFile=$blueprintSimulationFile
blueprintObstacleImageFile=$blueprintObstacleImageFile
blueprintObstacleExtent=$blueprintObstacleExtent

dataDir=$dataDir
recordPackedDir=$recordPackedDir

desktopColor=$desktopColor
desktopLabel=$desktopLabel

EOL

}

config="$confDir/config.txt"

if [ ! -f "$config" ] ; then
    
    if $nonint ; then
	answer=yes
    else
	read -p "create directory $confDir ? [y/N] " answer
    fi

    if [[ "$answer" =~ ^([Yy][Ee][Ss]|[Yy])$ ]] ; then
	createConfig $conf
    else
	exit 0
    fi
fi

source config.txt

lidartool="$lidarroot/lidartool"
lidaradmin="$lidarroot/lidaradmin"
lidarconfig="$lidarroot/lidarconfig"

cd $lidartool
if [ ! -L conf/$conf ] ; then
    ln -s "$confDir/$conf" conf/.
fi

cd $confDir

$lidarconfig/setupConfig.sh

if [ ! -f sensorDB.txt ] ; then
    echo "# Group Type Num NodeId Active MAC" > sensorDB.txt
    echo "# example: " >> sensorDB.txt
    echo "# samp  ld  01   001    +    xx:xx:xx:xx:xx:xx" >> sensorDB.txt
fi

if [ ! -f LidarSensors.txt ] ; then
    echo "# single lines with devices, e.g. for an LD06 at /dev/ttyUSB0" > LidarSensors.txt
    echo "# ld06:0" >> LidarSensors.txt
fi

if [ ! -f LidarRunMode.txt ] ; then
    if $useNodes ; then
	echo "simulation" > LidarRunMode.txt
    else
	echo "setup" > LidarRunMode.txt
    fi
fi

function argValue () {
      var="$(echo $1 | sed 's:=.*::')"
      value=$(echo "$1" | sed "s+$var=++")
      value=$(echo $value | sed 's+^[ ]*++g') # remove leading  whitespace
}

function paramValue () {

    line="$(grep $1= $config)"
    value=$(echo "$line" | sed "s+$1=++")
    if [ "$line" == "" ] ; then
	line="$(grep '#'$1= $config)"
	value='#'$(echo "$line" | sed "s+#$1=++")
    fi 
    value=$(echo $value | sed 's+^[ ]*++g') # remove leading  whitespace
}

function getValue () {

    paramValue $1
    if [ "$(echo $value | cut -c 1)" == "#" ] ; then
	value=
    fi
}

function putParam () {

    answer="$2"
    if [ "$answer" == "" ] ; then
	return
    fi

    answer=$(echo $answer | sed 's+^[ ]*++g') # remove leading  whitespace
    answer=$(echo $answer | sed 's+[ ]*$++g') # remove trailing whitespace
    if [ $(echo $answer | cut -c1) == "#" ] ; then
	answer=" $answer"
    fi

    if [ "$(grep $1= $config)" == "" ] ; then
	echo "# $3" >> $config
	echo "$1=$answer" >> $config
	echo "" >> $config
    else
	sed -i "s+^$1=.*+$1=$answer+" $config
    fi
}

function promptParam () {

    paramValue "$1"
    read -p "$2 ($1=$value): " answer
    putParam "$1" "$answer" "$2"
}

function updateBlueprintFiles {

    getValue blueprintImageFile
    imageFile="$value"
    getValue blueprintExtent
    bpExtent="$value"

    if [ "$imageFile" != "" ] && [ "$bpExtent" != "" ] ; then 

	cp -n "$imageFile" $conf/.

	$lidartool/lidarTool +conf $conf +setBlueprintImage  "$(basename $imageFile)"
	$lidartool/lidarTool +conf $conf +setBlueprintExtent "$bpExtent"

	getValue blueprintSimulationFile
	if [ "$value" != "" ] ; then
	    cp -n "$value" $conf/.
	    $lidartool/lidarTool +conf $conf +setBlueprintSimulationEnvMap "$(basename $value)"
	fi

	getValue blueprintOcclusionFile
	if [ "$value" != "" ] ; then
	    cp -n "$value" $conf/.
	    $lidartool/lidarTool +conf $conf +setBlueprintTrackOcclusionMap "$(basename $value)"
	fi
    fi

    if [ "$obstaclePersons" == "1" ] ; then
	blueprintObstacleImageFile="$lidarconfig/media/obstacleSinglePerson.png"
	blueprintObstacleExtent="0.7"
	putParam blueprintObstacleImageFile "$blueprintObstacleImageFile" "obstacle image file"
	putParam blueprintObstacleExtent "$blueprintObstacleExtent" "obstacle extent"
    elif [ "$obstaclePersions" == "2" ] ; then
	blueprintObstacleImageFile="$lidarconfig/media/obstacleTwoPersons.png" 
	blueprintObstacleExtent="2"
	putParam blueprintObstacleImageFile "$blueprintObstacleImageFile" "obstacle image file"
	putParam blueprintObstacleExtent "$blueprintObstacleExtent" "obstacle extent"
    fi

    getValue blueprintObstacleImageFile

    if [ "$value" != "" ] ; then
	cp -n "$value" $conf/.
	$lidartool/lidarTool +conf $conf +setBlueprintObstacleImage "$(basename $value)"
    fi

    getValue blueprintObstacleExtent
    bpObstacleExtent="$value"
    if [ "$bpObstacleExtent" != "" ] ; then 
	$lidartool/lidarTool +conf $conf +setBlueprintObstacleExtent "$bpObstacleExtent"
    fi
}

numParam=${#params[@]}

if [ $numParam -gt 0 ] ; then

    for ((i=0; i < $numParam; i++))
    do
      param="${params[$i]}"
      var="$(echo $param | sed 's:=.*::')"
      if [ "$var" == "obstaclePersons" ] ; then
	  eval $param
      elif [ "$var" == "runMode" ] ; then
	  argValue $param
	  echo "$value" > LidarRunMode.txt
      else
	  sed -i "s:^$var=.*:$param:g" $confDir/config.txt
      fi
    done
fi

updateBlueprintFiles

if [ "$initDir" != "" ] ; then
    GLOBIGNORE="$initDir/README.md:$initDir/*~:$initDir/.git:$initDir/.log"
    echo  cp -r "$initDir"/* .
    cp -r "$initDir"/* .

    [ -f observer.txt ] && mv observer.txt $conf/.
    if [ -e init_config.sh ] ; then
	echo ./init_config.sh
	./init_config.sh
	nonint=true
    fi
fi

exit 0

if [ "$regionName" != "" ] ; then
    $lidartool/lidarTool +conf $conf +setRegion $regionName $@
fi

if $nonint ; then
    exit
fi

promptParam useNodes  "Use nodes as virtual devices ?"
source config.txt

if $useNodes ; then
    promptParam server  "Admin server name or IP"
    promptParam portBase "prefix for virtal device ports"
    promptParam useServerGit "Manage registered nodes with git ?"
fi

promptParam webPort "Port to provide graphical UI per http"
promptParam adminPort "Port to provide admin access per http"

promptParam dataDir "directory where to store data. will be checked for full disk"
promptParam recordPackedDir "directory where to record packed data. leave empty for no recording"

promptParam blueprintExtent "Extent of blueprint in Pixel=Meter"

# blueprint image file name
paramValue blueprintImageFile
read -n1 -p "do you want to update the blueprint image file (blueprintImageFile=$value) ? [y/N] " answer
if [[ "$answer" =~ ^([Yy][Ee][Ss]|[Yy])$ ]] ; then
    file=$(nnn -p -)
    putParam blueprintImageFile "$file" "blueprint image file"

    updateBlueprintFiles
fi

# blueprint simulation file name
paramValue blueprintSimulationFile
read -n1 -p "do you want to update the blueprint simulation file (blueprintSimulationFile=$value) ? [y/N] " answer
if [[ "$answer" =~ ^([Yy][Ee][Ss]|[Yy])$ ]] ; then
    file=$(nnn -p -)
    putParam blueprintSimulationFile "$file" "blueprint simulation file"

    updateBlueprintFiles
fi

# blueprint occlusion file name
paramValue blueprintOcclusionFile
read -n1 -p "do you want to update the blueprint occlusion file (blueprintOcclusionFile=$value) ? [y/N] " answer
if [[ "$answer" =~ ^([Yy][Ee][Ss]|[Yy])$ ]] ; then
    file=$(nnn -p -)
    putParam blueprintOcclusionFile "$file" "blueprint occlusion file"

    updateBlueprintFiles
fi


# blueprint obstacle
paramValue blueprintObstacleImageFile
read -n1 -p "do you want to update the blueprint obstacle (blueprintObstacleImageFile=$value) ? [y/N] " answer
if [[ "$answer" =~ ^([Yy][Ee][Ss]|[Yy])$ ]] ; then
    
    echo
    read -n1 -p "single or double person ? [s/D] " answer
    echo

    if [[ "$answer" =~ ^([Ss])$ ]] ; then
	obstaclePersons="1"
    else
	obstaclePersons="2"
    fi

    updateBlueprintFiles
fi

# set desktop background for ubuntu mate
if [ "$(export | grep -i mate)" != "" ] ; then 

    read -n1 -p "do you want to update the desktop background (only Ubuntu MATE)? [y/N] " answer
    if [[ "$answer" =~ ^([Yy][Ee][Ss]|[Yy])$ ]] ; then
	promptParam desktopLabel "desktop label"
	promptParam desktopColor "desktop color"

	source $config
	convert -size 1920x1080 -background "#$desktopColor" -fill white -gravity center -pointsize 112 -font Helvetica label:"$desktopLabel\\nServer" $confDir/desktopBackground.jpg

	dconf write "/org/mate/desktop/background/picture-filename" "'$confDir/desktopBackground.jpg'"
    fi
fi

