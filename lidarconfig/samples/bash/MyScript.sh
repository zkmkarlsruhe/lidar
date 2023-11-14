#!/bin/bash

# parameter are handed over as environment variables:
# type = count|switch
# if type is switch
#    - switch = bool
# if type is count
#    - count = number
# @scriptParameter is a list of environment variable definitions which can be accessed here

if [ "$type" == "count" ] ; then
    echo "$0 count=$count region_name=\"$region\" param1=\"$param1\" param2=\"$param2\" param3=\"$param3\""
else
    echo "$0 switch=$switch region_name=\"$region\""
fi
