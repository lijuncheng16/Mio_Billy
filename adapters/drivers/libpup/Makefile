CC = gcc 
UTHASH_INCLUDE=../../libs/uthash
LIBPUP_INCLUDE=./src/
LIBPUP_LIBRARY=./obj/
CFLAGS = -O0 -g -Wall -I $(UTHASH_INCLUDE) -I $(LIBPUP_INCLUDE)
LDFLAGS = -L . -L $(LIBPUP_LIBRARY)

all : libpup.o pup_say_hello pup_write_attribute pup_read_attribute pup_pass_token

pup_pass_token : examples/pup_pass_token.c libpup.o
	  $(CC) $(CFLAGS) -o bin/pup_pass_token examples/pup_pass_token.c obj/libpup.o $(LDFLAGS)
pup_read_attribute : examples/pup_read_attribute.c libpup.o
	  $(CC) $(CFLAGS) -o bin/pup_read_attribute examples/pup_read_attribute.c obj/libpup.o $(LDFLAGS)
pup_write_attribute : examples/pup_write_attribute.c libpup.o
	  $(CC) $(CFLAGS) -o bin/pup_write_attribute examples/pup_write_attribute.c obj/libpup.o $(LDFLAGS)
pup_say_hello : examples/pup_say_hello.c libpup.o
	  $(CC) $(CFLAGS) -o bin/pup_say_hello examples/pup_say_hello.c obj/libpup.o 
libpup.o : src/libpup.c src/libpup.h
	  $(CC) $(CFLAGS) -c src/libpup.c;cp libpup.o obj/libpup.o;rm libpup.o
clean :
	  rm -r -f obj/* bin/*

