#ifndef META_TOOL_H
#define META_TOOL_H

int register_event_meta(mio_conn_t *conn, char* type, char* name, char* uuid, 
	char* parent_uuid, char* acm, int add_ref_child);
int add_lutron_area_meta(mio_meta_t *device_meta);
int add_firefly_sensor_meta(mio_meta_t *device_meta);
int add_firefly_plug_meta(mio_meta_t *device_meta);

#endif
