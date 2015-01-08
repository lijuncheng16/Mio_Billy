#include <stdio.h>
#include <uthash.h>
#include "libpup.h"
#define USAGE "./pup_pass_token -naddr [neighbour address] -addr [address] -f [serial path] -t [timeout]\n"

int32_t main(int32_t argc, char* argv[])
{
  uint32_t addr = 0, naddr =0;
  int32_t arg,timeout = 0;
  char serial_path[40];
  for(arg = 1; arg < argc;)
  {
	if (strcmp(argv[arg],"-addr") == 0)
	  sscanf(argv[arg+1],"%d",&addr);
	else if (strcmp(argv[arg],"-naddr") == 0)
	  sscanf(argv[arg+1],"%d",&naddr);
	else if (strcmp(argv[arg],"-f") == 0)
	  sscanf(argv[arg+1],"%s",serial_path);
	else if (strcmp(argv[arg],"-t") == 0)
	  sscanf(argv[arg+1],"%d",&timeout);

	arg = arg+2;
  }
  if ((addr == 0) || (serial_path == NULL))
  {
	printf(USAGE);
	return -1;
  }

  pup_context_t *context = new_pup_context(addr,serial_path,9600,NULL,timeout);
  context -> next_master = new_pup_device(naddr,NULL,NULL,NULL);
  connect_pup_context(context);
  pup_frame_t* frame;
  while (TRUE)
  {
	while(TRUE)
	{
	  frame = read_frame(context -> fd);
	  if ((frame -> command == pup_pass_token) && (frame -> addr == addr))
		break;
	}
	printf("received token\n");
	pass_token(context, 0);
	printf("passed token\n");
	free(frame);
  }

  disconnect_pup_context(context);
  free_pup_context(context);
  return 0;
}
