#!/bin/bash

if [ -d YDLidar-SDK ] ; then
    echo "YDLidar-SDK already exists"
else

    git clone https://github.com/YDLIDAR/YDLidar-SDK
    cd YDLidar-SDK
    git reset --hard 6e7e166

    git apply ../install/YDLidar-SDK_patch.diff
    cd ..
fi
    
cd YDLidar-SDK
mkdir -p build
cd build
cmake -D CMAKE_CXX_FLAGS="-Wno-dev" ..
#make $2 $3
cd ../..

