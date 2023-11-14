#!/bin/bash

[ -f config.txt ] && source config.txt

[ -f notification.sh ] && mv notification.sh $conf/.

