#include "libhue.h"
#include <string.h>
#define CONFIGURATION_USAGE "./configure_user -b [bridge ip] -u [username] "
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

typedef struct configuration_args {
    char* bridge_ip;
    char* username;
} configuration_args_t;

configuration_args_t* parse_configuration_args(int argc, char** args)
{
    configuration_args_t *conf_args = (configuration_args_t*) malloc(sizeof(configuration_args_t));
    memset(conf_args,0x0,sizeof(configuration_args_t));
    int i;
    for (i = 0; i < argc; i++)
    {
        if (strcmp(args[i], "-b") == 0)
        {
            conf_args -> bridge_ip = args[i+1];
            i++;
        } else if (strcmp(args[i], "-u") == 0)
        {
            conf_args -> username = args[i+1];
            i++;
        }  
    }

    return conf_args;
}

int main(int argc ,char** args)
{
    hue_bridge_t* bridge;
    configuration_args_t* conf_args = parse_configuration_args(argc,args);
    hue_configuration_t* config;
    if (conf_args == NULL)
    {
        printf(CONFIGURATION_USAGE);
        return 1;
    }
    if (conf_args -> bridge_ip != NULL)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        bridge = new_hue_bridge();
        bridge -> ip = conf_args -> bridge_ip;
        config = get_configuration(bridge, conf_args -> username);
        free_hue_configuration(config);
        free_hue_bridge(bridge);
        curl_global_cleanup();
      //  if (config )
    } else 
        printf(CONFIGURATION_USAGE);

    return 0;
}
