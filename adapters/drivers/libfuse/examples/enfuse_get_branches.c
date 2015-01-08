#include "libfuse.h"
#include <string.h>
#include <curl/curl.h>
#define BRANCH_USAGE "./enfuse_branches -u [username] -p [password] -i [location_id] -pid [panel_id] -b [branch_id]\n"

typedef struct branches_args 
{
    char* user_name;
    char* password;
    int location_id;
    int panel_id;
    int branch_id;
} branches_args_t;

branches_args_t* new_branches_args()
{
    branches_args_t* args = (branches_args_t*) malloc(sizeof(branches_args_t));
    memset(args,0x0,sizeof(branches_args_t));
    return args;
}

branches_args_t* parse_branches_args(int argc, char** args)
{
    branches_args_t *branch_args = new_branches_args();
    int i;
    for (i = 0; i < argc; i++)
    {
        if (strcmp(args[i], "-p") == 0)
        {
            branch_args -> password = args[i+1];
            i++;
        } else if (strcmp(args[i], "-u") == 0)
        {
            branch_args -> user_name = args[i+1];
            i++;
        } else if (strcmp(args[i], "-l") == 0)
        {
            branch_args -> location_id = atoi(args[i+1]);
            i++;
        } else if (strcmp(args[i], "-pid") == 0)
        {
            branch_args -> panel_id = atoi(args[i+1]);
            i++;
        } else if (strcmp(args[i], "-b") == 0)
        {
            branch_args -> branch_id = atoi(args[i+1]);
            i++;
        }
    }
    if (
        (branch_args -> password == NULL) ||
        (branch_args -> user_name == NULL) ||
        (branch_args -> location_id == 0))
    {
      free(branch_args);
      branch_args = NULL;
    }

    return branch_args;
}

int main(int argc ,char** args)
{
    branches_args_t* branch_args = parse_branches_args(argc,args);
    if (args == NULL)
    {
        printf(BRANCH_USAGE);
        free(branch_args);
        return 1;
    }
    curl_global_init(CURL_GLOBAL_ALL);
    enfuse_session_t* session = enfuse_authenticate(branch_args -> user_name, branch_args -> password);

    if (session == NULL)
        printf("error in enfuse authentication\n");
    else
    {
        printf("SessionID: %s\nUserName: %s\n", session -> session_id, session -> username);
    }
    
    enfuse_branch_t *branches;
    if (branch_args -> branch_id != 0)
        branches = enfuse_get_branch(session,branch_args -> branch_id);
    else if (branch_args -> panel_id != 0)
    {
        branches = enfuse_get_panel_branches(session, branch_args -> location_id,
                branch_args -> panel_id);
    }else if (branch_args -> location_id != 0)
        branches = enfuse_get_location_branches(session, branch_args -> location_id);
    else
    {
        printf(BRANCH_USAGE);
        free (branch_args);
        return 1;
    }
    enfuse_branch_t *branch = branches;
    int i = 0;
    for (; branch != NULL; branch = branch -> hh.next,i++)
    {
        printf("\n\nBranch ID %d\nPanel ID %d\nLabel %s\nName %s\n",
                branch -> branch_id, branch -> panel_id, branch -> label,
                branch -> name);
    }
    printf("There are %d branches\n",i);
    curl_global_cleanup();
    //release_enfuse_branch(branches);
    release_enfuse_session(session);
    free(branch_args); 
    return 0;
}
