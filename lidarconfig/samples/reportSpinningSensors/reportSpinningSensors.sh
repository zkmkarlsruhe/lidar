#!/bin/bash

# REPORTSPINNINGINFLUXURL="http://localhost:8086/api/v2/write?bucket=db/rp&precision=ms"
# REPORTSPINNINGINFLUXHEADER="Authorization: Token username:password"

for entry in $(echo "$1" | jq -c ".[]" )
  do
    id=$(echo "$entry" | jq -c ".id")
    spinning=$(echo "$entry" | jq -c ".spinning")
    sensorIN=$(echo "$entry" | jq -c ".sensorIN")

    if [ "$REPORTSPINNINGINFLUXURL" != "" ] && [ "$REPORTSPINNINGINFLUXHEADER" != "" ] && [ "$sensorIN" != "" ] ; then

	if [ "$sensorIN" != "" ] ; then
	    data="${INFLUXURL}SIN=$sensorIN,spinning=$spinning"

	    if $verbose ; then 
		echo curl -i -XPOST "$REPORTSPINNINGINFLUXURL" --header "$REPORTSPINNINGINFLUXHEADER" --data-raw "$data"
	    fi
	    curl -i -XPOST "$REPORTSPINNINGINFLUXURL" --header "$REPORTSPINNINGINFLUXHEADER" --data-raw "$data"
	fi
    else
	if $verbose ; then 
	    echo "$0 device=$id sensorIN=$sensorIN spinning=$spinning"
	fi
    fi
  done

