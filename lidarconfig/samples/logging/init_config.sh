#!/bin/bash

[ -f config.txt ] && source config.txt

./editConfig.sh createRegion region1 @x=-2 @width=2 @height=4
./editConfig.sh createRegion region2 @x=2  @width=2 @height=4

# create a region which covers the complete area
./editConfig.sh createRegion fullmap @x=0  @width=3 @height=4

