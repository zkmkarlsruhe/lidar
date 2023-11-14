#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

powerOff=false
loop=false
openOnStart=true

for arg do
  shift
  if [ "$arg" = "+loop" ] ; then
      loop=true
      continue
  elif [ "$arg" = "-openOnStart" ] ; then
      openOnStart=false
      continue
  fi
  set -- "$@" "$arg"
done

cmd=$(basename $0 .sh)
cd $(dirname $0)
EXECUTABLE="./$cmd"

ARGS=( "$@" )

while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
	+powerOff)
        powerOff=true
	shift # past argument
	;;
	isRunning)
	shift # past argument
	if [ "$1" != "" ] ; then 
	    arg="$1"
	    pattern=
	    patternconf=
	    if [ "${arg:0:1}" != "+" ] ; then
		patternconf="+conf $1 "
		shift # past argument
	    fi

	    while [[ $# -gt 0 ]]
	    do
		pattern+="$1 "
		shift # past argument
	    done
	      
	    pids=$(pgrep -xa $cmd|grep "$pattern"|grep "$patternconf")
	else
	    pids=$(pgrep -xa $cmd)
	fi
	[ -z "$pids" ] && echo false || echo true
	exit 0
	;;
	kill)
	shift # past argument
	if [ "$1" != "" ] ; then

	    arg="$1"
	    pattern=
	    patternconf=
	    if [ "${arg:0:1}" != "+" ] ; then
		patternconf="+conf $1 "
		shift # past argument
	    fi

	    while [[ $# -gt 0 ]]
	    do
		pattern+="$1 "
		shift # past argument
	    done

	    pids=$(echo $(pgrep -xa $cmd|grep "$pattern"|grep "$patternconf"|cut -d ' ' -f 1) $(pgrep -xa $cmd.sh|grep "$pattern"|grep "$patternconf"|cut -d ' ' -f 1))
	else
	    pids=$(echo $(pgrep -x $cmd) $(pgrep -x $cmd.sh))
	fi
	[ -z "$pids" ] || kill -9 $pids
	exit 0
	;;
	*) 
	shift # past argument
	;;
    esac
done

if $powerOff; then
    loop=false
fi

run()
{
    if $openOnStart ; then
	openOnStartParam=
    else
	openOnStartParam="-openOnStart"
	openOnStart=true
    fi
    
    
    $EXECUTABLE "${ARGS[@]}" $openOnStartParam
    exitCode="$?"
}


run
exitCode="$?"

while $loop
do
#  if [ "$exitCode" != "3" ] && [ "$exitCode" != "139" ] ; then # 3 is returned by start request, 139 is segfault
#      break
#  fi

  sleep 3
  run
done
