#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

dir=$(basename $(realpath -s `dirname $0`))

verbose=
entry=

sleepTimeout=5

once=
try=
config=false

printHelp() {
    echo "usage: $(basename $0) [+v] [+try] [+once] [+timeout timeout] [entry] [+config [MAC]] "
    echo 
    exit 0
}

while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
	-h|--help|help)
        printHelp
	shift # past argument
	;;
	+try)
        try=$1
	shift # past argument
	;;
	+once)
        once=$1
	shift # past argument
	;;
	+config)
        config=true
        HWaddr="$2"
	shift # past argument
	shift # past value
	;;
	+timeout)
        sleepTimeout="$2"
	shift # past argument
	shift # past value
	;;
	+v)
        verbose=$1
	shift # past argument
	;;
	*)
	argentry="$1"
	shift # past argument
	;;
    esac
done


if $config ; then

    configServer=lidar-config
    configPort=8888

    if [ "$LIDAR_CONFIG_SERVER" != "" ] ; then
	configServer="$LIDAR_CONFIG_SERVER"
    fi

    if [ "$LIDAR_CONFIG_PORT" != "" ] ; then
	configPort="$LIDAR_CONFIG_PORT"
    fi

    if [ -f config_server.txt ] ; then
	source config_server.txt
    fi

    if [ "$argentry" == "" ] ; then
	entry="$(./install_node.sh entry)"
    else
	entry="$argentry"
    fi

    timeout=3

    if [ "$verbose" != "" ] ; then
	echo "wget --timeout=$timeout http://$configServer:$configPort/config?entry=$entry"
    fi
    result=$(wget --timeout=$timeout "http://$configServer:$configPort/config?entry=$entry" -O - -o /dev/null)

    if [ "$result" != "" ] ; then
	echo "$result" > config.txt
    fi

    exit 0

fi

if [ -f config.txt ] ; then
    source config.txt
else
    source config_default.txt
fi

if [ "$server" == "" ] ; then
	echo "missing server definition !!!"
	echo "please create a config.txt file on your server and copy it to $(pwd)/config.txt on this client"
	exit 1
fi

success=false

while [ 1 ]
do
  if [ "$argentry" == "" ] ; then
      entry="$(./install_node.sh entry)"
  else
      entry="$argentry"
  fi

  if [ "$verbose" != "" ] ; then
      echo "wget http://$server:$adminPort/nodes?register=$entry"
  fi
  result=$(wget "http://$server:$adminPort/nodes?register=$entry" -O - -o /dev/null)

  if [ "$result" != "" ] ; then

      if $(echo "$result" | jq 'has("nodeId")') ; then
	  nodeId=$(jq .nodeId <<<"$result" | tr -d '"')
      else
	  nodeId=
      fi

      if $(echo "$result" | jq 'has("deviceType")') ; then
	  deviceType=$(jq .deviceType <<<"$result" | tr -d '"')
      else
	  deviceType=
      fi

      if [ -f LidarDeviceType.txt ] ; then
          LidarDeviceType="$(cat LidarDeviceType.txt)"
      fi

      if [ $(echo $nodeId | wc -c) == 4 ] ; then

	  if [ "$deviceType" != "$LidarDeviceType" ] ; then
	      if [ "$verbose" != "" ] ; then
		  echo setting deviceType $deviceType
	      fi
	      if [ "$deviceType" == "" ] ; then
		  rm -f LidarDeviceType.txt
	      else
		  echo $deviceType > LidarDeviceType.txt
	      fi
	  fi

	  if [ "$nodeId" != "$(cat LidarNodeId.txt)" ] ; then
	      if [ "$verbose" != "" ] ; then
		  echo setting nodeId $nodeId
	      fi

	      echo $nodeId > LidarNodeId.txt

	      sudo ./install_node.sh update

	      if [ "$argentry" == "" ] ; then
		  entry="$(./install_node.sh entry)"
	      else
		  entry="$argentry"
	      fi
	      if [ "$verbose" != "" ] ; then
		  echo "wget http://$server:$adminPort/nodes?register=$entry"
	      fi
	      wget "http://$server:$adminPort/nodes?register=$entry" -O - -o /dev/null

	      if [ "$once" != "" ] ; then
		  exit 1
	      fi
	  fi

	  if [ "$once" != "" ] ; then
	      exit 0
	  fi
	  
	  success=true
      fi
  fi

  if [ "$try" != "" ] ; then
      exit 0
  fi

  if $success ; then

      # if successful try again every hour

      if [ "$time" == "" ] ; then
	  time=$(shuf -i 800-2400 -n 1)
	  sleep $time
      else
	  time=$((3600-$time))
	  sleep $time
	  time=
      fi
  else
      # otherwise every sleepTimeout seconds
      sleep $sleepTimeout
  fi
done

