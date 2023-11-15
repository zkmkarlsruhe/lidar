# Lidar Toolset

The lidar toolset is intended to be used by artists for interactive applications or museums to gather information about visitor flow using laser range detection.

Copyright (c) 2022 ZKM | Karlsruhe.  
Copyright (c) 2022 Bernd Lintermann.

For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file "LICENSE" in this distribution.

BSD Simplified License.

# Description

The ***lidartool*** allows to track people in larger spaces by combining the input of several LiDAR scanners. Tracking data can be processed and distributed in several ways. Currently OSC, Websockets, MQTT, Influxdb, and the Lua scripting language are supported. It has a web GUI for operating it via a web browser.

In case of a larger setup with dozens of sensors distributed over several hundreds of square meters, sensors can be virtualized by attaching them to small computation units like Raspberry Pis. These so called *nodes* send their data via UDP in the local network to a main instance of lidartool.

The ***lidarnode*** virtualizes a physical LiDAR device. It usually uses a small computation unit like Raspberry Pi or similar with a locally connected LiDAR sensor. These units can easily be located in large spaces.

The ***lidaradmin*** allows to manage a large number of distributed lidar nodes.

Each configuration of LiDAR sensors in a space is called a **lidar configuration**. A lidar configuration is stored in a directory named **lidarconfig-*name*** which lives in the same directory as lidartool and lidaradmin.

This repository is structured the following way:

```
lidar/
└──lidartool/
└────conf/
└──lidaradmin/
└──lidarnode/
└──lidarconfig/
└────samples/
└──lidarconfig-.../
└────.../
```

- [**lidartool**](lidartool): implements collecting of LiDAR device data, locally or virtualized, and the detection and tracking of objects. It works as a standalone application if only one or a few sensors are used which can be connected to a single computer. The *conf* subdirectory contains configuration specific subdirectories

- [**lidaradmin**](lidaradmin): contains everything necessary to define and manage a large number of distributed LiDAR nodes. Makes use of *lidartool*.

- [**lidarnode**](lidarnode): contains files for defining a LiDAR client node. Makes use of *lidartool* and *lidaradmin*.

- [**lidarconfig**](lidarconfig): contains tools to a create and edit a lidar configuration and contains samples for use cases.

- **lidarconfig-...**: are created for each configuration (... is the configuration name) and contain configuration specific scripts and data

System Requirements
------------

System OS: Linux

Hardware and OS:
* Desktop PC, Ubuntu 20.04 Mate
* Rapberry 3 or 4 with raspios-bullseye
* [**Radxa Rock Pi S**](https://wiki.radxa.com/RockpiS) with  debian-buster

Required packages:
* g++, std=c++17, cmake, automake, libtool, m4, wget, curl, libcurl4-openssl-dev curl libudev-dev uuid-dev pkg-config jq bc

Optional:
* libmosquitto-dev for MQTT support
* libwebsockets-dev for websocket support
* liblo-dev, liblo-tools for OSC support
* lua5.3, liblua5.3-dev, luarocks for Lua support

# Installation

## Set up Single Computer Configuration

If you want to use just **one or a few  LiDAR sensors** attached to a single computer:

- cd to [**lidaradmin**](lidaradmin) directory and proceed with the instructions in the [README](lidaradmin/README.md) to build the executables

- cd to [**lidarconfig**](lidarconfig) directory and proceed with the instructions in the [README](lidarconfig/README.md) to set up your own configuration

## Set up a Client Server Configuration

In case of a **Large Area Configuration with several LiDAR sensors**, you have to work with a client/server architecture. One server collects the data of several client nodes. On each, the server and the clients, one instance of lidartool runs. 

Clients are configured to send their data to the server via UDP, the sensors are distinguished by UDP port number. Client nodes do not need much computation power, their task is to virtualize the sensor device, and can run on Raspberry Pi and/or Rock Pi S embedded computers. For operation, they just need to know the server address and UDP port to send data to. The MAC address of the Pis are used during setup for identification when they register on the server.

The server needs more computational power than the clients. An instance of lidartool collects the sensor data of all client nodes and does the calculation for object detection and tracking. In order to locate sensors on a map correctly, a pixel map (a drawing) of the tracked space is needed and the distance in meters in real space has to be known as exactly as possible for related pixels in the map. Ideally the distance of the two most distant walls are measured with a laser range finder in real space and the number of corresponding pixels is counted in the pixel map. In the planning phase, before the LiDARs are physically installed, LiDARs can be roughly placed on a map in a so-called simulation mode.

#### 1. Set up Client Node

For setting up the **client nodes**, clone this repository and:

- cd to [**lidarnode**](lidarnode) directory and proceed with the instructions in the [README](lidarnode/README.md)

#### 2. Set up Server

For setting up the **server**, clone this repository and:

- cd to [**lidaradmin**](lidaradmin) directory and proceed with the instructions in the [README](lidaradmin/README.md)

- cd to [**lidarconfig**](lidarconfig) directory and proceed with the instructions in the [README](lidarconfig/README.md) to set up your own configuration
  
  - set `useNode=true`
  - set `server=` to your server name or IP address
  - copy the `config.txt` file from the server lidarconfig-*name* directory to the client node directory `~/lidar/lidarnode`.

#### 3. Prepare a Simulation

- Define `blueprintImageFile`, `blueprintSimulationFile`, and `blueprintExtent` (see [BLUEPRINTS](lidarconfig/doc/BLUEPRINTS.md)).
- Define your sensors in the lidarAdmin UI in the web browser
- Place and rotate the sensors interactively in the WebUI in the web browser

#### 4. Set up each Physical Nodes

- Prepare client nodes with an SD card cloned from the SD card prepared in step 1. Identify the nodes MAC addresses.
- Place nodes in the real space and enter the MAC address in the sensor definition file.

# Examples

See [samples](lidarconfig/samples/README.md) for creating executable examples.

# The Intelligent Museum

An artistic-curatorial field of experimentation for deep learning and visitor participation.

The [ZKM | Center for Art and Media](https://zkm.de/en) and the [Deutsches Museum Nuremberg](https://www.deutsches-museum.de/en/nuernberg/information/) cooperate with the goal of implementing an AI-supported exhibition. Together with researchers and international artists, new AI-based works of art are realized during the years 2020-2023. They will be embedded in the AI-supported exhibition in both houses. The Project „The Intelligent Museum” is funded by the Digital Culture Programme of the [Kulturstiftung des Bundes](https://www.kulturstiftung-des-bundes.de/en) (German Federal Cultural Foundation) and funded by the [Beauftragte der Bundesregierung für Kultur und Medien](https://www.bundesregierung.de/breg-de/bundesregierung/staatsministerin-fuer-kultur-und-medien) (Federal Government Commissioner for Culture and the Media).

As part of the project, digital curating will be critically examined using various approaches of digital art. Experimenting with new digital aesthetics and forms of expression enables new museum experiences and thus new ways of museum communication and visitor participation. The museum is transformed to a place of experience and critical exchange.

![Logo](media/Logo_ZKM_DMN_KSB.png)

