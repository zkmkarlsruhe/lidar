# Raspberry Pi 3 or 4 Setup

This document describes the setup of a Raspberry Pi 3 or 4 as client node.

The operating system and software is installed once on an SDCard, which is cloned for any additional nodes. Server adresses are stored in a single config file. The UDP adress is retrieved by the server when the client registers to the server. Clients are identified by their MAC adress. See [OkDo Getting Started](https://www.okdo.com/getting-started/get-started-with-lidar-hat-for-raspberry-pi/) for details on how to do the basic steps for installing an image on SD Card and how to connect an LD06 compatible LiDAR sensor to the Raspberry Pi.

## Install Raspbian

Install [2023-05-03-raspios-bullseye](https://downloads.raspberrypi.org/raspios_armhf/images/raspios_armhf-2023-05-03/2023-05-03-raspios-bullseye-armhf.img.xz) on an SD Card. Set language to english.

### Enable Serial Device and SSH

Enable the Raspberry Pi’s serial port and disable serial console by graphical menu (Desktop Main Menu -> Preferences -> Raspberry Pi Configuration) or command line interface `raspi-config`:

- Select **Interfaces**
- **Enable SSH**
- **Enable Serial Port**
- **Disable Serial Console**

### Install Packages

Login to the Rapberry Pi

```console
sudo apt-get update
sudo apt-get upgrade

sudo apt-get -yq install nano build-essential git cmake automake libtool m4 bc dnsutils net-tools
sudo apt-get -yq install libmicrohttpd-dev
sudo apt-get -yq install psmisc
sudo apt-get -yq install dphys-swapfile
```

## Set timezone

```console
sudo timedatectl set-timezone "Europe/Berlin"
```

## Increase swap space

For compiling, more than the default swap space is needed. Increase swap space by editing file `etc/dphys-swapfile`:

```console
sudo nano /etc/dphys-swapfile
```

set

`CONF_SWAPSIZE=2048`

## reboot

```
sudo reboot
```

## Install lidartool and lidarnode in one go

```console
cd ~/lidar/lidarnode
./install/install_script.sh
```

# Application specific Configuration

Copy the `config.txt` from the admin server to `lidarnode` folder.

## Register Nodes

For each client Raspberry Pi S, clone this SD Card and reboot. When lidaradmin is running on the server, the client node registers when booting. This node should now appear in the `Maintain`- tab in the admin user interface

You can manually register the node by:

```console
cd ~/lidar/lidarnode
./registerNode.sh +v +once
```

