MIO_ROOT=../../..
MIO_LIBRARY_PATH=../../../libs/c/libMIO/
include $(MIO_LIBRARY_PATH)/vars.mk
UNAME := $(shell uname)

CC=gcc
LIBS= -l$(MIO_LIB_NAME) -lstrophe -lexpat -lssl -lresolv -lpthread
ifneq ($(UNAME), Darwin)
	 LIBS+= -luuid
endif

INCLUDE=-I. -I$(STROPHE_INCLUDE) -I$(STROPHE_INCLUDE_SRC) -I$(MIO_LIBRARY_PATH)
CFLAGS=-g -O3 -Wall $(INCLUDE)
LDFLAGS=-L. -L$(MIO_LIBRARY_PATH) -L$(STROPHE_LIB) $(LIBS)

A_SRCS=mio_authenticate.c
AAU_SRCS=mio_admin_user_add.c
ADU_SRCS=mio_admin_user_delete.c
AUU_SRCS=mio_admin_user_update.c
PD_SRCS=mio_publish_data.c
SA_SRCS=mio_actuate.c
PM_SRCS=mio_publish_meta.c
AP_SRCS=mio_acl_publisher_add.c
RP_SRCS=mio_acl_publisher_remove.c
NCO_SRCS=mio_acl_node_configure.c
AS_SRCS=mio_subscribe.c
NC_SRCS=mio_node_create.c
DN_SRCS=mio_node_delete.c
QA_SRCS=mio_acl_affiliations_query.c
QS_SRCS=mio_subscriptions_query.c
US_SRCS=mio_unsubscribe.c
NR_SRCS=mio_node_register.c
PC_SRCS=mio_password_change.c
SL_SRCS=mio_subscribe_listener.c
CNC_SRCS=mio_collection_node_create.c
CNCQ_SRCS=mio_collection_children_query.c
CNPQ_SRCS=mio_collection_parents_query.c
CNCA_SRCS=mio_collection_child_add.c
CNCR_SRCS=mio_collection_child_remove.c
MQ_SRCS=mio_meta_query.c
NTQ_SRCS=mio_node_type_query.c
NGRI_SRCS=mio_item_query.c
MIRQS_SRCS=mio_item_query_stanza.c
MMA_SRCS=mio_meta_add.c
MTA_SRCS=mio_meta_transducer_add.c
MPA_SRCS=mio_meta_property_add.c
MPR_SRCS=mio_meta_property_remove.c
MTR_SRCS=mio_meta_transducer_remove.c
MMR_SRCS=mio_meta_remove.c
MSEA_SRCS=mio_schedule_event_add.c
MSER_SRCS=mio_schedule_event_remove.c
MSQ_SRCS=mio_schedule_query.c
MGA_SRCS=mio_meta_geoloc_add.c
MGR_SRCS=mio_meta_geoloc_remove.c
MRQ_SRCS=mio_reference_query.c
MRCA_SRCS=mio_reference_child_add.c
MRCR_SRCS=mio_reference_child_remove.c
MAP_SRCS=mio_automap.c
MN_SRCS=mio_nuke.c
#PDC_SRCS=mio_pub_device_create.c
#VP_SRCS=mio_virtual_publisher.c


#all: mio_admin_user_add mio_admin_user_delete mio_admin_user_update mio_publish_meta mio_acl_node_configure
all:mio_authenticate mio_publish_data mio_subscribe_listener mio_collection_child_add mio_collection_child_remove mio_node_create mio_collection_node_create mio_collection_children_query mio_collection_parents_query mio_node_type_query mio_subscribe mio_subscriptions_query mio_node_delete mio_unsubscribe mio_node_register mio_password_change mio_actuate mio_acl_affiliations_query mio_acl_publisher_add mio_acl_publisher_remove mio_meta_query mio_item_query mio_item_query_stanza mio_meta_add mio_meta_transducer_add mio_meta_property_add mio_meta_property_remove mio_meta_transducer_remove mio_meta_remove mio_schedule_event_add mio_schedule_event_remove mio_schedule_query mio_meta_geoloc_add mio_meta_geoloc_remove mio_reference_query mio_reference_child_add mio_reference_child_remove mio_automap mio_nuke
#mio_pub_device_create mio_virtual_publisher mio_item_recent_get
 
default: all

mio_authenticate: $(A_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_admin_user_add: $(AAU_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_admin_user_delete: $(ADU_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_admin_user_update: $(AUU_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_publish_data: $(PD_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_publish_meta: $(PM_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_actuate: $(SA_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_acl_publisher_add: $(AP_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_acl_publisher_remove: $(RP_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_acl_node_configure: $(NCO_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_subscribe: $(AS_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_node_create: $(NC_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_node_delete: $(DN_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_acl_affiliations_query: $(QA_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_subscriptions_query: $(QS_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_unsubscribe: $(US_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_node_register: $(NR_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_password_change: $(PC_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_subscribe_listener: $(SL_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_collection_node_create: $(CNC_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_collection_children_query: $(CNCQ_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_collection_parents_query: $(CNPQ_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_collection_child_add: $(CNCA_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_collection_child_remove: $(CNCR_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_meta_query: $(MQ_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_node_type_query: $(NTQ_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_item_query: $(NGRI_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_item_query_stanza: $(MIRQS_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_meta_add: $(MMA_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_meta_transducer_add: $(MTA_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

mio_meta_property_add: $(MPA_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_meta_property_remove: $(MPR_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_meta_transducer_remove: $(MTR_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_meta_remove: $(MMR_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_schedule_event_add: $(MSEA_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_schedule_event_remove: $(MSER_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_schedule_query: $(MSQ_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_meta_geoloc_add: $(MGA_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_meta_geoloc_remove: $(MGR_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_reference_query: $(MRQ_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_reference_child_add: $(MRCA_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_reference_child_remove: $(MRCR_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_automap: $(MAP_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@
	
mio_nuke: $(MN_SRCS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/$@

#mio_pub_device_create: $(PDC_SRCS)
#	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

#mio_virtual_publisher: $(VP_SRCS)
#	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -rf *~ bin/*

