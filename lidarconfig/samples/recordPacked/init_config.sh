#!/bin/bash

[ -f config.txt ] && source config.txt

recordPackedDir=$(pwd)/data
mkdir -p $recordPackedDir

# set variable recordPackedDir to an existing directory
./editConfig.sh recordPackedDir=$recordPackedDir


# recording is done only in production mode
echo "production" > LidarRunMode.txt


