MIO_ROOT=../../../
include ./vars.mk
UNAME := $(shell uname)

CC=gcc
AR=ar
RANLIB=ranlib
INCLUDE=-I. -I$(STROPHE_INCLUDE) -I$(STROPHE_INCLUDE_SRC) -I../libs/libstrophe/expat/lib/

LIBS=-lstrophe -lexpat -lssl -lresolv -lpthread
ifneq ($(UNAME), Darwin)
	 LIBS+= -luuid
endif

CFLAGS=-Wall -g -O0
LDFLAGS=-L. -L$(STROPHE_LIB) $(LIBS)
LIB_SOURCES= mio.c mio_handlers.c

LIB_OBJECTS = $(LIB_SOURCES:.c=.o)

all: lib

lib: lib$(MIO_LIB_NAME).a

default: lib

lib$(MIO_LIB_NAME).a: $(LIB_OBJECTS)
	$(AR) rcs lib$(MIO_LIB_NAME).a $(LIB_OBJECTS) 

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -rf *~ $(LIB_OBJECTS) lib$(MIO_LIB_NAME).a
