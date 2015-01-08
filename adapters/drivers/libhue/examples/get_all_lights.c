#include "libhue.h"
#include <string.h>
#define GET_LIGHTS_USAGE "./get_all_lights -b [bridge ip] -u [username] "
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

typedef struct {
    char* bridge_ip;
    char* username;
} get_lights_args_t;

get_lights_args_t* parse_get_lights_args(int argc, char** args)
{
    get_lights_args_t *get_lights_args = (get_lights_args_t*) malloc(sizeof(get_lights_args_t));
    memset(get_lights_args,0x0,sizeof(get_lights_args_t));
    int i;
    for (i = 0; i < argc; i++)
    {
        if (strcmp(args[i], "-b") == 0)
        {
            get_lights_args -> bridge_ip = args[i+1];
            i++;
        } else if (strcmp(args[i], "-u") == 0)
        {
            get_lights_args -> username = args[i+1];
            i++;
        }  
    }

    return get_lights_args;
}

int main(int argc ,char** args)
{
    get_lights_args_t* get_lights_args = parse_get_lights_args(argc,args);
    hue_bulb_t* light_tmp;
    hue_bridge_t* bridge;
    hue_user_t *user;
    if (get_lights_args == NULL)
    {
        printf(GET_LIGHTS_USAGE);
        return 1;
    }
    if (get_lights_args -> bridge_ip != NULL)
    {
        
        curl_global_init(CURL_GLOBAL_ALL);
        bridge = new_hue_bridge();
        bridge -> ip = get_lights_args -> bridge_ip;
        user = malloc(sizeof(hue_user_t));
        memset(user,0x0,sizeof(hue_user_t));
        user -> username = get_lights_args -> username;
        hue_bulb_t* lights = get_all_lights(bridge, user);
        light_tmp = lights;
        while (light_tmp != NULL)
        {
             printf("LIGHT:\nBULB ID: %d \nBULB NAME: %s\n", light_tmp -> id,
                 light_tmp -> name);
             light_tmp = light_tmp -> next;
        }
        free_hue_bulb(lights);
        free_hue_bridge(bridge);
        curl_global_cleanup();
    } else 
        printf(GET_LIGHTS_USAGE);

    return 0;
}
