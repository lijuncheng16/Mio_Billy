#include "libfuse.h"
#include <string.h>
#define PANEL_USAGE "./enfuse_get_panels -u [username] -p [password] -l [location_id] -pid [panel_id]"

typedef struct panel_args 
{
    char* user_name;
    char* password;
    int location_id;
    int panel_id;
} panel_args_t;

panel_args_t* new_panel_args()
{
    panel_args_t* args = (panel_args_t*) malloc(sizeof(panel_args_t));
    memset(args,0x0,sizeof(panel_args_t));
    return args;
}

panel_args_t* parse_panel_args(int argc, char** args)
{
    panel_args_t *pan_args = new_panel_args();
    int i;
    for (i = 1; i < argc; i++)
    {
        if (strcmp(args[i], "-p") == 0)
        {
            pan_args -> password = args[i+1];
            i++;
        } else if (strcmp(args[i], "-u") == 0)
        {
            pan_args -> user_name = args[i+1];
            i++;
        } else if (strcmp(args[i], "-l") == 0)
        {
            pan_args -> location_id = atoi(args[i+1]);
            i++;
        } else if (strcmp(args[i], "-pid") == 0)
        {
            pan_args -> panel_id = atoi(args[i+1]);
            i++;
        }
    }
    if ((pan_args -> password == NULL) ||
        (pan_args -> user_name == NULL) ||
        (pan_args -> location_id == 0) )
       
    {
      free(pan_args);
      pan_args = NULL;
    }
    return pan_args;
}

int main(int argc ,char** args)
{
    panel_args_t* pan_args = parse_panel_args(argc,args);
    if (args == NULL)
    {
        printf(PANEL_USAGE);
        return 1;
    }
    enfuse_session_t* session = enfuse_authenticate(pan_args -> user_name, pan_args -> password);

    if (session == NULL)
        printf("error in enfuse authentication\n");
    else
    {
        printf("SessionID: %s\nUserName: %s\n", session -> session_id, session -> username);
    }
    
    enfuse_panel_t *panels;
    if (pan_args -> location_id != 0)
        panels = enfuse_get_panels(session, pan_args -> location_id);
    else if (pan_args -> panel_id != 0)
        panels = enfuse_get_panel(session, pan_args -> panel_id);
    else 
    {
        printf(PANEL_USAGE);
        return 1;
    }
    enfuse_panel_t *panel = panels;
    int i = 0;
    for (; panel != NULL; panel = panel -> hh.next,i++)
    {
        printf("\nPanel ID %d\nLocation ID %d\nLabel %s\nName %s\n",
                panel -> panel_id, panel -> location_id, panel -> label,
                panel -> name);
    }

    printf("There are %d panels\n", i);
    
    return 0;
}
