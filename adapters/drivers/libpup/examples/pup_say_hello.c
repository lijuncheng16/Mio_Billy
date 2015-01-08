#include <stdio.h>
#include "libpup.h"

#define USAGE "./say_hello -a [address] -f [serial path]\n"

int32_t main(int32_t argc, char* argv[])
{
  uint32_t addr = 0;
  int32_t arg;
  char* serial_path = NULL;
  for(arg = 1; arg < argc;)
  {
	if (strcmp(argv[arg],"-a") == 0)
	  sscanf(argv[arg+1],"%d",&addr);
	else if (strcmp(argv[arg],"-f") == 0)
	{
	  serial_path = (char*) malloc(40);
	  sscanf(argv[arg+1],"%s",serial_path);
	}
	arg = arg+2;
  }
  if ((addr == 0) || (serial_path == NULL))
  {
	printf(USAGE);
	return -1;
  }

  pup_context_t *context = new_pup_context(0,serial_path,9600,NULL,0);
  if (context == NULL)
  {
    printf("Could not create a new pup_context");
    free_pup_context(context);
  }

  if (connect_pup_context(context))
  {
    printf( "Could not connect to %s\nexiting\n", serial_path);
    free_pup_context(context);
    return -1;
  }
  if (say_hello(context, addr & 0xFFFF) == -1)
  {
    printf("Hello not acknowledged\n");
  } else 
  {
    printf("Hello acknowledged\n");
  }
  disconnect_pup_context(context);
  free_pup_context(context);
  return 0;
}
