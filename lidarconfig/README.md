# Lidar Config

A lidar configuration is stored in a directory named **lidarconfig-*name*** which lives in the same directory-level as lidartool and lidaradmin.

This directory lidarconfig-*name* contains the config subdirectory ***name***, which stores several automatically generated files and additional files like the floor plan images used in the Web UI.

## Create a Configuration

To create a new lidar configuration directory `lidar/lidarconfig-myconf` with default files:

```console
cd lidar/lidarconfig
./editConfig.sh create myconf
```

Change to the newly created configuration directory:

```console
cd ../lidarconfig-myconf
```

### Check out Samples

To create a new lidar configuration directory `lidar/lidarconfig-myconf` and initialize it with one of the samples use `editConfig.sh` with option *create path_to_sample/conf*.

For example: to check out the OSC sample, type:

```console
cd lidar/lidarconfig
./editConfig.sh create samples/osc
```

Change to the newly created configuration directory:

```console
cd ../lidarconfig-osc
```

See [samples/README](samples/README.md) for details.

### Start the Lidar Server

```console
./StartServer.sh +v
```

In a web browser, open page http://localhost:8080

### Start the Lidar Admin Server:

```console
./StartAdmin.sh +v
```

In a web browser, open page [http://localhost:8000](http://localhost:8000)

### Stop the Lidar Admin Server:

The Lidar Admin Server can be manually stopped by:

```console
./StopAdmin.sh
```

### Stop and Restart the Lidar Server

The Lidar Server is usually started and stopped out of the web interface of the Lidar Admin Server. In case it has to be manually started (e.g. at boot time) or stopped, you can:

```console
./StartServer.sh
./StopServer.sh
```

## Edit a Configuration

The file **`config.txt`** contains several settings as variable definitions.

You can use a guided edit of the **`config.txt`** file in your config directory:

```console
./editConfig.sh
```

See [CONFIG](doc/CONFIG.md) for details.

This file is also copied to the client nodes, which use the server address for registering. You have to set the `server` address on which the admin server runs before copying.

## Observers

Put your observer definitions in a text file `observer.txt` in the configuration directory. The script `StartServer.sh` sources the files `observer.txt` and `conf/observer.txt` if they exist.

## Using Floor Plans

If you want to use a Floor Plan in the background of the Web UI, you need a pixel image of the floor plan as PNG image. A second floor plan image is used to simulate sensors during the planning phase.

See [BLUEPRINTS](doc/BLUEPRINTS.md) for more information.

# Define Client Nodes

Client nodes virtualize physical sensors in order to place them in a larger space. The Lidar Admin communicates with these client nodes during setup. The nodes (virtual sensors) which the system should manage, are defined in the sensorDB.txt.

### sensorDB.txt

The file `sensorDB.txt` interfaces between physical or simulated client nodes and their appearance in the lidar system. Client nodes are identified by their MAC address.

Each node is defined by a single text line in `sensorDB.txt`. Node names are combinations of field values in the text line:

| Group Name | Lidar Model | Nr in Group | Port Offset | enabled (+/-) | Mac Address       |
| ---------- | ----------- | ----------- | ----------- | ------------- | ----------------- |
| *intr*     | *ld*        | *01*        | 001         | +             | b8:27:eb:55:df:59 |

A sensor entry could look like that:

`intr    ld  01    001      +  b8:27:eb:55:df:59`

The node name is constructed as `$GroupName_$LidarModel$NrInGroup`. The entry above would result in the node name *intr_ld01* and would use UDP port (40)001 (while 40 is defined as *portBase* in `config.txt`).

If a client node with MAC address b8:27:eb:55:df:59 registers to Lidar Admin, the port number and lidar device type to use is returned.

Available Sensor Types (defined in `modelMap.txt`):

| Lidar Model | Device Type      |
| ----------- | ---------------- |
| ld          | ld06, ld19       |
| st          | st27             |
| ms          | ms200            |
| tg          | tg50, tg30, tg15 |
| tm          | tmini            |
| g4          | g4               |
| a1          | a1m8             |
| a3          | a3m1             |

Use `+` or `-` to enable or disable nodes, e.g. if their MAC addresses are not known yet or if they should not be used by the system.

Once you have changed the file `sensorDB.txt`, you have to update the database.

### Update Sensor Entries

When the`sensorDB.txt` is modified, the button *Apply Config* in the LidarAdmin UI changes color to red. You can update all fies and restart the server by pressing this button.

In case you want to do it manually, the command `manageSensors.sh` creates and updates all files related to the enabled nodes (*groups.json*, *nikNames.json* or *nikNamesSimulationMode.json*):

For updating **Simulation Mode** related files:

```
./manageSensors.sh +s update
```

For updating **Physical Nodes** related files:

```
./manageSensors.sh update
```

# Client Node Remote Login

In order for lidarAdmin to manage the client nodes, it must have permissions to **remote login via ssh**.

You can either store the password of the remote clients in the config directory in the file `sshPasswd.txt`or log into to the server and copy its ssh id with:

```console
ssh-copy-id user@client.example.com
```

See [lidarnode](../lidarnode) how to set up a client node.

# Files

The configuration directory contains application-specific files:

- **config.txt** contains configuration parameter. See [CONFIG](doc/CONFIG.md) for details. Please edit if needed.

- **StartServer.sh** script for starting the Lidar Server. Please edit if needed.

- **LidarRunMode.txt** contains the current run mode (simulation, setup, production). Modified by Lidar Admin. Do not edit.

- **nodeDB.txt** contains information about physical nodes. Automatically generated. Do not edit.

- **modelMap.txt** contains the mapping of Lidar Model shortcuts to device types. Please edit if needed.

The configuration subdirectory ***name*** contains application specific files:

- **blueprints.json** references the floor plan image for simulation and production mode and an image with colored layers of the floor plan. See [BLUEPRINTS](doc/BLUEPRINTS.md) for more information. Modified by editConfig.sh. Please edit if needed.

- **groups.json** contains sensor grouping information. Automatically generated. Do not edit.

- **nikNames.json** and **nikNamesSimulationMode.json** contain sensor name alias information. Automatically generated. Do not edit.

- **observer.txt** contains observer definitions as bash commands, it is sourced on startup if it exists

- **observer.json** contains observer definitions in json format, it is used on startup if it exists

- **error.log** logs error messages

- **run.log** logs starting/stopping of Lidar Server and starting/stopping of sensors

For each sensor the following files are created in the configuration subdirectory ***name***:

- **LidarEnv_*devicenikname*.txt**, the measured ambient physical environment. Automatically generated when saving an environment scan. Do not edit

- **LidarEnv_*devicenikname*_Simulation.txt**, the ambient environment in simulation mode. Automatically generated when saving an environment scan in simulation mode. Do not edit

- **LidarMatrix_*devicenikname*.txt**, translation and rotation of sensor in space. Automatically generated when saving the registration. Do not edit

### nodeDB.txt

The file `nodeDB.txt` stores information of physical nodes. It is automatically managed by `lidarAdmin`. Do not change it by hand.

When nodes register with MAC and IP addresses, they get their UDP port offset and lidar type as return values.

| MAC               | IP           | Current Port Offset | Login User | Pi Model |
| ----------------- | ------------ | ------------------- | ---------- | -------- |
| b8:27:eb:56:c5:f6 | 192.168.1.23 | 013                 | rock       | RockPiS  |
