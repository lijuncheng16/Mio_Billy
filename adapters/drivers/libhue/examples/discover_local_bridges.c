#include "libhue.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#define DISCOVER_USAGE "./discover_local_bridges"


int main(int argc ,char** args)
{
    hue_bridge_t *bridges = discover_local_bridges();
    hue_bridge_t *bridge = bridges;
    curl_global_init(CURL_GLOBAL_ALL);
    while (bridge != NULL)
    {
        if (bridges -> mac != NULL)
            printf("ID %s\nIP %s\nMAC %s\n", bridges -> id, bridges -> ip, bridges -> mac); 
        else
            printf("ID %s\nIP %s\n", bridges -> id, bridges -> ip); 
        bridge = bridge -> hh.next;
    }
    curl_global_cleanup();
    return 0;
}
