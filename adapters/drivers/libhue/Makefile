CC = cc 

UTHASH_INCLUDE=./libs/uthash/src/
LIBHUE_INCLUDE=./src/
JSMN_INCLUDE=./libs/jsmn/

LIBHUE_LIBRARY=./obj/
JSMN_LIBRARY=./libs/jsmn/

CURL_LIBRARY=-lcurl
CFLAGS = -O0 -g -Wall -I $(UTHASH_INCLUDE) -I $(LIBHUE_INCLUDE) -I $(JSMN_INCLUDE) 
LDFLAGS = -L . -L $(LIBHUE_LIBRARY) -L $(JSMN_LIBRARY) $(CURL_LIBRARY)

all: libhue.o register_user discover_local_bridges get_configuration get_all_lights get_light_attributes set_light_attributes set_light_state

set_light_state : libhue.o examples/set_light_state.c
	$(CC) $(CFLAGS) -o bin/set_light_state examples/set_light_state.c obj/libhue.o libs/jsmn/jsmn.o $(LDFLAGS)
set_light_attributes : libhue.o examples/set_light_attributes.c
	$(CC) $(CFLAGS) -o bin/set_light_attributes examples/set_light_attributes.c obj/libhue.o libs/jsmn/jsmn.o $(LDFLAGS)
get_configuration : libhue.o examples/get_configuration.c
	$(CC) $(CFLAGS) -o bin/get_configuration examples/get_configuration.c obj/libhue.o libs/jsmn/jsmn.o $(LDFLAGS)
get_all_lights : libhue.o examples/get_all_lights.c
	$(CC) $(CFLAGS) -o bin/get_all_lights examples/get_all_lights.c obj/libhue.o libs/jsmn/jsmn.o $(LDFLAGS)
get_light_attributes : libhue.o examples/get_light_attributes.c
	$(CC) $(CFLAGS) -o bin/get_light_attributes examples/get_light_attributes.c obj/libhue.o libs/jsmn/jsmn.o $(LDFLAGS)
get_all_lights : libhue.o examples/get_all_lights.c
	$(CC) $(CFLAGS) -o bin/get_all_lights examples/get_all_lights.c obj/libhue.o libs/jsmn/jsmn.o $(LDFLAGS)
register_user : libhue.o examples/register_user.c
	$(CC) $(CFLAGS) -o bin/register_user examples/register_user.c obj/libhue.o libs/jsmn/jsmn.o $(LDFLAGS)
discover_local_bridges : libhue.o examples/discover_local_bridges.c
	$(CC) $(CFLAGS) -o bin/discover_local_bridges examples/discover_local_bridges.c obj/libhue.o libs/jsmn/jsmn.o $(LDFLAGS)
libhue.o : src/libhue.c src/libhue.h
	  $(CC) $(CFLAGS) -c src/libhue.c;cp libhue.o obj/libhue.o;rm libhue.o
clean :
	  rm -r -f obj/* bin/*

