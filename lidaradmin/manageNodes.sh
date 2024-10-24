#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

if [ ! -f config.txt ] ; then
    echo "ERROR: no config.txt in directory $(pwd)"
    exit 0
fi

source config.txt

localroot="$HOME/lidar"
localnode="$localroot/lidarnode"
localadmin="$localroot/lidaradmin"
localtool="$localroot/lidartool"

remoteroot="lidar"
remotenode="$remoteroot/lidarnode"
remoteadmin="$remoteroot/lidaradmin"
remotetool="$remoteroot/lidartool"

if [ -f modelMap.txt ] ; then
    source modelMap.txt
fi

simulationMode=
testMode=

if [ "$nodeDB" == "" ] ; then
    nodeDB="nodeDB.txt"
fi

if [ "$sensorDB" == "" ] ; then
    sensorDB="sensorDB.txt"
fi

cmd=
local=
remoteCmd=
verbose=
ip=
node=

pingTimeout=2
sshTimeout=3
batchSize=5
exec=

waitForCompletion=true
doBatch=false

noIP="10.0.0.1"

sshpass=""
if [ -f sshPasswd.txt ] ; then
    sshpass="sshpass -f sshPasswd.txt "
elif [ "$SSHPASS" != "" ] ; then
    sshpass="sshpass -e "
fi

pushd() { builtin pushd $1 > /dev/null; }
popd()  { builtin popd  $1 > /dev/null; }

repairGit() {
    
    find .git/objects -type f -empty -delete
    git prune
    git fetch --all --prune
    git reset HEAD --hard
    git pull
}

restartIfRunning() {

    isRunning=$($localtool/lidarTool.sh isRunning)

    if $isRunning ; then
	$localtool/lidarTool.sh kill sleep 3 $conf
	( ./StartNode.sh ) &
    fi
}

printHelp() {
    echo "usage: $(basename $0) [+t] [+v] [ip IPadress|node nodeId|sequential] startSensors|stopSensors|run|rerun|isRunning|kill|monitor [setup|pull|clean|make|permissions|up|list|reboot|shutdown|repair|exec|batch cmd|copyFile src [target]|setNodeId IPadress nodeId|enablePower bool|on|off|+q"
    echo 
    exit 0
}

isUp() {
    if [ "$IPaddr" == "" ] || [ "$IPaddr" == "$noIP" ]  ; then
	return 0
    fi

    ping -w $pingTimeout -c 1 $1 > /dev/null
    if [ $? -eq 0 ] ; then
	return 0
    else
	return 1
    fi
}

checkUp() {

    rm -f $1

    if [ -f "$nodeDB" ] ; then
	readarray nodes < $nodeDB
    fi

    numNodes=${#nodes[@]}

    for ((i=0; i < $numNodes; i++))
    do
      entry="${nodes[$i]}"
      
      IPaddr=$(echo $entry | awk '{print $2}')
      nodeId=$(echo $entry | awk '{print $4}')

      if [ "$IPaddr" != "" ] ; then
	  (
	      if isUp $IPaddr ; then 
		  echo "up   $IPaddr" >> $1
	      else
		  echo "down $IPaddr" >> $1
	      fi
	  ) &
      fi

    done

    for job in `jobs -p`
      do
      wait $job
    done
}

readSensors() {

    sensors=()
    
    if [ -f "$sensorDB" ] ; then

	readarray sensorArray < "$sensorDB"

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

readNodes() {

    nodes=()
    
    if [ -f "$nodeDB" ] ; then

	readarray nodeArray < "$nodeDB"

	numNodes=${#nodeArray[@]}

	for ((i=0; i < $numNodes; i++))
	  do
	  entry="${nodeArray[$i]}"
      
	  first=$(echo $entry | awk '{print $1}')

	  if [ "$first" != "" ] && [ "$(echo $first | cut -c 1)" != "#" ] ; then
	      nodes+=("$entry")
	  fi
      
	done
    fi
}

readNodes

nodeInfo() {

    for ((i=0; i < $numNodes; i++))
    do
      entry="${nodes[$i]}"
      
      IPaddr=$(echo $entry | awk '{print $2}')

      if [ "$IPaddr" == "$1" ] ; then
	  nodeId=$(echo $entry | awk '{print $3}')
	  HWaddr=$(echo $entry | awk '{print $1}')
	  User=$(echo $entry | awk '{print $4}')
	  Model=$(echo $entry | awk '{print $5}')

	  echo $nodeId $HWaddr $User $Model
	  return
      fi

    done
}

entryExists() {

    if [ ! -f "$2" ] ; then
	echo 0
	return
    fi
    
    readarray nodes1 < "$2"

    numNodes1=${#nodes1[@]}
    
    for ((i=0; i < $numNodes1; i++))
    do
      entry="$(echo ${nodes1[$i]} | tr -d '\r')"
      if [ "$entry" == "$1" ] ; then
	  echo 1
	  return
      fi
    done

    echo 0
}

macExists() {

    if [ ! -f "$2" ] ; then
	echo 0
	return
    fi

    for ((i=0; i < $numNodes; i++))
    do
      entry="$(echo ${nodes[$i]} | tr -d '\r' | awk '{print $1}')"
      if [ "$entry" == "$1" ] ; then
	  echo 1
	  return
      fi
    done

    echo 0
}


hasNodes() {

    if [ ! -f "$nodeDB" ] ; then
	echo 0
	exit 0
    fi

    readarray nodes1 < "$nodeDB"
	
    numNodes1=${#nodes1[@]}
    
    for ((i=0; i < $numNodes1; i++))
    do
      entry="$(echo ${nodes1[$i]} | tr -d '\r')"
      if [ "$entry" != "" ] ; then
	  echo 1
	  return
      fi
    done

    echo 0
}


checkDiff() {

    readarray nodes0 < $2

    numNodes0=${#nodes0[@]}
    
    for ((i=0; i < $numNodes0; i++))
    do
      entry="$(echo ${nodes0[$i]} | tr -d '\r')"
      if [ "$entry" != "" ] ; then
	  exists=$(entryExists "$entry" "$1")
	  if [ "$monitorArg" == "list" ] || [ "$exists" == "0" ] ; then
	      IPaddr=$(echo $entry | awk '{print $2}')
	      if [ "$verbose" != "" ] ; then
		  echo "$entry" $(nodeInfo $IPaddr)
	      else
		  echo "$entry"
	      fi
	  fi
      fi
    done
}

monitor() {

    file0="/tmp/$(basename $0 .sh)-0.txt"
    file1="/tmp/$(basename $0 .sh)-1.txt"

    checkUp $file0

    echo Ready...

    while [ 1 ]
    do
      mv $file0 $file1
      checkUp $file0

      checkDiff $file1 $file0

    done
}


sensorInfo() {
     
    numSensors=${#sensors[@]}

    for ((i=0; i < $numSensors; i++))
    do
      entry="${sensors[$i]}"
      
      group=$(echo $entry | awk '{print $1}')

      if [ "$group" != "" ] ; then

	  mac=$(echo $entry | awk '{print $6}' | tr "[:upper:]" "[:lower:]")

	  if [ "$1" == "$mac" ] ; then
	      model=$(echo $entry | awk '{print $2}')
	      groupId=$(echo $entry | awk '{print $3}')
	      nodeId=$(echo $entry | awk '{print $4}')
	      active=$(echo $entry | awk '{print $5}')
	      devType=${modelMap[$model]}

	      echo ${group}_${model}${groupId} $nodeId $devType $2 $3 $active
	  fi
      fi

    done
}

registerSensors() {
     
    readSensors

    numSensors=${#sensors[@]}

    hasCommit="nothing"

    for ((i=0; i < $numSensors; i++))
    do
      entry="${sensors[$i]}"
      
      group=$(echo $entry | awk '{print $1}')

      if [ "$group" != "" ] ; then
	  mac=$(echo $entry | awk '{print $6}' | tr "[:upper:]" "[:lower:]")
	  if [ "$mac" != "" ] && [ "$(macExists $mac $nodeDB)" == "0" ] ; then
	      new_entry="$mac $noIP 000 unknown unknown _ pwUndef"

	      echo "$new_entry" >> $nodeDB
	      if $useServerGit ; then
		  hasCommit=$(git commit $nodeDB -m "added node $new_entry" | grep nothing)
	      fi
	fi
      fi

    done

    if [ "$hasCommit" == "" ]; then
	git push &> /dev/zero
    fi
}


while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
	-h|--help|help)
        printHelp
	shift # past argument
	;;
	permissions)
        remoteCmd=$1
	shift # past argument
	;;
	ping|up|list)
        remoteCmd=ping
	if [ "$1" == "list" ] ; then
	    verbose="+v"
	fi
	shift # past argument
	;;
	local)
        local="local"
	shift # past argument
	;;
	ip)
        ip="$2"
	shift # past argument
	shift # past value
	;;
	sequential)
        sequential="$1"
	shift # past argument
	;;
	node)
        node="$2"
	shift # past argument
	shift # past value
	;;
	setNodeId)
        ip="$2"
        nodeId="$3"
	remoteCmd="nodeId $nodeId"

	shift # past cmd
	shift # past ip
	shift # past nodeId
	;;
	nodeId)
        nodeId="$2"

	if [ "$testMode" == "" ] ; then
	    ./install_node.sh $nodeId
	else
	    echo ./install_node.sh $nodeId
	fi

	shift # past nodeId
	shift # past value
	;;
	setDeviceType)
        deviceType="$2"
	remoteCmd="deviceType \"$deviceType\""

	shift # past cmd
	shift # past deviceType
	;;
	deviceType)
        deviceType="$2"

	if [ "$testMode" == "" ] ; then
	    if [ "$deviceType" == "" ] ; then
		rm -f LidarDeviceType.txt
	    else
		echo $deviceType > LidarDeviceType.txt
	    fi
	else
	    if [ "$deviceType" == "" ] ; then
		echo "rm  LidarDeviceType.txt"
	    else
		echo "echo $deviceType \> LidarDeviceType.txt"
	    fi
	fi

	shift # past arg
	shift # past value
	;;
	enablePower)
        enablePower="$2"

        if [ "$local" != "" ] ; then
	    if [ "$testMode" == "" ] ; then
		echo "$enablePower" > "$localtool/hardware/LidarPower.enable"
	    fi
	    echo $testMode $verbose
	    if [ "$testMode" != "" ] || [ "$verbose" != "" ] ; then
		echo "echo $enablePower \> $localtool/hardware/LidarPower.enable"
	    fi
	else
	    remoteCmd="local $1 $enablePower"
	    doBatch=true
	fi

	shift # past arg
	shift # past value
	;;
	repair)
        if [ "$local" != "" ] ; then
	    if [ "$testMode" == "" ] ; then
		repairGit;
	    else
		echo "repair git"
	    fi
	    exit 0
	else
	    remoteCmd="local $1"
	    doBatch=true
	fi
	shift # past argument
	;;
	reboot|shutdown)
        if [ "$local" != "" ] ; then
	    if [ "$testMode" == "" ] ; then
		./StopNode.sh
		if [ "$1" == "shutdown"  ] ; then
		    sudo init 0
		else
		    sudo "$1"
		fi
	    else
		if [ "$1" == "shutdown"  ] ; then
		    echo "sudo init 0"
		else
		    echo sudo "$1"
		fi
	    fi
	else
	    remoteCmd="local $1"
	fi
	shift # past argument
	;;
	hasNodes)
	hasNodes
	shift # past argument
	exit 0
	;;
	isRunning)
        if [ "$local" != "" ] ; then
	    isRunning=$($localtool/lidarTool.sh isRunning)
	    if $isRunning ; then
		echo 1
	    else
		echo 0
	    fi
	    exit 0
	else
	    remoteCmd="local $1"
	fi
	shift # past argument
	;;
	run|kill|rerun)
	cmd=run
	waitForCompletion=false
        if [ "$local" != "" ] ; then
	    isRunning=$($localtool/lidarTool.sh isRunning)
	    if $isRunning ; then
		if [ "$1" == "run" ] ; then
		    exit 0
		fi
	    fi
	    if [ "$testMode" == "" ] ; then
		if $isRunning ; then
		    ./StopNode.sh
		fi
		if [ "$1" == "run" ] || [ "$1" == "rerun" ] ; then
		    if [ "$1" == "rerun" ] ; then
			sleep 5
		    fi
		    if [ "$verbose" != "" ] ; then
			( ./StartNode.sh 2>&1 ) &
		    else
		    	( ./StartNode.sh 2> /dev/null > /dev/null ) &
		    fi
		    sleep 1
		fi
	    else
		if $isRunning ; then
		    echo ./StopNode.sh
		fi
		if [ "$1" == "run" ] || [ "$1" == "rerun" ] ; then
		    echo ./StartNode.sh
		fi
	    fi
	else
	    remoteCmd="local $1"
	fi
	shift # past argument
	;;
	start|stop|startSensors|stopSensors)
        if [ "$local" != "" ] ; then
	    url="http://localhost:$webPort/$1" 
#	    echo $url
	    if [ "$testMode" == "" ] ; then
		wget "$url" -q -O /dev/null &>> /dev/null >> /dev/null &
	    else
		echo "wget $url -q -O /dev/null &>> /dev/null >> /dev/null &"
	    fi
	else
	    remoteCmd="local $1"
	fi
	shift # past argument
	;;
	unused)
        cmd="$2"
	shift # past argument
	shift # past value
	
	nodeList=$($0 list | grep -v +)

	readarray -t array <<<"$nodeList"

	for element in "${array[@]}"
        do
	  line=($(echo $element | tr ',' "\n"))
	  ip=${line[2]}

	  if [ "$testMode" == "" ] ; then
	      if [ "$verbose" != "" ] ; then
		  ( ./manageNodes.sh $verbose ip $ip $cmd 2>&1 ) &
	      else
		  ( ./manageNodes.sh $verbose ip $ip $cmd 2> /dev/null > /dev/null ) &
	      fi
	  else
	      echo ./manageNodes.sh $verbose ip $ip $cmd
	  fi
	done
	exit 0
	;;
	register)
        new_entry="$2"
	
	HWaddr=$(echo $new_entry | awk '{print $1}')
        old_entry="$([ -f $nodeDB ] && grep $HWaddr $nodeDB)"

	readSensors
	sensorEntry=$(sensorInfo $HWaddr)

	nodeId=$(echo $sensorEntry | awk '{print $2}')
	devType=$(echo $sensorEntry | awk '{print $3}')

	if [ "$nodeId" == "" ] || [ "$(echo $nodeId | grep ':')" != "" ] ; then
	    nodeId="000"
	fi

	result="{ \"nodeId\": \"$nodeId\""
	if [ "$devType" != "" ] ; then
	    result="$result, \"deviceType\": \"$devType\""
	fi
	result="$result }"

	echo $result

	if $useServerGit ; then
	    git pull &> /dev/null
	fi

	hasCommit="nothing"
	if [ "$old_entry" == "" ] ; then
	    echo "$new_entry" >> $nodeDB
	    if $useServerGit ; then
		hasCommit=$(git commit $nodeDB -m "added node $new_entry" | grep nothing)
	    fi
	elif [ "$old_entry" != "$new_entry" ] ; then
	    sed -i "s/^$HWaddr.*/$new_entry/" $nodeDB
	    if $useServerGit ; then
		hasCommit=$(git commit $nodeDB -m "updated node $new_entry" | grep nothing)
	    fi
	fi
	if [ "$hasCommit" == "" ]; then
	    git push &> /dev/zero
	fi

	shift # past argument
	break
	;;
	remove)
	HWaddr="$2"

	if $useServerGit ; then
	    git pull &> /dev/null
	fi

	hasCommit="nothing"
	if [ "$(macExists $HWaddr $nodeDB)" == "1" ] ; then
	    sed -i "/^$HWaddr/d" $nodeDB
	    if $useServerGit ; then
		hasCommit=$(git commit $nodeDB -m "removed entry $HWaddr" | grep nothing)
	    fi
	fi
	if [ "$hasCommit" == "" ]; then
	    git push &> /dev/zero
	fi

	shift # past argument
	break
	;;
	exec)
        exec="$1"
	shift # past argument
        remoteCmd="$@"
	break
	;;
	copyFile)
        exec="exec"
        remoteCmd="copyFile"
	shift # past argument
        srcFile="$@"
	shift # past argument
        dstFile="$@"
	if [ "$dstFile" == "" ] ; then
	    dstFile="$srcFile"
	fi
 	break
	;;
	batch)
        exec="$1"
	doBatch=true
	shift # past argument
        remoteCmd="$@"
	break
	;;
	setup)
	remoteCmd="setup"
	doBatch=true
	shift # past argument
	;;
	pull)
	remoteCmd="pull"
	doBatch=true
	shift # past argument
	;;
	clean)
        if [ "$local" != "" ] ; then
	    if [ "$testMode" == "" ] ; then
		cd $localtool
		make clean
	    else
		echo cd $localtool
		echo make clean
	    fi
	else
	    remoteCmd="local $1"
	fi
	shift # past argument
	;;
	make)
        if [ "$local" != "" ] ; then
	    if [ "$testMode" == "" ] ; then
		./StopNode.sh
		pushd $localtool
		make
		popd
	    else
		echo ./StopNode.sh
		echo pushd $localtool
		echo make
		popd
	    fi
	else
	    remoteCmd="local $1"
	fi
	shift # past argument
	;;
	monitor)
	remoteCmd="$1"
	shift # past argument
	if [ "$2" == "list" ] ; then
	    monitorArg="$2"
	    shift # past value
	fi
	;;
	+t)
        testMode=$1
	shift # past argument
	;;
	+v)
        verbose=$1
	shift # past argument
	;;
	*) 
	shift # past argument
	;;
    esac
done

if [ "$remoteCmd" != "" ] ; then

    if [ "$remoteCmd" == "monitor" ] ; then
	monitor
	exit 0
    fi

    if  [ "$remoteCmd" == "setup" ] ; then
	runMode=""
	if [ -f LidarRunMode.txt ] ; then 
	    if [ "$(cat LidarRunMode.txt)" == "simulation" ] ; then
		runMode="+s"
	    fi
	fi

	./manageSensors.sh +q $testMode $runMode update

	registerSensors
	if [ "$runMode" == "simulation" ] ; then
	    exit 0
	fi
    fi

    if [ "$remoteCmd" == "ping" ] ; then
	readSensors
    fi

    numNodes=${#nodes[@]}

    batch=0
    for ((i=0; i < $numNodes; i++))
    do
      entry="${nodes[$i]}"
      
      HWaddr=$(echo $entry | awk '{print $1}')
      IPaddr=$(echo $entry | awk '{print $2}')
      nodeId=$(echo $entry | awk '{print $3}')

      if [ "$IPaddr" != "" ] ; then

	  doIt=true

	  if [ "$ip" != "" ] && [ "$IPaddr" != "$ip" ] ; then
	      doIt=false
	  fi

	  if [ "$node" != "" ] && [ "$nodeId" != "$node" ] ; then
	      doIt=false
	  fi

	  if $doIt ; then
	      
	      batch=$((batch+1))

	      if $waitForCompletion && $doBatch && [ $batch -gt $batchSize ] ; then
		  batch=1
		  for job in `jobs -p`
		  do
		    wait $job
		  done
	      fi

	      user=$(echo $entry | awk '{print $4}')

	      if [ "$remoteCmd" == "ping" ] || [ "$enablePower" != "" ] ; then

		  model=$(echo $entry | awk '{print $5}')
		  sensorIN=$(echo $entry | awk '{print $6}')
		  if [ "$sensorIN" == "" ] ; then
		      sensorIN="_";
		  fi
		  if [ "$enablePower" != "" ] ; then
		      if [ "$enablePower" == "true" ] ; then
			  sensorPW="pwEn";
		      elif [ "$enablePower" == "false" ] ; then
			  sensorPW="pwDis";
		      else
			  sensorPW="pwUndef";
		      fi
		  else
		      sensorPW=$(echo $entry | awk '{print $7}')
		      if [ "$sensorPW" == "" ] ; then
			  sensorPW="pwDis";
		      fi
		  fi
		  
		  if [ "$HWaddr" == "" ] ; then
		      HWaddr=$(echo $(nodeInfo $IPaddr) | awk '{print $2}')
		      echo got $HWaddr
		  fi

		  sensorEntry="$(sensorInfo $HWaddr $sensorIN $sensorPW)"
	      fi

	      if [ "$enablePower" != "" ] ; then
		  new_entry="$HWaddr $IPaddr $nodeId $user $model $sensorIN $sensorPW"

		  if [ "$test" != "" ] || [ "$verbose" != "" ] ; then
		      echo $0 register "$new_entry"
		  fi
		  
		  if [ "$test" == "" ] ; then
		      $0 register "$new_entry" > /dev/null
		  fi
	      fi

	      if [ "$remoteCmd" == "ping" ] ; then
		  (
		      entry="$HWaddr $IPaddr $nodeId $user $model"

		      if isUp $IPaddr ; then
			  up=up
		      else
			  up=
		      fi
		      if [ "$verbose" != "" ] ; then
			  if [ "$up" != "" ] ; then
			      sshRemote="${sshpass}ssh -o ConnectTimeout=$sshTimeout -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null $user@$IPaddr "
			      sshCmd="cd ~/$remotenode; ./manageNodes.sh $testMode local isRunning"
			      isRunning=$($sshRemote $sshCmd 2> /dev/null)

			      if [ "$isRunning" == "1" ] ; then
				  info="$entry running $sensorEntry"
			      else
				  info="$entry stopped $sensorEntry"
			      fi
			  else
			      info="$entry unknown $sensorEntry"
			  fi
		      else
			  info="$IPaddr"
		      fi
		      
		      if [ "$up" != "" ] ; then
			  echo "up  " $info
		      else
			  echo "down" $info
		      fi
		  ) &
	      elif [ "$remoteCmd" == "copyFile" ] ; then
		  (
		      if [ "$verbose" != "" ] ; then
			  echo scp \-o ConnectTimeout=$sshTimeout \-o StrictHostKeyChecking=no \-o UserKnownHostsFile=/dev/null $srcFile $user@$IPaddr:$dstFile
		      fi

		      if [ "$testMode" == "" ] ; then
			  ${sshpass}scp -o ConnectTimeout=$sshTimeout -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null $srcFile $user@$IPaddr:$dstFile
		      fi
		  ) &
	      else
		  sshRemote="${sshpass}ssh -o ConnectTimeout=$sshTimeout -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null $user@$IPaddr "

		  if  [ "$remoteCmd" == "permissions" ] ; then
		      sshCmd="echo ok"
		      
		      ( if isUp $IPaddr ; then
			    ok=$($sshRemote $sshCmd 2> /dev/null)
			    if [ "$ok" == "ok" ] ; then
				echo "permissions ok     $user@$IPaddr"
			    else
				echo "permissions failed $user@$IPaddr"
			    fi
		        else
		           echo "host down          $user@$IPaddr"
		        fi
		      ) &
		  else
		      if [ "$exec" != "" ] ; then
			  sshCmd="$remoteCmd"
		      elif [ "$remoteCmd" == "pull" ] ; then
			  sshCmd="cd ~/$remoteadmin; git pull; cd ../lidarnode; git pull; ./StopNode.sh; cd ../lidartool; git pull; "
		      elif [ "$remoteCmd" == "setup" ] ; then
			  sshCmd="cd ~/$remotenode; ./StopNode.sh; ( ./registerNode.sh +once +try &); sleep 3; ./install_node.sh update"
		      elif [ "$remoteCmd" == "repair" ] ; then
			  sshCmd="cd ~/$remoteadmin; ./manageNodes.sh repair; cd ../lidarnode; ./manageNodes.sh repair; cd ../lidartool; ../lidaradmin/manageNodes.sh repair"
		      elif [ "$enablePower" != "" ] ; then
			  sshCmd="cd ~/$remotenode; ./manageNodes.sh $testMode $remoteCmd"
		      else
			  sshCmd="cd ~/$remotenode; ./manageNodes.sh +v $testMode $remoteCmd"
		      fi

		      if [ "$verbose" != "" ] ; then
			  echo $sshRemote \"$sshCmd\"
		      fi

		      if [ "$testMode" == "" ] ; then

			  if  [ "$sequential" == "" ] ; then
			  ( if [ "$verbose" != "" ] ; then
				result="$($sshRemote $sshCmd)"
			    else
				result="$($sshRemote $sshCmd 2> /dev/zero)"
			    fi

			    if [ "$result" != "" ] ; then
				echo "$IPaddr:" $result
			    fi
			  ) &
			  else
			    if  [ "$remoteCmd" == "local reboot" ] ; then
			      waitTime=$(bc -l <<< "$(shuf -i 10-100 -n 1)/50")
			      sleep $waitTime
			    fi

		            if [ "$verbose" != "" ] ; then
				result="$($sshRemote $sshCmd)"
			    else
				result="$($sshRemote $sshCmd 2> /dev/zero)"
			    fi

			    if [ "$result" != "" ] ; then
				echo "$IPaddr:" $result
			    fi
		          fi
		      fi
		  fi
	      fi
	  fi
      fi

    done

fi

if $waitForCompletion ; then
    for job in `jobs -p`
    do
      wait $job
    done
fi

#if  [ "$remoteCmd" == "setup" ] ; then
#    restartIfRunning
#fi