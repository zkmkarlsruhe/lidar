#!/bin/bash

[ -f config.txt ] && source config.txt

[ -f reportSpinningSensors.sh ] && mv reportSpinningSensors.sh $conf/.
[ -f SensorIN.txt ] && mv SensorIN.txt $conf/.

