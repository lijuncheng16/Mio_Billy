#include "libhue.h"
#include <string.h>
#define CONFIGURATION_USAGE "./configure_user -b [bridge ip] -u [username] \n"
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

typedef struct link_char {
    char* val;
    struct link_char *next;
} link_char_t;

typedef struct light_state_args {
    char* bridge_ip;
    char* username;
    int bulb_id;
    link_char_t* state_names;
    link_char_t* state_values;
    int n_states;
} light_state_args_t;

light_state_args_t* parse_light_state_args(int argc, char** args)
{
    char* c;
    int i;
    link_char_t* tmp;
    light_state_args_t *state_args = (light_state_args_t*) malloc(sizeof(light_state_args_t));
    memset(state_args,0x0,sizeof(light_state_args_t));
    for (i = 1; i < argc; i++)
    {
        if (strcmp(args[i], "-b") == 0)
        {
            state_args -> bridge_ip = args[i+1];
            i++;
        } else if (strcmp(args[i], "-u") == 0)
        {
            state_args -> username = args[i+1];
            i++;
        } else if (strcmp(args[i], "-id") == 0)
        {
            state_args -> bulb_id = atoi(args[i+1]);
            i++;
        } else if (strcmp(args[i], "-names") == 0)
        {
            state_args -> n_states = 0;
            c = strtok(args[i+1],",");
            tmp = malloc(sizeof(link_char_t));
            memset(tmp,0x0,sizeof(link_char_t));
            state_args -> state_names = tmp;
            tmp -> val = c;
            while ((c = strtok ( NULL, "," )) != NULL)
            {
               tmp -> next = malloc(sizeof(link_char_t));
               tmp = tmp -> next;
               memset(tmp,0x0,sizeof(link_char_t));
               tmp -> val = c;
               state_args -> n_states++;
            }
            
            i++;
        }  else if (strcmp(args[i], "-values"))
        { 
            state_args -> n_states = 0;
            c = strtok(args[i+1],",");
            tmp = malloc(sizeof(link_char_t));
            memset(tmp,0x0,sizeof(link_char_t));
            state_args -> state_values = tmp;
            tmp -> val = c;
            while ((c = strtok ( NULL, "," )) != NULL)
            {
               tmp -> next = malloc(sizeof(link_char_t));
               tmp = tmp -> next;
               memset(tmp,0x0,sizeof(link_char_t));
               tmp -> val = c;
               state_args -> n_states++;
            }
            
            i++; 
        }
    }

    return state_args;
}

int main(int argc ,char** args)
{
    int err;
    light_state_args_t* light_args = parse_light_state_args(argc,args);
    link_char_t* name, *value;
    if (light_args == NULL)
    {
        printf(CONFIGURATION_USAGE);
        return 1;
    }
    if (light_args -> bridge_ip != NULL)
    {
        hue_bridge_t* bridge = new_hue_bridge();
        bridge -> ip = light_args -> bridge_ip;
        name = light_args -> state_names;
        value = light_args -> state_values;
        while((name != NULL) && (value != NULL))
        {
     printf("here\n");
            err = set_light_state(bridge, light_args -> username, light_args -> bulb_id, name -> val, value -> val);
            name = name -> next;
            value = value -> next;
        }
        free_hue_bridge(bridge);
    } else {
        printf(CONFIGURATION_USAGE);
    }
    return 0;
}
