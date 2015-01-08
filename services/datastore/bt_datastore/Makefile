
JSON_DIR = json-cpp-master

CPPFLAGS = -g -Wall -I$(JSON_DIR)/include -O3
# LDFLAGS = -L$(JSON_DIR)/libs -ljson_linux_libmt -static

JSON_SRCS = \
	$(JSON_DIR)/src/lib_json/json_value.cpp \
	$(JSON_DIR)/src/lib_json/json_reader.cpp \
	$(JSON_DIR)/src/lib_json/json_writer.cpp

SRCS = BinaryIO.cpp Binrec.cpp Channel.cpp crc32.cpp FilesystemKVS.cpp KVS.cpp Log.cpp Tile.cpp utils.cpp $(JSON_SRCS)

INCLUDES = BinaryIO.h Binrec.h Channel.h ChannelInfo.h crc32.h DataSample.h FilesystemKVS.h KVS.h Log.h Tile.h TileIndex.h

ifeq ($(shell uname -s),Linux)
  #LDFLAGS = -static
  LDFLAGS =
else
  # Need to build boost libs using macports!
  LDFLAGS =
endif

# SOURCES=tilegen.cpp mysql_common.cpp MysqlQuery.cpp Channel.cpp Logrec.cpp Tile.cpp utils.cpp Log.cpp

INSTALL_BINS=export import gettile info tile_server

all: $(INSTALL_BINS)

test: all
	make -C test

clean:
	rm -f $(INSTALL_BINS)
	make -C test clean

ARCH:=$(shell uname -s -m | sed 's/ /_/' | tr 'A-Z' 'a-z')

install-local: $(INSTALL_BINS)
	mkdir -p ../website/lib/datastore/$(ARCH)
	cp $^ ../website/lib/datastore/$(ARCH)

install-dev-local: $(INSTALL_BINS)
	mkdir -p ../website-dev/lib/datastore/$(ARCH)
	cp $^ ../website-dev/lib/datastore/$(ARCH)

install-prod-local: $(INSTALL_BINS)
	mkdir -p ../website-prod/lib/datastore/$(ARCH)
	cp $^ ../website-prod/lib/datastore/$(ARCH)

install-static: $(INSTALL_BINS)
	mkdir -p /u/apps/bodytrack/static/$(ARCH)
	cp $^ /u/apps/bodytrack/static/$(ARCH)

install-remote: $(INSTALL_BINS)
	scp $^ bodytrack2:/u/apps/bodytrack/static/$(ARCH)

install-deploy: $(INSTALL_BINS)
	mkdir -p /u/apps/bodytrack/current/lib/datastore/$(ARCH)
	rsync -a $^ /u/apps/bodytrack/current/lib/datastore/$(ARCH)

install-test-deploy: $(INSTALL_BINS)
	mkdir -p /u/apps/bodytrack-test/current/lib/datastore/$(ARCH)
	rsync -a $^ /u/apps/bodytrack-test/current/lib/datastore/$(ARCH)


#$(JSON_DIR)/libs/libjson_libmt.a:
#	(cd $(JSON_DIR) && python scons.py platform=linux-gcc && #cd libs && ln -sf linux*/*.a libjson_libmt.a)

copy: copy.cpp BinaryIO.cpp BinaryIO.h Binrec.cpp Binrec.h Channel.cpp Channel.h ChannelInfo.h crc32.cpp crc32.h DataSample.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Log.cpp Log.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

export: export.cpp $(SRCS) $(INCLUDES)
	g++ $(CPPFLAGS) $@.cpp -o $@ $(SRCS) $(LDFLAGS)

gettile: gettile.cpp $(SRCS) $(INCLUDES)
	g++ $(CPPFLAGS) $@.cpp -o $@ $(SRCS) $(LDFLAGS)


tile_server: tile_server.cpp gettile4server.cpp $(SRCS) $(INCLUDES)
	g++ $(CPPFLAGS) $@.cpp gettile4server.cpp -o $@ $(SRCS) $(LDFLAGS)


IMPORT_SRCS = import.cpp ImportBT.cpp ImportJson.cpp

import: $(IMPORT_SRCS) $(SRCS) $(INCLUDES)
	g++ $(CPPFLAGS) $(IMPORT_SRCS) -o $@ $(SRCS) $(LDFLAGS)

info: info.cpp $(SRCS) $(INCLUDES)
	g++ $(CPPFLAGS) $@.cpp -o $@ $(SRCS) $(LDFLAGS)

docs:
	doxygen KVS.cpp KVS.h

read_bt: Binrec.cpp crc32.cpp DataStore.cpp utils.cpp Log.cpp $(JSON_DIR)/libs
	g++ -g $(CPPFLAGS) Binrec.cpp crc32.cpp DataStore.cpp utils.cpp -o $@  -L./$(JSON_DIR)/libs/linux-gcc -ljson_linux-gcc
	./read_bt

#$(JSON_DIR)/libs:
#	cd $(JSON_DIR); python scons.py platform=linux-gcc check
#	cd $(JSON_DIR)/libs; ln -sf linux-gcc-* linux-gcc
#	cd $(JSON_DIR)/libs/linux-gcc; ln -sf libjson_linux-gcc-*.a libjson_linux-gcc.a

tilegen: $(SOURCES) $(INCLUDES)
	g++ -g $(CPPFLAGS) $(SOURCES) -o $@ $(LDFLAGS) 

#-./tilegen

tile.8:
	time curl -b btsession http://localhost:3000/tiles/1/Josh_Basestation.Temperature/8.9835.json > 8.9835.json

tile.7:
	time curl -b btsession http://localhost:3000/tiles/1/Josh_Basestation.Temperature/7.19670.json > 7.19670.json

tile.6:
	time curl -b btsession http://localhost:3000/tiles/1/Josh_Basestation.Temperature/6.39340.json > 6.39340.json

tile.0:
	time curl -b btsession http://localhost:3000/tiles/1/Josh_Basestation.Temperature/0.2517760.json > 0.2517760.json

clean-tiles:
	echo "delete from tiles where ch_name='Josh_Basestation.Temperature';" | /usr/local/mysql/bin/mysql -u root bodytrack-dev

login:
	curl -c btsession http://localhost:3000/login.json -d login=anne -d password=bodytrack

