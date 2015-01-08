#include "libfuse.h"
#include <string.h>
#include <curl/curl.h>
#define LOCATION_USAGE "./enfuse_get_locations -u [username] -p [password] -i [location_id] -n [location_name]"

typedef struct location_args 
{
    char* user_name;
    char* password;
    int location_id;
    char* location_name;
} location_args_t;

location_args_t* new_location_args()
{
    location_args_t* args = (location_args_t*) malloc(sizeof(location_args_t));
    memset(args,0x0,sizeof(location_args_t));
    return args;
}

location_args_t* parse_location_args(int argc, char** args)
{
    location_args_t *loc_args = new_location_args();
    int i;
    for (i = 0; i < argc; i++)
    {
        if (strcmp(args[i], "-p") == 0)
        {
            loc_args -> password = args[i+1];
            i++;
        } else if (strcmp(args[i], "-u") == 0)
        {
            loc_args -> user_name = args[i+1];
            i++;
        } else if (strcmp(args[i], "-i") == 0)
        {
            loc_args -> location_id = atoi(args[i+1]);
            i++;
        } else if (strcmp(args[i], "-n") == 0)
        {
            loc_args -> location_name = args[i+1];
            i++;
        }
    }

    if ( (loc_args -> password == NULL) ||
         (loc_args -> user_name == NULL))
    {
      free(loc_args);
      loc_args = NULL;
    }
    return loc_args;
}

int main(int argc ,char** args)
{
    location_args_t* loc_args = parse_location_args(argc,args);
    int err;
    enfuse_session_t *session;
    enfuse_location_t *locations;
    if (loc_args == NULL)
    {
        printf(LOCATION_USAGE);
        return 1;
    }
    err = curl_global_init(CURL_GLOBAL_ALL);
    if (err != CURLE_OK)
        return 1;
    session = enfuse_authenticate(loc_args -> user_name, loc_args -> password);
    if (session == NULL)
    {
        printf("error in enfuse authentication\n");
        curl_global_cleanup();
        free(loc_args);
        return 1;
    }else
    {
        printf("SessionID: %s\nUserName: %s\n", session -> session_id, session -> username);
    }
    
    if (loc_args -> location_id == 0)
        locations = enfuse_get_locations(session);
    else 
        locations = enfuse_get_location(session, loc_args -> location_id);
    
    if (locations == NULL)
         printf("Locations NULL\n");
    else 
    {
        printf("name %s\nid %d\nAddress %s %s %s \nCity %s\nState %s\nZipcode %s\nTimezone %s\nLatitude %f\nLongitude %f\n", 
        locations -> name, locations -> id, locations -> address1, locations -> address2,
        locations -> address3, locations -> city, locations -> state, 
        locations -> zipcode, locations -> timezoneID, 
        locations -> latitude, locations -> longitude);
        //release_enfuse_location(locations);
    }        
    curl_global_cleanup();
    release_enfuse_session(session);
    free(loc_args);
    return 0;
}
