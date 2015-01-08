#include "libhue.h"
#include <string.h>
#define CONFIGURATION_USAGE "./configure_user -b [bridge ip] -u [username] -id [bulb id] -n [bulb name]"
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

typedef struct  {
    char* bridge_ip;
    char* username;
    int bulb_id;
    char* name;
} args_t;

args_t* parse_args(int argc, char** argv)
{
    int i;

    args_t *arg = (args_t*) malloc(sizeof(args_t));
    memset(arg,0x0,sizeof(args_t));
    for (i = 1; i < argc-1; i++)
    {
        if (strcmp(argv[i], "-b") == 0)
        {
            arg -> bridge_ip = argv[i+1];
            i++;
        } else if (strcmp(argv[i], "-u") == 0)
        {
            arg -> username = argv[i+1];
            i++;
        } else if (strcmp(argv[i], "-id") == 0)
        {
            arg -> bulb_id = atoi(argv[i+1]);
            i++;
        } else if (strcmp(argv[i], "-name") == 0)
        {
            arg -> name = argv[i+1];
            i++;
        } 
    }

    return arg;
}

int main(int argc ,char** argv)
{
    int err;
    args_t* arg = parse_args(argc,argv);
    hue_bridge_t* bridge;
    if (arg == NULL)
    {
        printf(CONFIGURATION_USAGE);
        return 1;
    }
    if (arg -> bridge_ip != NULL)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        bridge = new_hue_bridge();
        bridge -> ip = arg -> bridge_ip;
        err = set_light_attributes(bridge, arg -> username, arg -> bulb_id, arg -> name);
        free_hue_bridge(bridge);
        curl_global_cleanup();
      //  if (config )
    } else 
        printf(CONFIGURATION_USAGE);

    return 0;
}
