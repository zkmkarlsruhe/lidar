#!/bin/bash
#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//

nodedir=$(pwd)

#install dependencies

cd ../lidartool
./install/install_script.sh

cd $nodedir

./install_node.sh
sudo ./install_node.sh install

