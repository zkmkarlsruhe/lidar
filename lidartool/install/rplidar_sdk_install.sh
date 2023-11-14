#!/bin/bash

if [ -d rplidar_sdk ] ; then
    echo "rplidar_sdk already exists"
else

    git clone https://github.com/Slamtec/rplidar_sdk.git
    cd rplidar_sdk
#    git reset --hard 39d988a
    git reset --hard b9ab2c8
    git apply ../install/rplidar_sdk_patch.diff
    cd ..
fi
    
