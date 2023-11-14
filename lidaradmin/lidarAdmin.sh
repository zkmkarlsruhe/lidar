#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

loop=false

for arg do
  shift
  if [ "$arg" = "+loop" ] ; then
      loop=true
      continue
  fi
  set -- "$@" "$arg"
done

cmd=$(basename $0 .sh)

ARGS="$@"

while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
	kill)
	conf="$2"
	if [ "$conf" != "" ] ; then
	    pids=$(echo $(pgrep -ax $cmd.sh|grep "+conf $conf"|cut -d ' ' -f 1) $(pgrep -xa $cmd|grep "+conf $conf"|cut -d ' ' -f 1))
	else
	    pids=$(echo $(pgrep -xa $cmd) $(pgrep -xa $cmd.sh))
	fi

	[ -z "$pids" ] || kill -9 $pids
	shift # past argument
	exit 0
	;;
	*) 
	shift # past argument
	;;
    esac
done

while $loop
  do
  ./$cmd $ARGS

  sleep 3
done

