# Steps for setting up a complete Client Server System

## 1. install lidaradmin on your server

2. create a floor plan of your space

3. decide where to place sensord

4. create a lidar config on your server

5. make a simulation with your sensors

    Setup real sensors

1. prepare an SD Card for your Pis
   
   copy confix.txt to the SD Card
   
   clone SD Card

2. for each client node:
   
   1. determine MAC Adress
   
   2. entry mac adress in sensorDB.txt
   
   3. run sensorDB.sh update
   
   4. place client nodes on location, connect to network, boot

3. Define regions

4. Define observers
