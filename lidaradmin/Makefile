#// Copyright (c) 2022 ZKM | Hertz-Lab (http://www.zkm.de)
#// Bernd Lintermann <bernd.lintermann@zkm.de>
#//
#// BSD Simplified License.
#// For information on usage and redistribution, and for a DISCLAIMER OF ALL
#// WARRANTIES, see the file, "LICENSE" in this distribution.
#//


#===========================================================================
# HTTPSERVER
#===========================================================================

HTTPSERVERDIR    = ../lidartool/libhttpserver

LIBMICROHTTPD    = libmicrohttpd
LIBHTTPSERVERLIB = $(HTTPSERVERDIR)/build/src/.libs/libhttpserver.a
HTTPSERVERLIB    = $(LIBHTTPSERVERLIB)

ifeq ($(shell pkg-config --exists $(LIBMICROHTTPD) && echo -n true),true)
	HTTPSERVERLIB += -lmicrohttpd
else
	HTTPSERVERLIB += $(HTTPSERVERDIR)/build/src/.libs/libhttpserver.a $(HTTPSERVERDIR)/../libmicrohttpd/lib/libmicrohttpd.a
endif

HTTPSERVERINC = -I$(HTTPSERVERDIR)/src -I$(HTTPSERVERDIR)/include/

ifeq ($(shell grep render_GET $(HTTPSERVERDIR)/src/httpserver/http_resource.hpp | grep 'virtual const' | wc -l),1)
	HTTPSERVERINC += -Drender_const=const
else
	HTTPSERVERINC += -Drender_const=
endif

#===========================================================================
# OBJDIR
#===========================================================================

OBJDIR	     = obj/

#===========================================================================
# APPLICATION
#===========================================================================

#===========================================================================
# CC
#===========================================================================

CC 	     = g++
CC_FLAGS     += -g -std=c++17

INCDIR       += -I. -Iinclude $(HTTPSERVERINC)

#===========================================================================
# RULES
#===========================================================================

default: lidarAdmin

#===========================================================================
# DEFAULT RULES
#===========================================================================

$(OBJDIR)%.o: %.cpp include/%.h
	mkdir -p obj
	$(CC) -c $(CC_FLAGS) $(INCDIR) -o $@ $<

#===========================================================================
# RULES
#===========================================================================

$(OBJDIR)jsonTool.o: Trackable/jsonTool.cpp Trackable/jsonTool.h
	mkdir -p obj
	$(CC) -c $(CC_FLAGS) $(INCDIR) -o $@ $<

lidarAdmin: $(OBJDIR)lidarAdmin.o
	$(CC) $(CC_FLAGS) $(INCDIR) -o $@ $< $(HTTPSERVERLIB) -lcurl -lpthread -lm 

#===========================================================================
# IMPLICIT RULES
#===========================================================================

clean:
	rm -f *.o *.so $(OBJDIR)*.o *~ */*~ lidarAdmin


