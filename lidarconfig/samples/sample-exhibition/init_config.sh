#!/bin/bash

[ -f config.txt ] && source config.txt

[ -f deviceFailed.sh ] && mv deviceFailed.sh $conf/.

#define useNodes as true
./editConfig.sh useNodes=true runMode=simulation

#define blueprint image with extension
./editConfig.sh blueprintImageFile=../lidarconfig/media/SampleFloorPlan.png blueprintExtent=1372=58,23

#define blueprint simulation image
./editConfig.sh blueprintSimulationFile=../lidarconfig/media/SampleFloorPlan_simulation.png 

#define default single persion obstacle image
./editConfig.sh obstaclePersons=1

# we have use already played the sensors and use the matrices. You would have to move them in the Web UI
[ -f LidarMatrix_dambck_ld01.txt         ] && mv LidarMatrix_*.txt         $conf/.
[ -f LidarEnv_dambck_ld01_Simulation.txt ] && mv LidarEnv_*_Simulation.txt $conf/.

# update the sensor related files, you cal also do that with the LidarAdmin UI 

# update simulation related files
./manageSensors.sh +s update
# update physical node related files
./manageSensors.sh update




