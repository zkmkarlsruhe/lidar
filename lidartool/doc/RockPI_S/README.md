# RockPI S Setup

This document describes the setup of a [**Radxa RockPi S**](https://wiki.radxa.com/RockpiS) operation system for lidartool.

The operating system and software is installed once on an SDCard, which is cloned for any additional nodes. Server adresses are stored in a single config file. The UDP adress is retrieved by the server when the client registers to the server. Clients are identified by their MAC adress.

### Installation

First a standard SD Card is prepared for the RockPi S with a debian linux and then the application specific software. 

## Install Debian Image on SD Card:

Download the RockPi S Debian Image: [rockpis_debian_buster_server_arm64_20210924_0412-gpt.img.gz](https://github.com/radxa/rock-pi-s-images-released/releases/download/rock-pi-s-v20210924/rockpis_debian_buster_server_arm64_20210924_0412-gpt.img.gz) and install it on the SDCard as described in the RockPi S [Getting Started](https://wiki.radxa.com/RockpiS/getting_started) documentation.

Insert the bootable SDCard and boot the RockPi S.

Figure out the IP Adress (for example connect it to a router and look at the router network information) and login via ssh as user`rock`and passwd`rock`.

Set your favoured password.

## Install Packages

```console
export DEBIAN_FRONTEND=noninteractive

sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9B98116C9AA302C7

sudo apt-get update
sudo apt-get upgrade

sudo apt-get -yq install nano build-essential git cmake automake libtool m4
sudo apt-get -yq install libmicrohttpd-dev
sudo apt-get -yq install psmisc
sudo apt-get -yq install dphys-swapfile
```

Accept defaults.

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

## Add default user to group dialout

```console
sudo usermod -a -G dialout `id -un`
```

## Clone Lidar Repository

```console
cd 
git clone ...
```

## Enable UART and PWM Overlays

Move `rk3308-uart3.dtbo` to the overlay directory:

```console
sudo cp ~/lidar/lidartool/doc/RockPI_S/rk3308-uart3.dtbo /boot/dtbs/4.4.143-65-rockchip-g58431d38f8f3/rockchip/overlay/.
```

Edit file `/boot/uEnv.txt`

```console
sudo nano /boot/uEnv.txt
```

Change the following lines:

```console
console=
overlays=rk3308-uart0 rk3308-uart1 rk3308-uart2 rk3308-uart3 rk3308-pwm2 rk3308-pwm3
```

## reboot

```
sudo reboot
```

Login again

# Install Software

Set computer model for lidartool:

```console
sudo sh -c "echo RockPiS > /etc/Model"
```

## Install lidartool and lidarnode in one go

```console
cd ~/lidar/lidarnode
./install/install_script.sh
```

# Application specific Configuration

Copy the `config.txt` from the admin server to `lidarnode` folder.

## Register Nodes

For each client RockPi S, clone this SD Card and reboot. When lidaradmin is running on the server, the client node registers when booting. This node should now appear in the `Maintain`- tab in the admin user interface

You can manually register the node by:

```console
cd ~/lidar/lidarnode
./registerNode.sh +v +once
```

# Hardware Configuration:

We are using the RockPi S [Hardware Revision V13 PIN Layout](https://wiki.radxa.com/RockpiS/hardware/gpio).

26-pin Header 1

Pin# 04: +5V  
Pin# 25: GND  
PIN# 23: UART1_RX  
Pin# 11: PWM2  
Pin# 12: GPIO 69 -> Transistor
