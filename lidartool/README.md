# lidartool

The lidartool is intended to be used by artists for interactive applications or museums to gather information about visitors flow.

The lidartool allows to track people in larger spaces by combining the input of several lidar scanners. Tracking data can be processed and distributed in several ways. Currently OSC, Websockets, MQTT, Influxdb and the Lua scripting language are supported.  It runs on Linux and has a web GUI for operating it via a webbrowser.

Copyright (c) 2023 ZKM | Karlsruhe.  
Copyright (c) 2023 Bernd Lintermann.

For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file "LICENSE" in this distribution.

BSD Simplified License.

Description
-----------

The lidar tool provides a command line tool to read several types of LiDAR sensors and provides additional functionality:

* combining an arbitrary number of sensors
* virtualization of sensors via network / UDP
* eliminating environment samples of e.g. walls, when the lidar sensor is installed in a static environment
* detection of objects (humans) from a group of samples
* merging detected objects from different sensors in a shared space and assigning a tracking id
* output of tracked objects as Json or OSC via TCP/UDP
* logging to files, via MQTT or as heatmap images
* processing object data via lua scripts or calling bash scripts at certain events 

An integrated web server provides a GUI via web browser and provides a REST API.

See [doc/scenarios](doc/scenarios/README.md) for use cases.  
See [doc/gui](doc/gui/README.md) for description of Web GUI.  
See [doc/regions](doc/regions/README.md) for region definition.
See [doc/observers](doc/observers/README.md) for description of observers.  

Dependencies

* g++,  std=c++17
* cmake, automake, wget, libcurl
* libudev-dev for UART/USB serial number support
* libmosquitto-dev for MQTT support
* libwebsockets-dev for websocket support
* liblo-dev for OSC support
* lua5.3 liblua5.3-dev luarocks for Lua support

# ToC

- [Installation & Build](#installation---build)
- * [1. Clone Repository](#1-clone-repository)
  * [2. Install and Build software](#2-install-and-build-software)
- [Quick Start](#quick-start)
  * [Graphical User Interface](#graphical-user-interface)
  * [Usage](#usage)
- [Devices](#devices)
  * [Physical Devices on Serial Port](#physical-devices-on-serial-port)
  - [Virtual Devices](#virtual-devices)
  - [Device Name Management](#device-name-management)
- [Tracking](#tracking)
  + [Enable Tracking](#enable-tracking)
  + [Restrict Tracking to a given Frame Rate](#restrict-tracking-to-a-given-frame-rate)
  + [Tracking Parameters](#tracking-parameters)
- [Regions](#regions)
- [Observers](#observers)
- [Examples](#examples)
- [Supported Devices](#supported-devices)

<small><i><a href='http://ecotrust-canada.github.io/markdown-toc/'>Table of contents generated with markdown-toc</a></i></small>

# Installation & Build

If you want to install on a **Raspberry 3 or 4**, follow these [instructions](doc/RaspberryPi/README.md).

If you want to install on a **Radxa RockPi S**, follow these [instructions](doc/RockPI_S/README.md).

## 1. Clone Repository

clone this repository

```
git clone https://github.com/zkmkarlsruhe/lidar.git
```

cd to path of cloned repository:

```console
cd lidar/lidartool
```

## 2. Install and Build software

```console
./install/install_script.sh
```

Type in root password when needed.

If you need support for any of MQTT, Lua Scripting Language, OSC or Websockets, append them selectively as options:

```console
./install/install_script.sh mqtt lua osc websockets
```

Use `all` as option for installing all packages:

```console
./install/install_script.sh all
```

# Quick Start

Connect your LiDAR sensor to the computer. Wait 20sec in order to let the system autodetect your lidar.

Run lidarTool: 

```console
./lidarTool +v +d 0
```

This tries to autodetect the lidar sensor connected to serial device `/dev/ttyUSB0` `/dev/ttyACM0`or `/dev/ttyS0`.

## Graphical User Interface

In a web browser open the URL `http://localhost:8080`

See [doc/gui](doc/gui/README.md) for description of Web GUI.

## Usage

```console
./lidarTool [-h|-help] [options...]
```

#### Help

For detailed help on a topic: 

```console
./lidarTool help topic
```

Where *topic* is one out of 

```console
general       general usage information
devices       sensor device usage
niknames      nik name usage
groups        groups of sensors usage
processing    sensor data processing parameter
tracking      tracking arguments
regions       regions  usage
observer      observer usage
blueprint     blueprint usage
simulation    sensor simulation usage
all           all helps
```

# Devices

## Physical Devices on Serial Port

Devices connected to the serial port are opend via `+d [deviceType:]deviceName`. The *deviceName* is */dev/ttyUSB*X, */dev/ttyACM*X or */dev/ttyS*X. For convenience the */dev* and even */dev/ttyUSB*, */dev/ttyACM* or */dev/ttyS* can be omitted. The device type is a keyword which selects the lidar software driver and model specific parameters. If no device type is given, the lidar device type is automatically detected.

```console
# these are equivalent:
> ./lidarTool +d /dev/ttyUSB0
> ./lidarTool +d ttyUSB0
> ./lidarTool +d 0

# RPLidar A1M8 connected to /dev/ttyUSB0 and LDRobot LD06/LD19 connected to /dev/ttyUSB1
> ./lidarTool +d a1m8:0 +d ld06:1
```

See section [Device Name Management](#device-name-management) for concenience handling of device names and with assigning devices by serial number to device names.

### Supported device types

| Manufacturer | Model            | Device Type      | Tested |
|:------------ |:---------------- |:---------------- |:------ |
| Slamtec      | A1M8, A3M1       | a1m8, a3m1       | yes    |
| Slamtec      | A2M7, A2M8       | a2m7, a2m8       | no     |
| Slamtec      | autodetect       | slamtec, rplidar |        |
| YDLidar      | Tmini pro        | tmini            | yes    |
| YDLidar      | G1, G2, G2A, G2C | g1, g2, g2a, g2c | no     |
| YDLidar      | G4               | g4               | yes    |
| YDLidar      | G4PRO            | g4pro            | no     |
| YDLidar      | X2, X2L, X4      | x2, x2l, x4      | no     |
| YDLidar      | S2, S4, S4B      | s2, s4, s4b      | no     |
| YDLidar      | F4, F4PRO, R2    | f4, f4pro, r2    | no     |
| YDLidar      | TX8, TX20        | tx8, tx20        | no     |
| YDLidar      | TG15, TG30, TG50 | tg15, tg30, tg50 | yes    |
| YDLidar      | autodetect       | ydlidar          |        |
| LDRobot/Okdo | LD06/LD19,STL27L | ld06,ld19,st27   | yes    |
| LDRobot/Okdo | autodetect       | ldrobot          |        |
| LSLidar      | N10, M10         | n10, m10         | yes    |
| LSLidar      | autodetect       | lslidar          |        |
| ORadar       | MS200            | ms200            | yes    |
| ORadar       | autodetect       | mslidar          |        |

For sensor details see section [Supported Devices](#supported-devices)

## Virtual Devices

Virtual devices are implemented by sending UDP messages between two applications

One application binds to a local UDP port and listens for another application to connect. Each of the communication partners can play the role of the client or server. 

#### Application with physical sensor acts as Server:

```console
# Source application opens the serial device 0 and listens on port 8090 for clients 
myRaspi> ./lidarTool +d 0 +virtual 8090

# Drain application connects to the server for reading scan data
hostname> ./lidarTool +d virtual:myRaspi.domain.de:8090
```

#### Application with physical sensor acts as Client:

```console
# Source application opens the serial device 0 and sends scan data to the server
hostname> ./lidarTool +d 0 +virtual myServer.domain.de:8090

# Drain application listens on port 8090 for clients to read scan data 
myServer> ./lidarTool +d virtual:8090
```

# Device Name Management

Device name management simplifies the handling of device names. On the one hand, it allows physical devices to be associated with a persistent device name via serial number; on the other hand, nik names facilitate the readability of device names.

See [doc/devicenames](doc/devicenames/README.md) for description of Device Name Management.

# Tracking

Tracked objects, e.g. humans, get assigned a simple id and a uuid which is garanteed to never being used twice. Once detected, objects are tracked over time and their data can be processed in several ways, e.g. send to a host via OSC or as JSON formatted messages via websockets. Objects are only detected if tracking is enabled.

### Enable Tracking

Enable tracking of objects with option `+track`.

### Restrict Tracking to a given Frame Rate

Tracking operates with a given frame rate independend of the frame rates of the connected devices. If a sensor deliveres a lower frame rate nthen the system frame rate, the tracking data will be smoothed. The default system frame rate is 60Hz.

```console
# restricts the frame rate to 30Hz (frames per second)
> ./lidarTool +fps 30 +d 0 +track
```

### Tracking Parameters

Tracking parameters influence the tracking algorithm. Type `./lidarTool help tracking` for details.

# Regions

Regions are rectangular or elliptical areas on which specific processing of objects can happen. Regions are managed via the command line and the Web UI. Regions can be created, deleted and parameters assigned. Their geometry can be also be edited via the graphical web interface.

By default region usage is turned off. To make regions active, use option `+useRegions` when running lidarTool

Region definitions are stored in a json file in `conf/regions.json`.

See [doc/regions](doc/regions/README.md) for details.

# Observers

A universal way of processing and sending data via different communication channels - like file logging, OSC, MQTT, bash / Lua scripting or websockets - are **Observers**. Multiple observers allow for processing data in parallel.

See [doc/observers](doc/observers/README.md) for a detailed description of observers.

# Examples

See [../lidarconfig/samples](../lidarconfig/samples/README.md) for creating executable examples.
See [doc/scenarios](doc/scenarios/README.md) for use cases.

# Appendix

## Supported Devices

| Manufact                                                                                  | Model                                                                                                   | DevType | Rnge    | SmpFq    | ScnFq  | Res    | LasPow | WavLen | Notes                                                                    |
|:----------------------------------------------------------------------------------------- |:------------------------------------------------------------------------------------------------------- |:------- |:------- |:-------- |:------ |:------ |:------ |:------ |:------------------------------------------------------------------------ |
| [Slamtec](https://www.slamtec.com/en)                                                     | [A1M8](https://www.waveshare.com/wiki/RPLIDAR_A1)                                      | a1m8    | 12 m    | 8 kHz    | 5.5 Hz | 1°     | 3 mW   | 785 nm | Temperature: 0/40°C                                                      |
| [Slamtec](https://www.slamtec.com/en)                                                                                   | [A2M8](https://download.slamtec.com/api/download/rplidar-a2m8-datasheet/2.6?lang=en)                                      | a2m8    | 12 m    | 8 kHz    | 10 Hz  | 0.9°   | 3 mW   | 785 nm | Temperature: 0/40°C                                                      |
| [Slamtec](https://www.slamtec.com/en)                                                                                   | [A2M7](https://download.slamtec.com/api/download/rplidar-a2m7-datasheet/1.3?lang=en)                                      | a2m7    | 10-16 m | 16 kHz   | 10 Hz  | 0.225° | 10 mW  | 785 nm | Temperature: 0/40°C                                                      |
| [Slamtec](https://www.slamtec.com/en)                                                                                   | [A3M1](https://download.slamtec.com/api/download/rplidar-a3m1-datasheet/1.9?lang=en)                                      | a3m1    | 10-25 m | 16 kHz   | 10 Hz  | 0.225° | 10 mW  | 785 nm | Temperature: 0/40°C                                                      |
| [YDLidar](https://www.ydlidar.com/lidars.html)                                            | Most devices with [Serial Interface](https://github.com/YDLIDAR/YDLidar-SDK/blob/master/doc/Dataset.md) | ydlidar | *       | *        | *      | *      | *      | *      | *                                                                        |
| [LDRobot](https://www.ldrobot.com/product/list/en/8)                                      | [LD06/ LD19](https://www.inno-maker.com/wp-content/uploads/2020/11/LDROBOT_LD06_Datasheet.pdf)                                                      | ld06    | 12 m    | 4.5 kHz  | 10 Hz  | 1°     | 25 mW  | 905 nm | Waterproof and dustproof-IPX4, Temperature: -10/40°C, 30000 lux, 10000 h |
| [LDRobot](https://www.ldrobot.com/product/list/en/8)                                      | [STL27L](https://github.com/May-DFRobot/DFRobot/blob/master/SEN0589_Datasheet.pdf)                                               | st27    | 25m     | 21.6 kHz | 10 Hz  | 0.167° | -      | 905 nm | Waterproof and dustproof-IPX5, Temperature: -10/50°C, 60000 lux, 10000 h |
| [LSLidar](https://www.leishenlidar.com/de/produkt/m10-low-cost-to-lidar-scanner-outdoor/) | [M10](https://www.leishenlidar.com/wp-content/uploads/2022/02/M10.pdf)                                                                                | m10     | 12 m    | 10 kHz   | 10 Hz  | 0.36°  | -      | 905 nm | Temperature: -20/60°C                                                    |
| [LSLidar](https://www.leishenlidar.com)                                                   | [N10](https://gehtmarketplace.com/public/uploads/listings/3666d10108779b2fdd514ca0b3095182/1.%20LSLiDAR_N10%20PLUS_User_Manual_v2.0.2_20221212.pdf)                                                                                | n10     | 12 m    | 4.5 kHz  | 10 Hz  | 0.8°   | -      | 905 nm | Temperature: -10/40°C, 30000 lux                                         |
| [ORADAR](http://en.oradar.com.cn/)                                                        | [MS200](https://www.orbbec.com/wp-content/uploads/2023/07/MS200-dToF-Lidar-user-manual-A0-20230605-1.pdf)                                                           | ms200   | 12 m    | 4.5 kHz  | 10 Hz  | 0.8°   | -      | 940 nm | Waterproof and dustproof-IPX4, Temperature: -10/40°C, 30000 lux, 10000 h |
| For supported YDLidar devices see [Supported Device Types](#supported-device-types)       |                                                                                                         |         |         |          |        |        |        |        |                                                                          |
