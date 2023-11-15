#!/bin/bash

[ -f config.txt ] && source config.txt

[ -f deviceFailed.sh ] && mv deviceFailed.sh $conf/.

# define useNodes as true
./editConfig.sh useNodes=true runMode=simulation

# define blueprint image with extension
#./editConfig.sh blueprintImageFile=../lidarconfig/media/SampleFloorPlan.png blueprintExtent=1372=58,23

# define blueprint simulation image
#./editConfig.sh blueprintSimulationFile=../lidarconfig/media/SampleFloorPlan_simulation.png 

# define default single persion obstacle image
./editConfig.sh obstaclePersons=1

# update simulation related files
./manageSensors.sh +s update

# update physical node related files
./manageSensors.sh update

# set the node password if defined in file sshPasswd.txt or it might be read at runtime from environment variable $SSHPASS
[ "$NODE_SSHPASS" != "" ] && echo "$NODE_SSHPASS" > sshPasswd.txt

