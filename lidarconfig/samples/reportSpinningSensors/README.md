# Report Spinning Sensor Identification Numbers

To keep track of spin times of sensor mechanics, one can frequently call a script in which the current state could be written to a database.

This example writes to an InfluxDB.

In case the lidarTool is used without starting it with StartServer.sh, the file reportSpinningSensors.sh can be copied to lidar/lidartool.

The sensorIDs are searched for in the directory [conf]/sensorIN.txt or lidartool/sensorIN.txt.

Copy [reportSpinningSensors.sh](reportSpinningSensors.sh) to the configuration subdirectory and edit it.

Create a text file with your sensor INs in [conf]/sensorIN.txt or lidartool/sensorIN.txt

See [reportSpinningSensors.sh](reportSpinningSensors.sh) for details on using the script.
