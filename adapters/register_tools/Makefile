MIO_ROOT=../..
MIO_LIBRARY_PATH=../../libs/c/libMIO/
include $(MIO_LIBRARY_PATH)/vars.mk
UNAME := $(shell uname)

CC=gcc
LIBS= -l$(MIO_LIB_NAME) -lstrophe -lexpat -lssl -lresolv -lpthread
ifneq ($(UNAME), Darwin)
	 LIBS+= -luuid
endif

INCLUDE=-I. -I$(STROPHE_INCLUDE) -I$(STROPHE_INCLUDE_SRC) -I$(MIO_LIBRARY_PATH)
CFLAGS=-g -O0 -Wall $(INCLUDE)
LDFLAGS=-L. -L$(MIO_LIBRARY_PATH) -L$(STROPHE_LIB) $(LIBS)


MT_SRCS=src/meta_tool.c

all: meta_tool

meta_tool: $(MT_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

clean:
	rm -rf bin/*
