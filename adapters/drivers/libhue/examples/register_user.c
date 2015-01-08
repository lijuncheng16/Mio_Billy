#include "libhue.h"
#include <curl/curl.h>
#include <string.h>
#define REGISTER_USAGE "./register_user -b [bridge ip] -u [username] -d [device type]"
#include <stdio.h>
#include <stdlib.h>

typedef struct register_args {
    char* bridge_ip;
    char* username;
    char* device_type;
} register_args_t;

register_args_t* parse_register_args(int argc, char** args)
{
    register_args_t *reg_args = (register_args_t*) malloc(sizeof(register_args_t));
    memset(reg_args,0x0,sizeof(register_args_t));
    int i;
    for (i = 0; i < argc; i++)
    {
        if (strcmp(args[i], "-b") == 0)
        {
            reg_args -> bridge_ip = args[i+1];
            i++;
        } else if (strcmp(args[i], "-u") == 0)
        {
            reg_args -> username = args[i+1];
            i++;
        } else if (strcmp(args[i], "-d") == 0)
        {
            reg_args -> device_type = args[i+1];
            i++;
        } 
    }
    return reg_args;
}

int main(int argc ,char** args)
{
    register_args_t* reg_args = parse_register_args(argc,args);
    hue_bridge_t* bridge;
    char* username;
    if (reg_args == NULL)
    {
        printf(REGISTER_USAGE);
        return 1;
    }
    if (reg_args -> bridge_ip != NULL)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        bridge = new_hue_bridge();
        bridge -> ip = reg_args -> bridge_ip;
        username = create_user(bridge, reg_args -> username, reg_args -> device_type);
        if (username != NULL)
        {
          printf("Username %s\n",username);
          free(username);
        } else 
          printf("Username NULL\n");
        curl_global_cleanup();
    } else 
        printf(REGISTER_USAGE);

    return 0;
}
