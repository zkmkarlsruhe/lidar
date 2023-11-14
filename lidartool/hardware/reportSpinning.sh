#!/bin/bash

REPORTSPINNINGINFLUXURL="http://localhost:8086/api/v2/write?bucket=db/rp&precision=ms"
REPORTSPINNINGINFLUXHEADER="Authorization: Token username:password"

for entry in $(echo "$1" | jq -c ".[]" )
  do
    id=$(echo "$entry" | jq -c ".id")
    spinning=$(echo "$entry" | jq -c ".spinning")
    sensorIN=$(echo "$entry" | jq -c ".sensorIN")

    if [ "$REPORTSPINNINGINFLUXURL" != "" ] && [ "$REPORTSPINNINGINFLUXHEADER" != "" ] && [ "$sensorIN" != "" ] ; then
	data="${INFLUXURL}SIN=$sensorIN,spinning=$spinning"
echo	curl -i -XPOST "$REPORTSPINNINGINFLUXURL" --header "$REPORTSPINNINGINFLUXHEADER" --data-raw "$data"
    else
	echo "device=$id sensorIN=$sensorIN spinning=$spinning"
    fi
  done

