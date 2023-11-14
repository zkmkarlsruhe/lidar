#!/bin/bash

sudo add-apt-repository -y ppa:blaze/main
sudo apt-get -yq update
sudo apt-get -yq upgrade

sudo apt-get -yq install joe xemacs21 ssh hfsplus hfsutils dselect csh tcsh synaptic curl wget build-essential cmake automake libtool m4 git libglw1-mesa-dev libglu1-mesa-dev libglc-dev libglfw3-dev freeglut3-dev mesa-common-dev libx11-dev libxext-dev libxv-dev libxmu-dev libxi-dev xvfb libxrender-dev libxtst-dev libxcursor-dev libncurses-dev pkg-config gparted wmctrl metacity sawfish sawfish-data sawfish-themes sawfish-lisp-source net-tools libcurl4-openssl-dev ssh-askpass sshpass x11vnc nfs-kernel-server 

sudo apt-get -yq install build-essential git cmake automake libtool m4 libx11-dev libjpeg-dev libpng-dev libudev-dev wget libcurl4-openssl-dev curl uuid-dev pkg-config jq

sudo apt-get -yq install libmosquitto-dev libwebsockets-dev liblo-dev liblo-tools lua5.3 liblua5.3-dev luarocks lua-socket

sudo add-apt-repository -y ppa:blaze/main
sudo apt-get -yq update
sudo apt-get -yq install zeit

# set git user if needed

# git config --global user.email "linter@zkm.de"
# git config --global user.name "Bernd Lintermann"

# if git user:

 protocol=git@git.zkm.de:

# otherwise:
 
# protocol=https://git.zkm.de/

# install lidartool

cd
git clone $(protocol)Hertz-Lab/hci-lab/lidar/lidartool
cd lidartool
./install_script.sh

# install lidaradmin

cd
git clone $(protocol)Hertz-Lab/hci-lab/lidar/lidaradmin
cd lidaradmin
make

# create lidarconfig

export conf=mack

./createConfig.sh $conf

confDir="$HOME/lidarconfig-$conf"

if [ ! -f $HOME/.config/autostart/StartServer.sh.desktop ] || [ "$(grep $confDir $HOME/.config/autostart/StartServer.sh.desktop)" == "" ]; then
    mkdir -p $HOME/.config/autostart
    cp doc/ubuntu20.04-mate/*.desktop $HOME/.config/autostart/.
    sed -i "s#\$confDir#$confDir#" $HOME/.config/autostart/StartServer.sh.desktop
    sed -i "s#\$confDir#$confDir#" $HOME/.config/autostart/StartAdmin.sh.desktop

fi

