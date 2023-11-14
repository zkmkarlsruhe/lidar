#!/bin/bash

[ -f config.txt ] && source config.txt

./editConfig.sh createRegion myRegion @width=5 @height=4

[ -f MyScript.sh ] && mv MyScript.sh $conf/.

