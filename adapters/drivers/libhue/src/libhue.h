#ifndef LIBHUE_H
#define LIBHUE_H
#include <stdint.h>
#include <string.h>
#include <uthash.h>
// uncomment and use when Phillip implements feature
// #define POINT_SYMBOL

// URL www.meethue.com/api/nupnp
#define DISCOVER_BRIDGES_URL "https://www.meethue.com/api/nupnp"
#define DISCOVER_URL_SIZE   5000
#define N_TOKENS            512

typedef struct hue_state
{
    int on;
    int bri;
    int hue;
    int sat;
    int ct; // mirrored color temperature
    double x;
    double y;
    char* alert;
    char* effect;
    char* colormode;
    int reachable;
} hue_state_t;

hue_state_t* new_hue_state();
void free_hue_state();

typedef struct hue_bulb
{
    int id;             // bulb id
    hue_state_t *state;
    char* type;
    char* name;         // bulb name
    char* model;        // model id
    char* sw_version;   // version of software
    struct hue_bulb *next;
} hue_bulb_t;

hue_bulb_t* new_hue_bulb();
void free_hue_bulb(hue_bulb_t* bulb);

typedef struct hue_user
{
    char json_url[255];
    char json_addr[255];
    char* username;
    hue_bulb_t* bulbs;
} hue_user_t;

hue_user_t* new_hue_user(char* url, char* addr, hue_bulb_t* bulbs);
void release_hue_user(hue_user_t* user);

typedef struct hue_configuration
{
    uint16_t proxyport;
    char name[16];
    char proxy_address[40];
    uint8_t linkbutton;
    char ip_address[15];
    char netmask[15];
    char gateway_ip[15];
    uint8_t dhcp;
    uint8_t portal_services;
} hue_configuration_t;

hue_configuration_t* new_hue_configuration();
void free_hue_configuration(hue_configuration_t*);

typedef struct hue_bridge {
    char* id;
    char* ip;
    char* mac; // mac doesn't seem to be supported
    hue_configuration_t *config;
    hue_bulb_t *bulbs;
    UT_hash_handle hh;
} hue_bridge_t;

hue_bridge_t* new_hue_bridge();
void free_hue_bridge(hue_bridge_t*);

typedef struct response_buf
{
    char* buf;
    size_t buf_size;
} response_buf_t;

response_buf_t* new_response_buf();
void free_response_buf(response_buf_t*);

// portal api
hue_bridge_t* discover_local_bridges();

// lights api
hue_bulb_t* get_all_lights(hue_bridge_t*,hue_user_t*);
//hue_bulb_t* get_new_lights(hue_user_t*);
int set_light_attributes(hue_bridge_t* bridge, char* user, int bulb_id, char* name);
int set_light_state(hue_bridge_t* bridge, char* user, int bulb_id, char* name, char* value);

// configuration api
char* create_user(hue_bridge_t* bridge,char* username, char* device_type);
hue_configuration_t* get_configuration(hue_bridge_t* gateway, char* username);
hue_bulb_t* get_light_attributes(hue_bridge_t* bridge, hue_user_t* user, int bulb_id);
//int modify_configuration(hue_configuration_t* config);

#endif
