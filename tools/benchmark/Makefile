CC = gcc

#MORTAR IO LIBRARIES
MIO_ROOT=../../
MIO_LIBRARY_PATH=../../libs/libc_strophe/
include $(MIO_LIBRARY_PATH)/vars.mk

UNAME := $(shell uname)
LIBS = -lcurl -lstrophe -lexpat -lssl -lresolv -lpthread -l$(MIO_LIB_NAME)
ifneq ($(UNAME),Darwin)
	LIBS+=-luuid
endif




INCLUDE = -I$(STROPHE_INCLUDE) -I$(STROPHE_INCLUDE_SRC) -I$(MIO_LIBRARY_PATH) -I$(MIO_LIBRARY)
CFLAGS = -g -Wall $(INCLUDE)
LDFLAGS = -L./obj/ -L$(STROPHE_LIB) -L$(MIO_LIBRARY_PATH) -L$(STROPHE_LIB) $(LIBS)


all : benchmark

benchmark : benchmark.c

adapters : libfuse_adapter.o
	$(CC) $(CFLAGS) -o bin/adapter adapter/adapter.c $(LDFLAGS)

clean :
	  rm -r -f obj/* bin/*

