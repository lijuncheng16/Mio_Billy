#include "libhue.h"
#include <string.h>
#define GET_LIGHT_ATTRIBUTES_USAGE "./get_attributes_user -b [bridge ip] -u [username] -id [bulb id]"
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

typedef struct  {
    char* bridge_ip;
    char* username;
    int bulb_id;
} light_attr_args_t;

light_attr_args_t* parse_light_attr_args(int argc, char** args)
{
    light_attr_args_t *light_attr_args = (light_attr_args_t*) malloc(sizeof(light_attr_args_t));
    memset(light_attr_args,0x0,sizeof(light_attr_args_t));
    int i;
    for (i = 0; i < argc; i++)
    {
        if (strcmp(args[i], "-b") == 0)
        {
            light_attr_args -> bridge_ip = args[i+1];
            i++;
        } else if (strcmp(args[i], "-u") == 0)
        {
            light_attr_args -> username = args[i+1];
            i++;
        }  else if (strcmp(args[i], "-id") == 0)
        {
          light_attr_args -> bulb_id = atoi(args[i+1]);
          i++;
        }
    }

    return light_attr_args;
}

int main(int argc ,char** args)
{
    light_attr_args_t* light_attr_args = parse_light_attr_args(argc,args);
    hue_bulb_t *bulb;
    hue_bulb_t *bulb_tmp;
    hue_state_t *state;
    hue_bridge_t* bridge;
    int bulb_id;
    hue_user_t *user;
    if (light_attr_args == NULL)
    {
        printf(GET_LIGHT_ATTRIBUTES_USAGE);
        return 1;
    }
    if (light_attr_args -> bridge_ip != NULL)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        bridge = new_hue_bridge();
        bridge -> ip = light_attr_args -> bridge_ip;
        user = malloc(sizeof(hue_user_t));
        memset(user,0x0,sizeof(hue_user_t));
        user -> username = light_attr_args -> username;
        bulb_id =  light_attr_args -> bulb_id;
        bulb = get_light_attributes(bridge, user, bulb_id);
        bulb_tmp = bulb;
        while (bulb_tmp != NULL)
        {
            state = bulb_tmp -> state;
            printf("HUE BULB: \nID: %d\nNAME: %s\nTYPE: %s\nMODEL: %s\nSW VERSION: %s\nSTATE:\n\tON: %d\n\tBRI: %d\n\tHUE: %d\n\tSAT: %d\n\tCT %d\n\tALERT: %s\n\tEFFECT: %s\n\tCOLORMODE: %s\n\tX,Y: %lf,%lf\n\tREACHABLE: %d\n",
                 bulb_tmp -> id, bulb_tmp -> name, bulb_tmp -> type, bulb_tmp -> model, 
                 bulb_tmp -> sw_version, state -> on, state -> bri, state -> hue, state -> sat, 
                 state -> ct, state -> alert, state -> effect, state -> colormode, state -> x, state -> y,
                 state -> reachable);
            bulb_tmp = bulb_tmp -> next;
        }  
        free_hue_bulb(bulb);
        curl_global_cleanup();
    } else 
        printf(GET_LIGHT_ATTRIBUTES_USAGE);

    return 0;
}
