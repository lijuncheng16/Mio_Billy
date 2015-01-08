#include "../../libs/c/libMIO/mio.h"
#include "../../libs/c/libMIO/mio_handlers.h"
#include "meta_tool.h"

#define LUTRON_AREA "lutron_area\0"
#define FIREFLY_SENSOR "firefly_sensor\0"
#define FIREFLY_PLUG "firefly_plug\0"
#define ENFUSE_LOCATION "enfuse_location\0"
#define ENFUSE_PANEL "enfuse_panel\0"
#define ENFUSE_BRANCH "enfuse_branch\0"
#define BOSCH_TWIST "bosch_twist\0"
#define WISTAT "wistat\0"
#define PHILLIPS_HUE "hue_bulb"
#define HUE_BRIDGE "hue_bridge"

#define USAGE "./meta_tool -n [device name] -type [device type] -id [node_id] -u [username] -p [password] -t [parent uuid in tree] -a [add reference to child 1 or 0] -acm [access control model]\n"

typedef struct {
	char* name;
	char* type;
	char* node_id;
	char* parent_id;
	int add_ref_child;
	char* acm;
	char* username;
	char* password;
} args_t;


int add_enfuse_branch_meta(mio_meta_t *device_meta);
int add_enfuse_location_meta(mio_meta_t *device_meta);
int add_enfuse_panel_meta(mio_meta_t *device_meta);
int add_bosch_twist_meta(mio_meta_t *device_meta);
int add_wistat_meta(mio_meta_t *device_meta);
int add_phillips_hue_meta(mio_meta_t *device_meta);
int add_wistat_meta(mio_meta_t *device_meta);
int add_hue_bridge_meta(mio_meta_t *device_meta);

args_t* _parse_args(int argc, char **argv)
{
	int i;
	args_t *args = malloc(sizeof(args_t));
	memset(args,0x0,sizeof(args_t));
	for (i = 1; i < argc; i+=2)
	{
        printf("%s\n",argv[i]);
		if (strcmp(argv[i],"-n") == 0)
		{
			args -> name = argv[i+1];
		} else if (strcmp(argv[i],"-type") == 0)
		{
			args -> type = argv[i+1];
		} else if (strcmp(argv[i],"-id") == 0)
			args -> node_id = argv[i+1];
		else if (strcmp(argv[i],"-u") == 0)
			args -> username = argv[i+1];
		else if (strcmp(argv[i],"-p") == 0)
			args -> password = argv[i+1];
		else if (strcmp(argv[i],"-t") == 0)
			args -> parent_id = argv[i+1];
		else if (strcmp(argv[i],"-a") == 0)
			args -> add_ref_child = atoi(argv[i+1]);
		else if (strcmp(argv[i],"-acm") == 0)
			args -> add_ref_child = atoi(argv[i+1]);
	}
	if (args -> node_id == NULL)
	{
		printf("event node id (-id) required\n");
		free(args);
		args = NULL;
	} else if (args -> username == NULL)
	{ 
		printf("username (-u) required\n");
		free(args);
		args = NULL;
	} else if (args -> password == NULL)
	{
		printf("password (-p) required\n");
		free(args);
		args = NULL;
	}
	return args;
}

int main(int argc, char **argv)
{
	int err;
	args_t *args = _parse_args(argc, argv);


	if (args == NULL)
	{
		printf(USAGE);
		return 1;
	}

	mio_conn_t *conn = mio_conn_new(MIO_LEVEL_ERROR);
	mio_connect(args -> username, args -> password, NULL, NULL, conn);
	err = register_event_meta(conn, args -> type, args -> name, args -> node_id, args -> parent_id, args -> acm, args -> add_ref_child);

	if (err != 0)
	{
		printf("error registering event node\n");
	}
	mio_disconnect(conn);
	mio_conn_free(conn);
	if (err != 0)
	{
		printf("Meta Tool: There was an error publishing meta information\n");
		return 1;
	}
	return 0;
}



int register_event_meta(mio_conn_t *conn, char* type, char* name, char* uuid, 
	char* parent_uuid, char* acm, int add_ref_child)
{
	int err;
	int actuate = 0;
	char act_uuid [125];
	mio_response_t *response = mio_response_new();
	err = mio_node_create(conn,uuid,name,acm,response);
	mio_response_print(response);
	mio_response_free(response);

	// publish meta information 
	mio_meta_t *device_meta = mio_meta_new();
	device_meta -> name = name;
    if (type == NULL)
    {
      printf("type is NULL\n");
      return 1;
    }

	if (strcmp(type, LUTRON_AREA) == 0)
	{
		device_meta -> meta_type =  MIO_META_TYPE_DEVICE;
		err = add_lutron_area_meta(device_meta);
		actuate = 1;
	} else if (strcmp(type, FIREFLY_SENSOR) == 0)
	{
		device_meta -> meta_type =  MIO_META_TYPE_DEVICE;
		err = add_firefly_sensor_meta(device_meta);
	} else if (strcmp(type, FIREFLY_PLUG) == 0)
	{
		device_meta -> meta_type =  MIO_META_TYPE_DEVICE;
		err = add_firefly_plug_meta(device_meta);
		actuate = 1;
	} else if (strcmp(type, ENFUSE_LOCATION) == 0)
	{
		device_meta -> meta_type =  MIO_META_TYPE_LOCATION;
		err = add_enfuse_location_meta(device_meta);
	} else if (strcmp(type, ENFUSE_PANEL) == 0)
	{
		device_meta -> meta_type =  MIO_META_TYPE_LOCATION;
		err = add_enfuse_panel_meta(device_meta);
	} else if (strcmp(type, ENFUSE_BRANCH) == 0)
	{
		device_meta -> meta_type =  MIO_META_TYPE_DEVICE;
		err = add_enfuse_branch_meta(device_meta);
	} else if (strcmp(type, BOSCH_TWIST) == 0)
	{
		device_meta -> meta_type = MIO_META_TYPE_DEVICE;
		err = add_bosch_twist_meta(device_meta);
		actuate = 1;
	} else if (strcmp(type, WISTAT) == 0)
	{

		device_meta -> meta_type = MIO_META_TYPE_DEVICE;
		err = add_wistat_meta(device_meta);
		actuate = 1;
	} else if (strcmp(type, PHILLIPS_HUE) == 0)
	{
		device_meta -> meta_type = MIO_META_TYPE_DEVICE;
		err = add_phillips_hue_meta(device_meta);
		actuate = 1;
        
	} else if (strcmp(type, HUE_BRIDGE) == 0)
	{
		device_meta -> meta_type = MIO_META_TYPE_LOCATION;
		err = add_hue_bridge_meta(device_meta);
	}

	if (actuate == 1) 
	{
		sprintf(act_uuid,"%s_act", uuid);
		response = mio_response_new();
		err = mio_node_create(conn,act_uuid,name,acm,response);
		mio_response_print(response);
		mio_response_free(response);
        response = mio_response_new();
        mio_subscribe(conn, act_uuid,response);
        mio_response_free(response);
	}

	response = mio_response_new();
    	device_meta->timestamp = mio_timestamp_create();
	mio_meta_merge_publish(conn, uuid, device_meta,device_meta -> transducers, NULL, response);
	mio_response_print(response);
	mio_response_free(response);
    response = mio_response_new();
    mio_subscribe(conn, uuid,response);
    printf("here\n");
    mio_response_free(response);
	if (parent_uuid != NULL)
	{
        printf("adding parent\n");
		response = mio_response_new();
		if(add_ref_child)
			mio_reference_child_add(conn, parent_uuid, uuid, MIO_ADD_REFERENCE_AT_CHILD, response);
		else
			mio_reference_child_add(conn, parent_uuid, uuid, MIO_NO_REFERENCE_AT_CHILD, response);
		mio_response_free(response);
	} else 
    {
       printf("no parents\n");
    }
	return 0;
}




int add_lutron_area_meta(mio_meta_t *device_meta)
{

	mio_transducer_meta_t *transducer_tmp = NULL;
	mio_property_meta_t *property_tmp = NULL;
	mio_enum_map_meta_t *enum_tmp = NULL;

	device_meta -> info = "A Lutron Lighting Area\0"; 
	device_meta -> meta_type = MIO_META_TYPE_DEVICE;

	device_meta -> properties = mio_property_meta_new();
	property_tmp  = device_meta -> properties;
	property_tmp -> name = "type\0";
        property_tmp -> value = "BACnet\0";

        property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "BACnet Type\0";
	property_tmp -> value = "Lutron Area\0";
	

	// BACnet Specific fields
	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "IP Address\0";
	property_tmp  -> value = "\0";
	
	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Device Image\0";
	property_tmp  -> value = "http://sensor.andrew.cmu.edu/device-imgs/lutron_area.jpg\0";
	
	device_meta -> transducers = mio_transducer_meta_new();
	transducer_tmp = device_meta -> transducers;	
	transducer_tmp -> name = "Lighting State\0";
	transducer_tmp -> type = "state light\0";
	transducer_tmp -> interface = "Lighting State\0";
	transducer_tmp -> info = "On or off lighting state\0";
	transducer_tmp -> unit = "enum\0";
	transducer_tmp -> enumeration = mio_enum_map_meta_new();

	enum_tmp = transducer_tmp -> enumeration;
	enum_tmp -> name = "Off\0";
	enum_tmp  -> value = "0\0";
	enum_tmp -> next = mio_enum_map_meta_new();
	enum_tmp  -> next ->  name = "On\0";
	enum_tmp  -> next -> value = "1\0";

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp  = transducer_tmp -> properties;
	property_tmp -> name = "Object ID\0";
        property_tmp -> value = "3\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Type\0";
	property_tmp -> value = "BV\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Name\0";
	property_tmp -> value = "Lighting State\0";


	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Lighting Level\0"; 	
	transducer_tmp -> interface = "Lighting Level\0"; 	
	transducer_tmp -> type = "light dim percentage\0";
	transducer_tmp -> info = "Percentage of power given to light.\0";
	transducer_tmp -> unit = "Percentage\0";
	transducer_tmp -> min_value = "0\0";
	transducer_tmp -> max_value = "100\0";

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp  = transducer_tmp -> properties;
	property_tmp -> name = "Object ID\0";
        property_tmp -> value = "2\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Type\0";
	property_tmp -> value = "AV\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Name\0";
	property_tmp -> value = "Lighting Level\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Daylighting Enabled\0";
	transducer_tmp -> interface = "Daylighting Enabled\0"; 	
	transducer_tmp -> type = "light daylighting\0";
	transducer_tmp -> interface = "Daylighting\0";
	transducer_tmp -> info = "If Daylighting Enabled will dim and brighten lights based on luminesence. Will not do this if Disabled.\0";
	transducer_tmp -> unit = "enum\0";
	transducer_tmp -> enumeration = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> name = "Disabled\0";
	transducer_tmp -> enumeration -> value = "0\0";
	transducer_tmp -> enumeration -> next = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> next -> name = "Enabled\0";
	transducer_tmp -> enumeration -> next -> value = "1\0";
	
	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp  = transducer_tmp -> properties;
	property_tmp -> name = "Object ID\0";
        property_tmp -> value = "5\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Type\0";
	property_tmp -> value = "BV\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Name\0";
	property_tmp -> value = "Daylighting Enabled\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Permanently Disable Occupancy\0";
	transducer_tmp -> interface = "Permanently Disable Occupancy\0"; 	
	transducer_tmp -> type = "light occupancy\0";
	transducer_tmp -> interface = "Occupancy\0";
	transducer_tmp -> info = "If Occupancy Enabled will turn on or off lights based on whether the room is occupied. Will not do this if Disabled.\0";
	transducer_tmp -> unit = "enum\0";
	transducer_tmp -> enumeration = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> name = "Disabled\0";
	transducer_tmp -> enumeration -> value = "0\0";
	transducer_tmp -> enumeration -> next = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> next -> name = "Enabled\0";
	transducer_tmp -> enumeration -> next -> value = "1\0";

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp  = transducer_tmp -> properties;
	property_tmp -> name = "Object ID\0";
        property_tmp -> value = "7\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Type\0";
	property_tmp -> value = "BV\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Name\0";
	property_tmp -> value = "Permanently Disable Occupancy\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Loadshed Allowed\0";
	transducer_tmp -> interface = "Loadshed Allowed\0"; 	
	transducer_tmp -> type = "light loadshed\0";
	transducer_tmp -> interface = "Loadshed\0";
	transducer_tmp -> info = "If Loadshed Enabled will shed load to meet specified percentage of current load. Will not do this if Disabled.\0";
	transducer_tmp -> unit = "enum\0";
	transducer_tmp -> enumeration = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> name = "Disabled\0";
	transducer_tmp -> enumeration -> value = "0\0";
	transducer_tmp -> enumeration -> next = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> next -> name = "Enabled\0";
	transducer_tmp -> enumeration -> next -> value = "1\0";

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp  = transducer_tmp -> properties;
	property_tmp -> name = "Object ID\0";
        property_tmp -> value = "12\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Type\0";
	property_tmp -> value = "BV\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Name\0";
	property_tmp -> value = "Loadshed Allowed\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Loadshed Goal\0"; 	
	transducer_tmp -> interface = "Loadshed Goal\0"; 	
	transducer_tmp -> type = "light loadshed\0";
	transducer_tmp -> info = "Percentage of current light power to keep when Loadshed Allowed is Enabled.\0";
	transducer_tmp -> unit = "Percentage\0";
	transducer_tmp -> min_value = "0\0";
	transducer_tmp -> max_value = "100\0";
	
	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp  = transducer_tmp -> properties;
	property_tmp -> name = "Object ID\0";
        property_tmp -> value = "13\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Type\0";
	property_tmp -> value = "AV\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Name\0";
	property_tmp -> value = "Loadshed Goal\0";


	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Total Power\0"; 	
	transducer_tmp -> type = "light power\0";
	transducer_tmp -> info = "Total Power being used in the area.\0";
	transducer_tmp -> unit = "Watt\0";
	transducer_tmp -> min_value = "0\0";

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp  = transducer_tmp -> properties;
	property_tmp -> name = "Object ID\0";
        property_tmp -> value = "18\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Type\0";
	property_tmp -> value = "AV\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Object Name\0";
	property_tmp -> value = "Total Power\0";

	return 0;
}

int add_firefly_sensor_meta(mio_meta_t *device_meta)
{

	mio_transducer_meta_t *transducer_tmp = NULL;

	device_meta -> info = "A Firefly wireless sensing device.\0"; 

	device_meta -> properties = mio_property_meta_new();
	device_meta -> properties -> name = "type\0";
    device_meta -> properties -> value = "Firefly Sensor\0";
	device_meta -> properties -> next = mio_property_meta_new();
    device_meta -> properties -> next -> name = "Firefly Type\0";
    device_meta -> properties -> next -> value = "Firefly Sensor\0";

	transducer_tmp = mio_transducer_meta_new();	
	device_meta -> transducers = transducer_tmp;
	transducer_tmp -> name = "Battery Level\0";
	transducer_tmp -> type = "battery\0";
	transducer_tmp -> info = "Voltage at battery terminals.\0";
	transducer_tmp -> unit = "Voltage\0";

	transducer_tmp -> next = mio_transducer_meta_new();
	transducer_tmp = transducer_tmp -> next;	
	transducer_tmp -> name = "Light Meter\0";
	transducer_tmp -> type = "light intensity\0";
	transducer_tmp -> info = "Light intensity measured in Lumens at firefly light sensor.\0";
	transducer_tmp -> unit = "Lux\0";
	transducer_tmp -> min_value = "0\0";
	transducer_tmp -> max_value = "1024\0";

	transducer_tmp -> next = mio_transducer_meta_new();
	transducer_tmp = transducer_tmp -> next;	
	transducer_tmp -> name = "Thermometer Analog\0";
	transducer_tmp -> type = "temperature analog\0";
	transducer_tmp -> info = "Analog Thermometer reading.\0";
	transducer_tmp -> unit = "Celcius\0";
	transducer_tmp -> min_value = "-50\0";
	transducer_tmp -> max_value = "100\0";
	
	transducer_tmp -> next = mio_transducer_meta_new();
	transducer_tmp = transducer_tmp -> next;	
        transducer_tmp -> name = "Accelerometer X\0";
	transducer_tmp -> type = "accelerometer\0";
	transducer_tmp -> info = "Aceleration in firefly X direction.\0";
	transducer_tmp -> unit = "G\0";
	transducer_tmp -> min_value = "-3\0";
	transducer_tmp -> max_value = "3\0";

	transducer_tmp -> next = mio_transducer_meta_new();
	transducer_tmp = transducer_tmp -> next;	
	transducer_tmp -> name = "Accelerometer Y\0";
	transducer_tmp -> type = "accelerometer\0";
	transducer_tmp -> info = "Aceleration in firefly Y direction.\0";
	transducer_tmp -> unit = "G\0";
	transducer_tmp -> min_value = "-3\0";
	transducer_tmp -> max_value = "3\0";

	transducer_tmp -> next = mio_transducer_meta_new();
	transducer_tmp = transducer_tmp -> next;	
	transducer_tmp -> name = "Accelerometer Z\0";
	transducer_tmp -> type = "accelerometer\0";
	transducer_tmp -> info = "Aceleration in firefly Z direction.\0";
	transducer_tmp -> unit = "G\0";
	transducer_tmp -> min_value = "-3\0";
	transducer_tmp -> max_value = "3\0";

	transducer_tmp -> next = mio_transducer_meta_new();
	transducer_tmp = transducer_tmp -> next;	
	transducer_tmp -> name = "Microphone\0";
	transducer_tmp -> type = "audio\0";
	transducer_tmp -> info = "Received decible strength at sampled time.\0";
	transducer_tmp -> unit = "SPL\0";
	transducer_tmp -> min_value = "0\0";
	transducer_tmp -> max_value = "100\0";

	transducer_tmp -> next = mio_transducer_meta_new();
	transducer_tmp = transducer_tmp -> next;	
	transducer_tmp -> name = "Humidity Sensor\0";
	transducer_tmp -> type = "humidity\0";
	transducer_tmp -> info = "Sensed humidity as percentage of firefly sensor.\0";
	transducer_tmp -> unit = "kg/kg\0";
	transducer_tmp -> min_value = "0";
	transducer_tmp -> max_value = "100";

	transducer_tmp -> next = mio_transducer_meta_new();
	transducer_tmp = transducer_tmp -> next;	
	transducer_tmp -> name = "Barometer\0";
	transducer_tmp -> type = "barometer\0";
	transducer_tmp -> info = "Sensed barometric pressure as pascal at firefly sensor.\0";
	transducer_tmp -> unit = "hpa\0";
	transducer_tmp -> min_value = "0\0";
	transducer_tmp -> max_value = "100\0";

	transducer_tmp -> next = mio_transducer_meta_new();
	transducer_tmp = transducer_tmp -> next;	
	transducer_tmp -> name = "Motion Sensor\0";
	transducer_tmp -> type = "barometer\0";
	transducer_tmp -> info = "PIR enabled motion sensing.\0";
	transducer_tmp -> unit = "Intensity\0";
	transducer_tmp -> min_value = "0\0";
	transducer_tmp -> max_value = "100\0";

	return 0;
}



int add_firefly_plug_meta(mio_meta_t *device_meta)
{

	mio_transducer_meta_t *transducer_tmp = NULL;

	device_meta -> info = "A firefly plug meter for power sensing and actuation.\0"; 
	device_meta -> properties = mio_property_meta_new();
	device_meta -> properties -> name = "type\0";
    device_meta -> properties -> value = "Firefly Plug\0";
    device_meta -> properties -> next = mio_property_meta_new();
    device_meta -> properties -> next -> name = "Firefly Type";
    device_meta -> properties -> next -> value = "Firefly Plug";


	device_meta -> transducers = mio_transducer_meta_new();
	transducer_tmp = device_meta -> transducers;	
	transducer_tmp = device_meta -> transducers;	
	transducer_tmp -> name = "Frequency Meter\0";
	transducer_tmp -> type = "frequency power\0";
	transducer_tmp -> info = "Frequency of power line signal.\0";
	transducer_tmp -> unit = "Voltage\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "RSSI\0"; 	
	transducer_tmp -> type = "rssi 2.4Ghz\0";
	transducer_tmp -> info = "Received signal strength indicator for the 2.4Ghz radio.\0";
	transducer_tmp -> unit = "Decibel\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Uptime Counter\0"; 	
	transducer_tmp -> type = "counter uptime\0";
	transducer_tmp -> info = "Count in seconds the firefly has been up.\0";
	transducer_tmp -> unit = "Count\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "RMS Voltmeter\0"; 	
	transducer_tmp -> type = "voltage rms\0";
	transducer_tmp -> info = "The root mean squared estimate of voltage.\0";
	transducer_tmp -> unit = "Volt\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Socket State\0"; 	
	transducer_tmp -> interface = "Socket State\0";
	transducer_tmp -> type = "State of socket relay.\0";
	transducer_tmp -> info = "Relay state of the plug socket.\0";
	transducer_tmp -> unit = "enum\0";
	transducer_tmp -> enumeration = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> name = "Off\0";
	transducer_tmp -> enumeration -> value = "0\0";
	transducer_tmp -> enumeration -> next = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> next -> name = "On\0";
	transducer_tmp -> enumeration -> next -> value = "1\0";




	return 0;
}

int add_enfuse_branch_meta(mio_meta_t *device_meta)
{
	mio_transducer_meta_t *transducer_tmp = NULL;
	mio_property_meta_t *property_tmp = NULL;

	device_meta -> info = "Electrical readings using enfuse system of a branch of specified breaker.\0"; 
	device_meta -> properties = mio_property_meta_new();
	property_tmp = device_meta -> properties;
	property_tmp -> name = "type\0";
    property_tmp -> value = "Enfuse Branch\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Branch ID\0";
        property_tmp -> value = "\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Panel ID\0";
        property_tmp -> value = "\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Location ID\0";
        property_tmp -> value = "\0";

	device_meta -> transducers = mio_transducer_meta_new();
	transducer_tmp = device_meta -> transducers;	
	transducer_tmp = device_meta -> transducers;	
	transducer_tmp -> name = "Watt Minutes\0";
	transducer_tmp -> type = "breaker power watt-minute\0";
	transducer_tmp -> info = "Watt Minute Reading.\0";
	transducer_tmp -> unit = "Watt Minute\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "va\0"; 	
	transducer_tmp -> type = "volt-amp power breaker\0";
	transducer_tmp -> info = "Volt Ampere reading of the enfuse branch.\0";
	transducer_tmp -> unit = "Volt Ampere\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "kw\0"; 	
	transducer_tmp -> type = "watt power breaker\0";
	transducer_tmp -> info = "Kilowatts of power consumption at enfuse branch.\0";
	transducer_tmp -> unit = "Kilo Watt\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Minimum Voltage\0"; 	
	transducer_tmp -> type = "voltage breaker min\0";
	transducer_tmp -> info = "Minimum Voltage seen at breaker.\0";
	transducer_tmp -> unit = "Volt\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Minimum Current\0"; 	
	transducer_tmp -> type = "current breaker min\0";
	transducer_tmp -> info = "Minimum current seen at breaker.\0";
	transducer_tmp -> unit = "Amperage\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Maximum Voltage\0"; 	
	transducer_tmp -> type = "voltage breaker max\0";
	transducer_tmp -> info = "Maximum Voltage seen at breaker.\0";
	transducer_tmp -> unit = "Volt\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Maximum Current\0"; 	
	transducer_tmp -> type = "current breaker max\0";
	transducer_tmp -> info = "Maximum current seen at breaker.\0";
	transducer_tmp -> unit = "Amperage\0";
    return 0;
}

int add_enfuse_panel_meta(mio_meta_t *device_meta)
{
	mio_property_meta_t *property_tmp = NULL;
	device_meta -> info = "Enfuse Panel device which monitors one circuit breaker. Links to the branches where data streems can be found..\0"; 

	device_meta -> properties = mio_property_meta_new();
	property_tmp = device_meta -> properties;
	property_tmp -> name = "type\0";
        property_tmp -> value = "Enfuse Panel\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Panel ID\0";
        property_tmp -> value = "\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Location ID\0";
        property_tmp -> value = "\0";


    return 0;
}

int add_enfuse_location_meta(mio_meta_t *device_meta)
{
	mio_property_meta_t *property_tmp = NULL;
	device_meta -> info = "Represents an Inscope Enfuse Deplyment at a single location.\0"; 

	device_meta -> properties = mio_property_meta_new();
	property_tmp = device_meta -> properties;
	property_tmp -> name = "type\0";
        property_tmp -> value = "Enfuse Location\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Location ID\0";
        property_tmp -> value = "\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "User\0";
        property_tmp -> value = "\0";

	property_tmp -> next = mio_property_meta_new();
        property_tmp = property_tmp -> next;
	property_tmp -> name = "Password\0";
        property_tmp -> value = "\0";
	
	device_meta -> geoloc = mio_geoloc_new();
	device_meta -> geoloc -> street = "\0";
	device_meta -> geoloc ->lat = 0;
	device_meta -> geoloc -> lon = 0;

    return 0;
}


int add_bosch_twist_meta(mio_meta_t *device_meta)
{

	mio_property_meta_t *property = NULL;
	mio_transducer_meta_t *transducer_tmp = NULL;

	device_meta -> info = "Bosch Twist Node sensor.\0"; 
	device_meta -> meta_type = MIO_META_TYPE_DEVICE;

	device_meta -> properties = mio_property_meta_new();
	property = device_meta -> properties;
	property -> name = "type\0";
    property -> value = "Twist\0";

	property -> next = mio_property_meta_new();
	property = property -> next;
	property-> name = "MAC Address\0";
        property -> value = "\0";


	device_meta -> transducers = mio_transducer_meta_new();
	transducer_tmp = device_meta -> transducers;	
	transducer_tmp -> name = "Temperature\0";
	transducer_tmp -> type = "temperature\0";
	transducer_tmp -> info = "Sensed temrperature.\0";
    	transducer_tmp -> unit = "Celcius\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Humidity\0"; 	
	transducer_tmp -> type = "humidity\0";
	transducer_tmp -> info = "Sensed percent humidity.\0";
	transducer_tmp -> unit = "Percent\0";
	transducer_tmp -> min_value = "0\0";
	transducer_tmp -> max_value = "100\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Pressure\0"; 	
	transducer_tmp -> type = "barometric pressure\0";
	transducer_tmp -> info = "Sensed barometric pressure.\0";
	transducer_tmp -> unit = "Bar\0";

    	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Vibration\0"; 	
	transducer_tmp -> type = "acceleration\0";
	transducer_tmp -> info = "Measure of acceleration.\0";
	transducer_tmp -> unit = "Acceleration\0";

    	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Microphone\0"; 	
	transducer_tmp -> type = "sound microphone\0";
	transducer_tmp -> info = "Measure of sound level.\0";
	transducer_tmp -> unit = "Decibel\0";
    
    	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Magnetic Field\0"; 	
	transducer_tmp -> type = "flux magentic field\0";
	transducer_tmp -> info = "Measure of magnetic flux at point.\0";
	transducer_tmp -> unit = "Decibel\0";


    	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Light\0"; 	
	transducer_tmp -> type = "luminance photosensor\0";
	transducer_tmp -> info = "Measure of light received at twist node.\0";
	transducer_tmp -> unit = "Lux\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "RSSI\0"; 	
	transducer_tmp -> type = "rssi 2.4Ghz\0";
	transducer_tmp -> info = "RSSI value of signal to twist node.\0";
	transducer_tmp -> unit = "None\0";


	return 0;
}

int add_wistat_meta(mio_meta_t *device_meta)
{
	mio_property_meta_t *property_tmp = NULL;
     	mio_transducer_meta_t *transducer_tmp = NULL;
     
	device_meta -> info = "Millenial Net Wistat, wireless thermostat.\0"; 
	device_meta -> meta_type = MIO_META_TYPE_DEVICE;

	device_meta -> properties = mio_property_meta_new();
	property_tmp = device_meta -> properties;
	property_tmp -> name = "type\0";
    property_tmp -> value = "Modbus\0";
    property_tmp -> next = mio_property_meta_new();
	property_tmp -> next -> name = "Modbus Type\0";
    property_tmp -> next -> value = "Wistat\0";
    property_tmp = property_tmp -> next;

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Address\0";
    property_tmp -> value = "\0";

	device_meta -> transducers = mio_transducer_meta_new();
	transducer_tmp = device_meta -> transducers;	
	transducer_tmp -> name = "RSSI\0";
	transducer_tmp -> type = "rssi 2.4Ghz\0";
	transducer_tmp -> info = "RSSI of signal from thermostat.\0";
	transducer_tmp -> unit = "rssi\0";

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp = transducer_tmp -> properties;
	property_tmp -> name = "Address\0";
	property_tmp -> value = "204\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Type\0";
	property_tmp -> value = "FE2\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Length\0";
	property_tmp -> value = "8\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Set Point Type\0"; 	
	transducer_tmp -> type = "temperature setpoint\0";
	transducer_tmp -> info = "State that the setpoint is in.\0";
	transducer_tmp -> enumeration = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> name = "Off\0";
	transducer_tmp -> enumeration -> value = "0\0";
	transducer_tmp -> enumeration -> next = mio_enum_map_meta_new();
	transducer_tmp -> unit = "enum\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Branch Line Pressure\0"; 	
	transducer_tmp -> type = "pneumatic thermostat\0";
	transducer_tmp -> info = "Measured Branch Line pneumatic pressure\0";
	transducer_tmp -> unit = "psi\0";

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp = transducer_tmp -> properties;
	property_tmp -> name = "Address\0";
	property_tmp -> value = "58\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Type\0";
	property_tmp -> value = "FE2\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Length\0";
	property_tmp -> value = "8\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Branch Line Set Point\0"; 	
	transducer_tmp -> type = "pneumatic thermostat\0";
	transducer_tmp -> info = "Measured Branch Line pneumatic pressure\0";
	transducer_tmp -> unit = "psi\0";

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp = transducer_tmp -> properties;
	property_tmp -> name = "Address\0";
	property_tmp -> value = "58\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Type\0";
	property_tmp -> value = "FE2\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Length\0";
	property_tmp -> value = "8\0";

    property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Offset\0";
	property_tmp -> value = "8\0";




	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Current Mode\0"; 	
	transducer_tmp -> type = "thermostat mode\0";
	transducer_tmp -> info = "Mode of operation thermostat is in.\0";
	transducer_tmp -> unit = "enum\0";
	transducer_tmp -> enumeration = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> name = "Off\0";
	transducer_tmp -> enumeration -> value = "0\0";
	transducer_tmp -> enumeration -> next = mio_enum_map_meta_new();

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp = transducer_tmp -> properties;
	property_tmp -> name = "Address\0";
	property_tmp -> value = "60\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Type\0";
	property_tmp -> value = "INT\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Length\0";
	property_tmp -> value = "16\0";



	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Temperature Set Point\0"; 	
	transducer_tmp -> type = "setpoint thermostat\0";
	transducer_tmp -> info = "Temperature which the thermostat is trying reach.\0";
	transducer_tmp -> unit = "Celcius\0";

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp = transducer_tmp -> properties;
	property_tmp -> name = "Address\0";
	property_tmp -> value = "61\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Type\0";
	property_tmp -> value = "FE2\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Length\0";
	property_tmp -> value = "16\0";


	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "User Requested Set Point\0"; 	
	transducer_tmp -> interface = "User Requested Set Point\0"; 	
	transducer_tmp -> type = "setpoint thermostat\0";
	transducer_tmp -> info = "Temperature set point the user has entered .\0";
	transducer_tmp -> unit = "Celcius\0";

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp = transducer_tmp -> properties;
	property_tmp -> name = "Address\0";
	property_tmp -> value = "62\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Type\0";
	property_tmp -> value = "FE2\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Length\0";
	property_tmp -> value = "16\0";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Error Status\0"; 	
	transducer_tmp -> type = "error status thermostat\0";
	transducer_tmp -> info = "Error registered with thermostat.\0";
	transducer_tmp -> unit = "enum\0";
	transducer_tmp -> enumeration = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> name = "Off\0";
	transducer_tmp -> enumeration -> value = "0\0";
	transducer_tmp -> enumeration -> next = mio_enum_map_meta_new();

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp = transducer_tmp -> properties;
	property_tmp -> name = "Address\0";
	property_tmp -> value = "68\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Type\0";
	property_tmp -> value = "INT\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Length\0";
	property_tmp -> value = "16\0";


	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "HVAC Mode\0"; 	
	transducer_tmp -> type = "status thermostat\0";
	transducer_tmp -> info = "The current hvac control state.\0";
	transducer_tmp -> unit = "enum\0";
	transducer_tmp -> enumeration = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> name = "Off\0";
	transducer_tmp -> enumeration -> value = "0\0";
	transducer_tmp -> enumeration -> next = mio_enum_map_meta_new();

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp = transducer_tmp -> properties;
	property_tmp -> name = "Address\0";
	property_tmp -> value = "Unset\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Type\0";
	property_tmp -> value = "75\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Length\0";
	property_tmp -> value = "16\0";



	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Temperature\0"; 	
	transducer_tmp -> type = "temperature thermostat\0";
	transducer_tmp -> info = "The sensed temperature at thermostat.\0";
	transducer_tmp -> unit = "Celcius\0";

	transducer_tmp -> properties = mio_property_meta_new();
	property_tmp = transducer_tmp -> properties;
	property_tmp -> name = "Address\0";
	property_tmp -> value = "57\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Type\0";
	property_tmp -> value = "FE2\0";

	property_tmp -> next = mio_property_meta_new();
	property_tmp = property_tmp -> next;
	property_tmp -> name = "Register Length\0";
	property_tmp -> value = "16\0";

	return 0;
}

int add_phillips_hue_meta(mio_meta_t *device_meta)
{
    mio_transducer_meta_t *transducer_tmp = NULL;
    mio_property_meta_t *property_tmp; 

	device_meta -> meta_type = MIO_META_TYPE_DEVICE;
	device_meta -> info = "Phillips Hue light bulb.\0"; 

	device_meta -> properties = mio_property_meta_new();
	property_tmp = device_meta -> properties;
	property_tmp -> name = "type\0";
    property_tmp -> value = "Hue Bulb\0";

	device_meta -> transducers = mio_transducer_meta_new();
	transducer_tmp = device_meta -> transducers;	
	transducer_tmp -> name = "On\0";
	transducer_tmp -> interface = "On\0";
	transducer_tmp -> type = "light state\0";
	transducer_tmp -> info = "The state of the hue bulb, on or off.\0";
	transducer_tmp -> unit = "rssi\0";
	transducer_tmp -> enumeration = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> name = "Off\0";
	transducer_tmp -> enumeration -> value = "0\0";
	transducer_tmp -> enumeration -> next = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> name = "On\0";
	transducer_tmp -> enumeration -> value = "1\0";
	
	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Brightness\0"; 	
	transducer_tmp -> interface = "Brightness\0"; 	
	transducer_tmp -> type = "brightness light\0";
	transducer_tmp -> info = "Brightness value of hue.\0";
	transducer_tmp -> unit = "Brightness Value\0";
    transducer_tmp -> min_value = "0";
    transducer_tmp -> max_value = "255";
	
	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Hue\0"; 	
	transducer_tmp -> interface = "Hue\0"; 	
	transducer_tmp -> type = "hue light\0";
	transducer_tmp -> info = "Hue value of the bulb.\0";
	transducer_tmp -> unit = "Hue\0";
    transducer_tmp -> min_value = "0";
    transducer_tmp -> max_value = "65535";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Saturation\0"; 	
	transducer_tmp -> interface = "Saturation\0"; 	
	transducer_tmp -> type = "saturation light\0";
	transducer_tmp -> info = "Saturation value of the bulb.\0";
	transducer_tmp -> unit = "Saturation\0";
    transducer_tmp -> min_value = "0";
    transducer_tmp -> max_value = "255";

	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Color Temperature\0"; 	
	transducer_tmp -> interface = "Color Temperature\0"; 	
	transducer_tmp -> type = "color temp light\0";
	transducer_tmp -> info = "Color temperature of the bulb.\0";
	transducer_tmp -> unit = "Color Temperature\0";
    transducer_tmp -> min_value = "0";
    transducer_tmp -> max_value = "65535";
	
	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "X\0"; 	
	transducer_tmp -> type = "color position light\0";
	transducer_tmp -> info = "Color value of the bulb.\0";
	transducer_tmp -> unit = "Color Value\0";
	
	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Y\0"; 	
	transducer_tmp -> type = "color position light\0";
	transducer_tmp -> info = "Color value of the bulb.\0";
	transducer_tmp -> unit = "Color Value\0";
	
	transducer_tmp -> next = mio_transducer_meta_new();	
	transducer_tmp = transducer_tmp -> next;
	transducer_tmp -> name = "Reachable\0"; 	
	transducer_tmp -> type = "status light\0";
	transducer_tmp -> info = "Whether the hue gateway can reach the bulb.\0";
	transducer_tmp -> unit = "enum\0";
    transducer_tmp -> enumeration = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> name = "False\0";
	transducer_tmp -> enumeration -> value = "0\0";
	transducer_tmp -> enumeration -> next = mio_enum_map_meta_new();
	transducer_tmp -> enumeration -> next ->  name = "True\0";
	transducer_tmp -> enumeration -> next ->  value = "1\0";
	return 0;	
}
int add_hue_bridge_meta(mio_meta_t *device_meta)
{
    	mio_property_meta_t *property_tmp; 

	device_meta -> info = "Phillips Hue bridge.\0"; 

	device_meta -> properties = mio_property_meta_new();
	property_tmp = device_meta -> properties;
	property_tmp -> name = "type\0";
    property_tmp -> value = "Hue Bridge\0";

	return 0;
}
