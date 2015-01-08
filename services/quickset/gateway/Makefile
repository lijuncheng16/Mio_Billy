MIO_ROOT=../../../
MIO_LIBRARY_PATH=../../../libs/c/libMIO/
include $(MIO_LIBRARY_PATH)/vars.mk

all : adapter_dispatcher


CC=gcc -O3
AR=ar
INCLUDE=-I$(STROPHE_INCLUDE) -I$(STROPHE_INCLUDE_SRC) -I$(MIO_LIBRARY_PATH) -I.src/
CFLAGS=-g -Wall
LIBS = -lstrophe -lexpat -lssl -lresolv -lpthread -l$(MIO_LIB_NAME)
LDFLAGS= -L$(STROPHE_LIB) -L$(MIO_LIBRARY_PATH) $(LIBS) -L$(STROPHE_INCLUDE_SRC)

adapter_dispatcher: src/adapter_dispatcher.c
	$(CC) $(CFLAGS) $(INCLUDE) -o bin/adapter_dispatcher src/adapter_dispatcher.c $(LDFLAGS) $(MIO_LIBRARY_PATH)/libMIO.a $(STROPHE_INCLUDE)/libstrophe.a
