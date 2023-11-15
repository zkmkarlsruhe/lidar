# lidarnode

This document describes the setup of a client node which virtualizes a physical LiDAR device. It usually uses a small embedded computer like Raspberry Pi, Rock Pi S, or similar with a locally connected LiDAR sensor. These units can easily be located in large spaces.

All client nodes differ only by a single config file, where the admin server address is stored. The role-specific data, e.g., the UDP port to send LiDAR data to, is retrieved when the client node registers on the server. During administration and when registering, clients are identified by their MAC address.

# Install Client Node

## Raspberry Pi and Rock Pi S

If you are using Raspberry Pi or ROck Pi S as the client node computation platform, the operating system and software has to be installed on a SD card. This SD card can be setup beforehand and then cloned for any additional nodes. First, follow the instructions to set up the operating system for a [Raspberry Pi](../lidartool/doc/RaspberryPi/README.md) or a [Rock Pi S](../lidartool/doc/RockPI_S/README.md).

## Install lidartool and lidarnode in One Go

Change directory to this directory and install:
```console
cd lidar/lidarnode
./install/install_script.sh
```

# Application Specific Configuration

Copy the `config.txt` from the admin server to `lidarnode` folder.

You can now clone the SD card.

## Register Nodes

When lidarAdmin is running on the remote server, the client node registers after booting. The node should then appear in the `Maintain`- tab in the lidarAdmin user interface.

You can manually register the node by:

```console
cd lidar/lidarnode
./registerNode.sh +v +once
```
