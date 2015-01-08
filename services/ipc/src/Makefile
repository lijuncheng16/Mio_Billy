CC = gcc -O3
#MORTAR IO LIBRARIES
MIO_ROOT=../..
MIO_LIBRARY_PATH=../../libs/libc_strophe/
include $(MIO_LIBRARY_PATH)/vars.mk

UTHASH_INCLUDE=libs/uthash/
UNAME := $(shell uname)
LIBS = -lcurl -lstrophe -lexpat -lssl -lresolv -lpthread -l$(MIO_LIB_NAME)
ifneq ($(UNAME),Darwin)
	LIBS+=-luuid
endif

INCLUDE = -I$(UTHASH_INCLUDE) -I$(STROPHE_INCLUDE) -I$(STROPHE_INCLUDE_SRC) -I$(MIO_LIBRARY_PATH)  
CFLAGS = -g -Wall $(INCLUDE)
LDFLAGS = -L./obj/ -L/usr/local/lib/ -L$(STROPHE_LIB) -L$(MIO_LIBRARY_PATH) -L$(STROPHE_LIB)  $(LIBS) 

all: multiplexer.o ipc

ipc: multiplexer.o ipc.c ipc.h
	$(CC) $(CFLAGS) -o bin/ipc ipc.c $(LDFLAGS) obj/multiplexer.o

multiplexer.o: multiplexer.c multiplexer.h
	$(CC) $(CFLAGS) -c multiplexer.c;mv multiplexer.o obj/ 

clean: 
	rm -r -f bin/* obj/*
