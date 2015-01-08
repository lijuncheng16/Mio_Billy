#ifndef __TILE_SERVER_H__
#define __TILE_SERVER_H__

#define DATASTORE_DIR "."
#define KVS_DIR "../store.kvs"

int gettile(int uid, char* channel_name, int tile_level, long long tile_offset, int http_fd, char* http_header_buf);
void send_json_http(int fd, char *header_buf, const char *data_buf, int data_size);

#endif
