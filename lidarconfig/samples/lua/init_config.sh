#!/bin/bash

[ -f config.txt ] && source config.txt

./editConfig.sh createRegion myRegion @width=5 @height=4

[ -f udp.lua ] && mv udp.lua $conf/.
[ -f osc.lua ] && mv osc.lua $conf/.

