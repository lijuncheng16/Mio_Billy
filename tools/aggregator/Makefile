#!/usr/bin/make -f
UNAME := $(shell uname)

############ MORTAR IO LIBRARIES ###############
MIO_ROOT=../../
MIO_LIBRARY_PATH=../../libs/c/libMIO/
include $(MIO_LIBRARY_PATH)/vars.mk

ifneq ($(UNAME),Darwin)
	LIBS+=-luuid
	DFLAGS+=-DUNIX
	ALLIB+= $(BACNETLIB)
endif

UTHASH_INCLUDE=./libs/uthash/

CC=gcc -O3

CFLAGS =-g -Wall

LIBS += -lstrophe -lexpat -lssl -lresolv -lpthread -l$(MIO_LIB_NAME) 

INCLUDE = -I$(UTHASH_INCLUDE) -I$(STROPHE_INCLUDE) -I$(STROPHE_INCLUDE_SRC) -I$(MIO_LIBRARY_PATH) 
LDFLAGS = -L$(STROPHE_LIB)  -L$(MIO_LIBRARY_PATH) $(LIBS)

aggregate: src/tmp_agg.c
	$(CC) $(CFLAGS) $(INCLUDE)  -o bin/aggregate src/tmp_agg.c $(LDFLAGS)

clean:
	rm -r bin/*

