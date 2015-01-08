#include "libfuse.h"
#include <string.h>
#define STATUS_USAGE "./enfuse_status -u [username] -p [password] -i [location_id] -pid [panel_id]"

typedef struct status_args 
{
    char* user_name;
    char* password;
    int ping;
    int status;
    int version; 
} status_args_t;

status_args_t* new_status_args()
{
    status_args_t* args = (status_args_t*) malloc(sizeof(status_args_t));
    memset(args,0x0,sizeof(status_args_t));
    return args;
}

status_args_t* parse_status_args(int argc, char** args)
{
    if (argc != 6)
        return NULL;
    status_args_t *stat_args = new_status_args();
    int i;
    for (i = 0; i < argc; i++)
    {
        if (strcmp(args[i], "-p") == 0)
        {
            stat_args -> password = args[i+1];
            i++;
        } else if (strcmp(args[i], "-u") == 0)
        {
            stat_args -> user_name = args[i+1];
            i++;
        } else if (strcmp(args[i], "-ping") == 0)
        {
            stat_args -> ping = 1;
        } else if (strcmp(args[i], "-v") == 0)
        {
            stat_args -> version = 1;
        } else if (strcmp(args[i], "-s") == 0)
        {
            stat_args -> status = 1;
        }
        if ((stat_args -> password == NULL) ||
          (stat_args -> user_name == NULL))
        {
          free(stat_args);
          stat_args = NULL;
        }
    }
    return stat_args;
}

int main(int argc ,char** args)
{
    status_args_t* stat_args = parse_status_args(argc,args);
    if (args == NULL)
    {
        printf(STATUS_USAGE);
        return 1;
    }
    enfuse_session_t* session = enfuse_authenticate(stat_args -> user_name, stat_args -> password);

    if (session == NULL)
        printf("error in enfuse authentication\n");
    else
    {
        printf("SessionID: %s\nUserName: %s\n", session -> session_id, session -> username);
    }
    
    enfuse_status_t *status;
    if (stat_args -> ping)
        status = enfuse_ping(session);
    else if (stat_args -> version)
        status = enfuse_get_version(session);
    else if (stat_args -> status)
        status = enfuse_get_status(session);
    else 
    {
        printf(STATUS_USAGE);
        return 1;
    }
    
    printf("Version %s\nStarted At %s\nUser %s\nEmail %s\n", status -> version, status -> started_at, status -> user_name, status -> email);
    return 0;
}
