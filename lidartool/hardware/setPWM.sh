#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

chip=0
channel=0
polarity="normal"

period="24000"
offPWM="240"
onPWM="8400"
speed="1"

help() {

    echo "usage: $1 [-h] [+chip chip] [+channel channel] [+period nsec] [+offPWM nsec] [+onPWM nsec] on|off|speed"
    echo ""
    echo "  chip    = $chip"
    echo "  channel = $channel"
    echo "  period  = $period"
    echo "  offPWM  = $offPWM"
    echo "  onPWM   = $onPWM"

    pwmchip=pwmchip$chip
    pwm=/sys/class/pwm/$pwmchip/pwm$channel

    echo "  $pwm"
    echo ""
}

if [ "$1" == "" ] ; then
    help
    exit 0
fi

while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
	+h|+help|-h|-help|--help)
        help
        exit 0
        shift # past value
	;;
	+chip)
        chip=$2
	shift # past argument
	shift # past value
	;;
	+channel)
        channel=$2
	shift # past argument
	shift # past value
	;;
	+period)
        period=$2
	shift # past argument
	shift # past value
	;;
	+offPWM)
        offPWM=$2
	shift # past argument
	shift # past value
	;;
	+onPWM)
        onPWM=$2
	shift # past argument
	shift # past value
	;;
	on)
        speed="1.0"
	shift # past argument
	shift # past value
	;;
	off)
        speed="0.0"
	shift # past argument
	shift # past value
	;;
	*) 
        help
        exit 0
	shift # past argument
	;;
    esac
done

pwmchip=pwmchip$chip
pwm=/sys/class/pwm/$pwmchip/pwm$channel

duty_cycle=$(bc <<< "$offPWM+($onPWM-$offPWM)*$speed/1")

if [ ! -d "$pwm" ] ; then
	echo 0 > /sys/class/pwm/$pwmchip/export
fi

if [ "$(cat $pwm/polarity)" != "$polarity" ] ; then
    echo $polarity   > $pwm/polarity
fi

echo $period     > $pwm/period
echo $duty_cycle > $pwm/duty_cycle

if [ "$(cat $pwm/enable)" != "1" ] ; then
	echo 1 > $pwm/enable
fi


