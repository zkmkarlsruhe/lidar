#// Copyright (c) 2023 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//


#===========================================================================
# CIMG
#===========================================================================

CIMGDIR      =  Trackable/CImg

#===========================================================================
# RPLIDAR
#===========================================================================

RPLIDARDIR   =  rplidar_sdk
RPLIDARINC   = 	-I$(RPLIDARDIR)/src \
	 	-I$(RPLIDARDIR)/include \
		-I$(RPLIDARDIR)/sdk/src \
		-I$(RPLIDARDIR)/sdk/include

RPLIDARLIB   =  $(RPLIDARDIR)/output/Linux/Release/libsl_lidar_sdk.a

#===========================================================================
# YDLIDAR
#===========================================================================

YDLIDARDIR   =  YDLidar-SDK
YDLIDARINC   = 	-I$(YDLIDARDIR) $(DYDLIDARDRIVER)
YDLIDARLIB   =  $(YDLIDARDIR)/build/libydlidar_sdk.a

#===========================================================================
# LDLIDAR
#===========================================================================

LDLIDARDIR    = ldlidar_stl_sdk_v3.0.5
LDLIDARINCDIR =	$(LDLIDARDIR)/include
LDLIDARSRC    =	$(LDLIDARDIR)/src

LDLIDARINC    =	-I$(LDLIDARINCDIR)/core \
                -I$(LDLIDARINCDIR)/dataprocess \
                -I$(LDLIDARINCDIR)/filter \
                -I$(LDLIDARINCDIR)/logger \
                -I$(LDLIDARINCDIR)/networkcom \
                -I$(LDLIDARINCDIR)/serialcom

LDLIDARCPPFILES = $(shell find $(LDLIDARSRC) -type f -name "*.cpp")
LDLIDAROFILES  := $(LDLIDARCPPFILES:.cpp=.o)

LDLIDARLIB    =  $(LIBDIR)ldlidar_stl_sdk.a

#===========================================================================
# MSLIDAR
#===========================================================================

MSLIDARDIR   =  oradar_sdk
MSLIDARINC   = 	-I$(MSLIDARDIR) -I$(MSLIDARDIR)/src
MSLIDARLIB   =  $(MSLIDARDIR)/build/liboradar_sdk.a

#===========================================================================
# HTTPSERVER
#===========================================================================

LIBHTTPSERVERDIR = libhttpserver

LIBMICROHTTPD    = libmicrohttpd
HTTPSERVERINC    = -I$(LIBHTTPSERVERDIR)/src -I$(LIBHTTPSERVERDIR)/include/
LIBHTTPSERVERLIB = $(LIBHTTPSERVERDIR)/build/src/.libs/libhttpserver.a
HTTPSERVERLIB    = $(LIBHTTPSERVERLIB)

ifeq ($(shell pkg-config --exists $(LIBMICROHTTPD) && echo -n true),true)
	HTTPSERVERLIB += -lmicrohttpd
else
	HTTPSERVERLIB += $(LIBHTTPSERVERDIR)/build/src/.libs/libhttpserver.a $(LIBHTTPSERVERDIR)/../libmicrohttpd/lib/libmicrohttpd.a
	HTTPSERVERINC += -I$(LIBHTTPSERVERDIR)/../libmicrohttpd/include/
endif

#ifeq ($(shell grep render_GET $(LIBHTTPSERVERDIR)/src/httpserver/http_resource.hpp | grep 'virtual const' | wc -l),1)
#	HTTPSERVERINC += -Drender_const=const
#else
#	HTTPSERVERINC += -Drender_const=
#endif

HTTPSERVERINC += -Drender_const=const

#===========================================================================
# RAPIDJSON
#===========================================================================

RAPIDJSONINC = -ITrackable/rapidjson/include/

#===========================================================================
# OBJDIR
#===========================================================================

OBJDIR	     = obj/
LIBDIR       = lib/

#===========================================================================
# LIDARKITOFILES
#===========================================================================

LIDARKITOFILES = $(OBJDIR)lidarKit.o \
		 $(OBJDIR)lidarVirtDriver.o \
		 $(OBJDIR)ldlidarDriver.o \
		 $(OBJDIR)ydlidarDriver.o \
		 $(OBJDIR)lslidarDriver.o \
		 $(OBJDIR)mslidarDriver.o 

#===========================================================================
# APPLICATION
#===========================================================================

#USE_MARKER	   = -DUSE_MARKER=1
#USE_CAMERA	   = -DUSE_CAMERA=1

USE_CIMG	  = -DUSE_CIMG=1
USE_INFLUXDB 	  = -DUSE_INFLUXDB=1

ifeq ($(shell pkg-config --exists liblo && echo -n true),true)
	USE_LIBLO	  = -DUSE_LIBLO=1
	LIBLOLIB	  = -llo
endif

ifeq ($(shell pkg-config --exists libmosquitto && echo -n true),true)
	USE_MOSQUITTO	  = -DUSE_MOSQUITTO=1
	MOSQUITTOLIB	  = -lmosquitto
endif

ifeq ($(shell pkg-config --atleast-version=3 libwebsockets && echo -n true),true)
	USE_WEBSOCKETS 	  = -DUSE_WEBSOCKETS=1 -DLWS_NO_EXTENSIONS
	CPPWEBSOCKETSLIB  = -lwebsockets
	LIDARKITOFILES	 += $(OBJDIR)trackableHUB.o 
endif

LUA_VERSION    = lua-5.3
ifeq ($(shell pkg-config --exists $(LUA_VERSION) && echo -n true),true)
	USE_LUA	  	= -DUSE_LUA=1
	LUA_CPPFLAGS	= $(shell pkg-config --cflags $(LUA_VERSION))
	LUALIB	  	= $(shell pkg-config --libs $(LUA_VERSION))
endif

ifeq ($(shell pkg-config --exists libudev && echo -n true),true)
	USE_LIBUDEV	  = -DUSE_LIBUDEV=1
	LIBUDEVLIB	  = -ludev
endif

#===========================================================================
# CC
#===========================================================================

CC 	     = g++

ifeq ($(shell test -x /usr/bin/g++-8 && echo -n true),true)
	CC   = g++-8
endif
ifeq ($(shell test -x /usr/bin/g++-9 && echo -n true),true)
	CC   = g++-9
endif

CC_FLAGS     += -g  -std=c++17 -Wno-psabi
CC_FLAGS     += $(USE_MARKER) $(USE_CAMERA) $(USE_CIMG) $(USE_LIBLO) $(USE_MOSQUITTO) $(USE_WEBSOCKETS) $(USE_INFLUXDB) $(USE_LUA) $(USE_LIBUDEV)

INCDIR       += -I. -Iinclude -ITrackable $(RAPIDJSONINC) $(LUA_CPPFLAGS)
INCDIR       += $(HTTPSERVERINC)
INCDIR       += $(RPLIDARINC)

LIBS          = -luuid -ljpeg -lpng -lX11 -lcurl -lpthread -lm -lstdc++fs -latomic

#===========================================================================
# RULES
#===========================================================================

default: lidarTool

#===========================================================================
# DEFAULT RULES
#===========================================================================

$(OBJDIR)%.o: cpp/%.cpp include/%.h $(CIMGDIR)
	mkdir -p $(OBJDIR)
	$(CC) -c $(CC_FLAGS) $(INCDIR) -o $@ $<

$(LDLIDAROFILES): %.o: %.cpp
	$(CC) -c $(CC_FLAGS) $(LDLIDARINC) -o $@ $<

#===========================================================================
# RULES
#===========================================================================

$(CIMGDIR):
	cd Trackable; ./CImg_install.sh

$(YDLIDARDIR):
	./install/$(YDLIDARDIR)_install.sh

$(RPLIDARDIR):
	./install/$(RPLIDARDIR)_install.sh

$(LIBHTTPSERVERDIR):
	./install/$(LIBHTTPSERVERDIR)_install.sh

$(YDLIDARLIB): $(YDLIDARDIR)
	cd $(YDLIDARDIR)/build; $(MAKE)

$(RPLIDARLIB): $(RPLIDARDIR)
	cd $(RPLIDARDIR); $(MAKE)

$(MSLIDARLIB):
	cd $(MSLIDARDIR); mkdir -p build; cd build; cmake ..; $(MAKE)

$(LIBHTTPSERVERLIB): $(LIBHTTPSERVERDIR)
	cd $(LIBHTTPSERVERDIR)/build; $(MAKE)

$(OBJDIR)ydlidarDriver.o: cpp/ydlidarDriver.cpp include/ydlidarDriver.h $(YDLIDARLIB)
	mkdir -p obj
	$(CC) -c $(CC_FLAGS) $(shell test -f YDLidar-SDK/src/YDlidarDriver.h && echo '-DYDLIDARVERSION=117') $(INCDIR) $(YDLIDARINC) -o $@ $<

$(OBJDIR)mslidarDriver.o: cpp/mslidarDriver.cpp include/mslidarDriver.h $(MSLIDARLIB)
	mkdir -p obj
	$(CC) -c $(CC_FLAGS) -I. -Iinclude -ITrackable $(MSLIDARINC) -o $@ $<

$(OBJDIR)ldlidarDriver.o: cpp/ldlidarDriver.cpp $(LDLIDARCPPFILES)
	mkdir -p obj
	$(CC) -c $(CC_FLAGS) -I. -Iinclude -ITrackable $(LD_VERSION) $(LDLIDARINC) -o $@ $<

$(OBJDIR)TrackableLuaObserver.o: Trackable/TrackableLuaObserver.cpp Trackable/TrackableLuaObserver.h
	mkdir -p obj
	$(CC) -c $(CC_FLAGS) -fpermissive $(INCDIR) -o $@ $<

$(OBJDIR)jsonTool.o: Trackable/jsonTool.cpp Trackable/jsonTool.h
	mkdir -p obj
	$(CC) -c $(CC_FLAGS) $(INCDIR) -o $@ $<

$(LIBDIR)lidarKit.a: $(LIDARKITOFILES)
	mkdir -p $(LIBDIR)
	$(AR) rcvs $@ $(LIDARKITOFILES)
	ranlib $@

$(LDLIDARLIB): $(LDLIDAROFILES)
	mkdir -p $(LIBDIR)
	$(AR) rcvs $@ $(LDLIDAROFILES)
	ranlib $@

$(LIBDIR)lidarTrack.a: $(CIMGDIR) $(OBJDIR)lidarTrack.o $(OBJDIR)jsonTool.o Trackable/Trackable.h Trackable/TrackableObserver.h Trackable/TrackableImageObserver.h Trackable/TrackableMQTTObserver.h 
	mkdir -p $(LIBDIR)
	$(AR) rcvs $@ $(OBJDIR)lidarTrack.o $(OBJDIR)jsonTool.o
	ranlib $@

$(OBJDIR)lidarKit.o: $(CIMGDIR) $(RPLIDARLIB)

$(OBJDIR)lidarTool.o: $(LIBHTTPSERVERLIB) $(OBJDIR)lidarKit.o

lidarTool: $(OBJDIR)lidarTool.o $(OBJDIR)TrackableLuaObserver.o $(LIBDIR)lidarKit.a $(LDLIDARLIB) $(LIBDIR)lidarTrack.a
	$(CC) $(CC_FLAGS) $(INCDIR) -o $@ $<  $(OBJDIR)TrackableLuaObserver.o $(LIBDIR)lidarTrack.a $(LIBDIR)lidarKit.a $(LDLIDARLIB) $(HTTPSERVERLIB) $(RPLIDARLIB) $(YDLIDARLIB) $(MSLIDARLIB) $(LIBLOLIB) $(MOSQUITTOLIB) $(CPPWEBSOCKETSLIB) $(LUALIB) $(LIBUDEVLIB) $(LIBS)

$(OBJDIR)packedPlayer.o: cpp/packedPlayer.cpp include/packedPlayer.h
	mkdir -p obj
	$(CC) -c $(CC_FLAGS) $(INCDIR) -o $@ $<

packedPlayer: $(OBJDIR)packedPlayer.o $(OBJDIR)TrackableLuaObserver.o
	$(CC) $(CC_FLAGS) $(INCDIR) -o $@ $< $(OBJDIR)TrackableLuaObserver.o $(LIBLOLIB) $(MOSQUITTOLIB) $(CPPWEBSOCKETSLIB) $(LUALIB) $(LIBS)

#===========================================================================
# IMPLICIT RULES
#===========================================================================

clean:
	rm -f *.o *.so $(OBJDIR)*.o $(LDLIDAROFILES) $(LIBDIR)*.a *~ */*~ *.a *.o lidarTool packedPlayer

distclean: clean
	rm -rf rplidar_sdk YDLidar-SDK oradar_sdk/build libhttpserver Trackable/CImg

