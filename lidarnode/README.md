# lidarnode

This document describes the setup of a client node. A client node virtualizes a physical LiDAR device. It usually uses a small computation unit like Raspberry Pi or similar with a locally connected LiDAR sensor. These units can easily be located in large spaces.

All client nodes differ only in a single config file, where the admin server adress is stored. The role specific data, e.g., the UDP port to send LiDAR data to, is retrieved when the client node registers to the server. During administration and when registering, clients are identified by their MAC adress.

# Install Client Node

## Raspberry Pi and RockPi S

If you are using Pis as client node computation platform, the operating system and software has to be installed on a SDCard. This SD Card can be cloned for any additional nodes. Follow first the instructions to setup the operation system for a [RaspberryPi](../lidartool/doc/RaspberryPi/README.md) or a [RockPiS](../lidartool/doc/RockPI_S/README.md).

## Install lidartool and lidarnode in one go

cd to this directory and install:
```console
cd lidar/lidarnode
./install/install_script.sh
```

# Application Specific Configuration

Copy the `config.txt` from the admin server to `lidarnode` folder.

You can clone now the SD Card.

## Register Nodes

When lidarAdmin is running on the remote server, the client node registers after booting. The node should then appear in the `Maintain`- tab in the lidarAdmin user interface

You can manually register the node by:

```console
cd lidar/lidarnode
./registerNode.sh +v +once
```
