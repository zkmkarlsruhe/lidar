# Lidar Config

To create or edit a lidar configuration:

```console
cd lidar/lidarconfig
./manageConfig myconf
```

Follow the instructions.

In case it does not exist, this creates the directory `lidar/lidarconfig-myconf` with default files and the file *config.txt*.

# Nodes vs. Local Devices

#### Local Devices

If you have sensors locally connected to the computer, set *useNodes* to **false** (default). The sensors to use are stored in file *LidarSensors.txt*.

```console
./editConfig.sh create myconf useNodes=false
```

#### Nodes

If you want to set up a configuration with several nodes running on Raspis, set *useNodes* to **true**. The definitions of the virtual sensors are stored in file *sensorDB.txt*:

```console
./editConfig.sh create myconf useNodes=true
```

# config.txt

The file *config.txt* stores parameter definitions which seldomly change. 

You can use a guided edit of the **`config.txt`** file:

```console
./editConfig.sh 
```

**Parameter Description:**

* `conf=myconf` Name of the configuration. Also used to construct a directory name.
  **Do not change**

* `useNodes=bool`  *true* if devices are virtualized by nodes, *false* if devices are connected locally. If nodes are used:
  
  - `server=myserver.mydomain.de` Server Adress
  
  - `portBase=40` Prefix to construct the port number: port=portBase|portOffset, e.g. portBase=40, portOffset=013 -> port=40013
  
  - `useServerGit=false` Wether git should be used for keeping track of changing node tables. In case of damages, nodeDB.txt can be reconstructed from git. You have to manually create a repository

* `webPort=8080` Port which `lidarTool` uses for Web GUI

* `hubPort= #5000` Uncomment, if you want the observers to run on a separate instance of `lidarTool` (HUB) (not recommended)

* `adminPort=8000` Port which `lidarAdmin` uses for communication with nodes

* `recordPackedDir=dirname` Directory where to store packed tracking data. If empty no data will be recorded. If relative path, path is constructed by *$conf*/*dirname*/

* `blueprintImageFile` PNG image file with the floor plan image
  `blueprintExtent=Pixel=Meter`   
  `blueprintOcclusionFile` PNG image file with masks
  `blueprintSimulationFile` PNG image file with the floor plan image for simulation. Pixels which are non black are obstacles.
  See [BLUEPRINTS](BLUEPRINTS.md) for details.

To set one of the default obstacle Images for simulation of persons in the space, set *obstaclePersons=1* or *obstaclePersons=2*:

```console
./editConfig.sh obstaclePersons=1
```

# sensorDB (File sensorDB.txt)

The file `sensorDB.txt` interfaces between physical nodes identified by their MAC adress and their role in the lidar system. Node name are combinations of field values of an entry:

| Group Name | Lidar Model | Nr in Group | Port Offset | enabled (+/-) | Mac               |
| ---------- | ----------- | ----------- | ----------- | ------------- | ----------------- |
| *intr*     | *yd*        | *03*        | 013         | +             | bc:23:ab:cb:45:cd |

A node name is constructed as \$*GroupName*_\$*LidarModel\*$*NrInGroup*. The entry above would result in the node name: *intr_yd03*

Use `+` or `-` to enable or disable nodes, e.g. if their MAC Adresses are not known yet, or they do not exist in the network.

The command `manageSensors.sh` creates and updates all files related to the enabled nodes.

Once you have changed the file `sensorDB.txt`, you have to update the database:

### Update Sensor Entries in Simulation Mode:

```
./manageSensors.sh +s update
```

### Update Sensor Entries for Physical Nodes:

```
./manageSensors.sh update
```

## nodeDB (File nodeDB.txt)

The file `nodeDB.txt` stores information of physical nodes. It is automatically managed by `lidarAdmin`. Do not change it by hand.

When nodes register with MAC and IP adresses, they get their UDP port offset and lidar type.

| MAC               | IP           | Port Offset | Login User | Pi Model |
| ----------------- | ------------ | ----------- | ---------- | -------- |
| bc:23:ab:cb:45:cd | 192.168.1.23 | 013         | rock       | RockPiS  |

## Additional Files needed

- blueprints.json (change the configuration name in json)
- observer.json (change the configuration name in json)

# Remote Login

For lidaradmin being able to manage the client nodes, it must have the permissions to **remote login via ssh**.

You can either store the password of the remote clients in `sshPasswd.txt`or login the server and copy its ssh id with:

```console
ssh-copy-id user@client.example.com
```

# Add Cron jobs

Graphical User interface: [zeit](https://www.tecmint.com/zeit-gui-tool-to-cron-jobs-in-linux/)
