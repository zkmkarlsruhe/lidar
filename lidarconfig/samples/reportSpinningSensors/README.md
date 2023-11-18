# Report Spinning Sensor Identification Numbers

To keep track of spin times of sensor mechanics, a script in which the current state is reported to a database, can frequently be called.

This example writes to an InfluxDB.

lidarTool searches for the sensorIDs in the directory [conf]/sensorIN.txt and then in lidartool/sensorIN.txt.

In case the lidarTool is used without starting it with StartServer.sh, the script file reportSpinningSensors.sh can be copied to lidar/lidartool.

Copy reportSpinningSensors.sh to the configuration subdirectory and edit it.

Create a text file with your sensor INs in [conf]/sensorIN.txt or lidartool/sensorIN.txt

See reportSpinningSensors.sh for details on using the script.
