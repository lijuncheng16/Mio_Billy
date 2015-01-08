#include <stdio.h>
#include <uthash.h>
#include "libpup.h"
#define USAGE "./pup_read_attribute -addr [address] -f [serial path] -c [channel] -a [attribute]\n"

int32_t main(int32_t argc, char* argv[])
{
  uint32_t addr = 0;
  int32_t arg, value = 0;
  char* serial_path = NULL;
  char attribute[2];
  uint32_t channel;
  for(arg = 1; arg < argc;)
  {
	if (strcmp(argv[arg],"-addr") == 0)
	  sscanf(argv[arg+1],"%d",&addr);
	else if (strcmp(argv[arg],"-f") == 0)
	  sscanf(argv[arg+1],"%s",serial_path);
	else if (strcmp(argv[arg],"-c") == 0)
	  sscanf(argv[arg+1],"0x%x",&channel);
	else if (strcmp(argv[arg],"-a") == 0)
	  sscanf(argv[arg+1],"%s",attribute);
	arg = arg+2;
  }
  if ((addr == 0) || (serial_path == NULL))
  {
	printf(USAGE);
	return -1;
  }

  pup_attribute_t *attributes = NULL;
  pup_attribute_t *attribute_tmp = new_pup_attribute(attribute,NULL,(uint8_t*) &value,sizeof(value));
  HASH_ADD_STR(attributes, attribute_name, attribute_tmp);

  pup_channel_t *channel_tmp = new_pup_channel((uint16_t) channel,NULL,attributes);
  pup_channel_t *channels = NULL;
  HASH_ADD_INT(channels,channel,channel_tmp);

  pup_device_t *device_tmp = new_pup_device((uint16_t) addr, NULL, NULL, channels);
  pup_device_t *devices = NULL;
  HASH_ADD_INT(devices,addr,device_tmp);

  pup_context_t *context = new_pup_context(0,serial_path,9600,devices,0);
  connect_pup_context(context);

  read_attribute(context, addr & 0xFFFF,(uint16_t) channel, attribute_tmp);
  printf("Value %d\n",(int) *attribute_tmp -> value);
  disconnect_pup_context(context);
  free_pup_context(context);
  return 0;
}
