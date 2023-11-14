#!/bin/bash

if [ -d libhttpserver ] ; then
    echo "libhttpserver already exists"
else
    VERSION=$(dpkg-query --show --showformat '${Version}' libmicrohttpd-dev)

    git clone https://github.com/etr/libhttpserver.git

#    if $(dpkg --compare-versions $VERSION lt 0.9.64) ; then
	cd libhttpserver
    	git reset --hard c974604
	./bootstrap
    	cd ..
#    fi
fi

cd libhttpserver
mkdir -p build
cd build
../configure --disable-doc --disable-examples --disable-shared
#make $2 $3
cd ../..
