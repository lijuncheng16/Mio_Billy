MIO_ROOT=../..
MIO_LIBRARY_PATH=../../libs/c/libMIO/
UTHASH_INCLUDE=./uthash/src/
include $(MIO_LIBRARY_PATH)/vars.mk
UNAME := $(shell uname)

CC=gcc
LIBS= -l$(MIO_LIB_NAME) -lstrophe -lexpat -lssl -lresolv -lpthread `pkg-config fuse --libs`
ifneq ($(UNAME), Darwin)
	 LIBS+= -luuid
endif

INCLUDE=-I$(STROPHE_INCLUDE) -I$(STROPHE_INCLUDE_SRC) -I$(MIO_LIBRARY_PATH) -I$(UTHASH_INCLUDE)
CFLAGS=-g -O0 -Wall $(INCLUDE) `pkg-config fuse --cflags`
LDFLAGS=-L$(MIO_LIBRARY_PATH) -L$(STROPHE_LIB) $(LIBS)

all:miofs

miofs: miofs.o mio_client.o
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o bin/$@

%.o: %.c
	$(CC) -c $< $(CFLAGS)


clean:
	rm -rf *~ *.o bin/*

