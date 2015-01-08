CC = gcc 
UTHASH_INCLUDE=./libs/uthash/
JSMN_INCLUDE=./libs/jsmn/
JSMN_LIBRARY=$(JSMN_INCLUDE)
LIBFUSE_INCLUDE=./src/
LIBFUSE_LIBRARY=./obj/
CFLAGS = -O0 -g -Wall -I $(UTHASH_INCLUDE) -I $(LIBFUSE_INCLUDE) -I $(JSMN_INCLUDE)
LDFLAGS = -L . -L $(LIBFUSE_LIBRARY) -lcurl -L $(JSMN_LIBRARY)

all : libfuse.o enfuse_authenticate enfuse_get_locations enfuse_get_panels enfuse_get_branches enfuse_get_status enfuse_get_data

enfuse_get_data : examples/enfuse_get_data.c libfuse.o
	  $(CC) $(CFLAGS) -o bin/enfuse_get_data examples/enfuse_get_data.c obj/libfuse.o $(JSMN_LIBRARY)/jsmn.o $(LDFLAGS)
enfuse_get_status : examples/enfuse_get_status.c libfuse.o
	  $(CC) $(CFLAGS) -o bin/enfuse_get_status examples/enfuse_get_status.c obj/libfuse.o $(JSMN_LIBRARY)/jsmn.o $(LDFLAGS)
enfuse_get_branches : examples/enfuse_get_branches.c libfuse.o
	  $(CC) $(CFLAGS) -o bin/enfuse_get_branches examples/enfuse_get_branches.c obj/libfuse.o $(JSMN_LIBRARY)/jsmn.o $(LDFLAGS)
enfuse_get_panels : examples/enfuse_get_panels.c libfuse.o
	  $(CC) $(CFLAGS) -o bin/enfuse_get_panels examples/enfuse_get_panels.c obj/libfuse.o $(JSMN_LIBRARY)/jsmn.o $(LDFLAGS)
enfuse_get_locations : examples/enfuse_get_locations.c libfuse.o
	  $(CC) $(CFLAGS) -o bin/enfuse_get_locations examples/enfuse_get_locations.c obj/libfuse.o $(JSMN_LIBRARY)/jsmn.o $(LDFLAGS)
enfuse_authenticate : examples/enfuse_authenticate.c libfuse.o
	  $(CC) $(CFLAGS) -o bin/enfuse_authenticate examples/enfuse_authenticate.c obj/libfuse.o $(JSMN_LIBRARY)/jsmn.o $(LDFLAGS)
libfuse.o : src/libfuse.c src/libfuse.h 
	  $(CC) $(CFLAGS) -c src/libfuse.c;mv libfuse.o obj/libfuse.o
clean :
	  rm -r -f obj/* bin/*

