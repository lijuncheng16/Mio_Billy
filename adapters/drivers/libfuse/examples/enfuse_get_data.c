#include "libfuse.h"
#include <string.h>
#define DATA_USAGE "./enfuse_get_data -u [username] -p [password] -l [location_id] -b [branch_id] -s [start_date] -e [end_date]"

typedef struct data_args 
{
    char* user_name;
    char* password;
    int location_id;
    char* start_date;
    char* end_date;
    int branch_id;
} data_args_t;

data_args_t* new_data_args()
{
    data_args_t* args = (data_args_t*) malloc(sizeof(data_args_t));
    memset(args,0x0,sizeof(data_args_t));
    return args;
}

data_args_t* parse_data_args(int argc, char** args)
{
    data_args_t *data_args = new_data_args();
    int i;
    for (i = 0; i < argc; i++)
    {
        if (strcmp(args[i], "-p") == 0)
        {
            data_args -> password = args[i+1];
            i++;
        } else if (strcmp(args[i], "-u") == 0)
        {
            data_args -> user_name = args[i+1];
            i++;
        } else if (strcmp(args[i], "-l") == 0)
        {
            data_args -> location_id = atoi(args[i+1]);
            i++;
        } else if (strcmp(args[i], "-s") == 0)
        {
            data_args -> start_date =(args[i+1]);
            i++;
        } else if (strcmp(args[i], "-e") == 0)
        {
            data_args -> end_date = (args[i+1]);
            i++;
        }  else if (strcmp(args[i], "-b") == 0)
        {
            data_args -> branch_id = atoi(args[i+1]);
            i++;
        }  
    }
    if ( (data_args -> password == NULL) || 
         (data_args -> user_name == NULL) ||
         (data_args -> location_id == 0) || 
         (data_args -> start_date == NULL))
    {
      free(data_args);
      data_args = NULL;
    }
    return data_args;
}

int main(int argc ,char** args)
{
    data_args_t* data_args = parse_data_args(argc,args);
    if (args == NULL)
    {
        printf(DATA_USAGE);
        return 1;
    }
    enfuse_session_t* session = enfuse_authenticate(data_args -> user_name, data_args -> password);

    if (session == NULL)
        printf("error in enfuse authentication\n");
    else
    {
        printf("SessionID: %s\nUserName: %s\n", session -> session_id, session -> username);
    }
    
    enfuse_data_t *data;
    if (data_args -> location_id != 0)
    {
        if (data_args -> start_date != NULL)
        {
            if (data_args -> end_date != NULL)
            { 
                if (data_args -> branch_id != 0)
                {
                    data = enfuse_get_branch_data(session, data_args -> location_id, data_args -> start_date,data_args -> end_date,
                        data_args -> branch_id);
                } else
                {
                    data = enfuse_get_data_range(session, data_args -> location_id, data_args -> start_date, data_args -> end_date);
                }
            } else
            {
                data = enfuse_get_data(session, data_args -> location_id, data_args -> start_date);
            }
        }else 
        {
            printf(DATA_USAGE);
            return 1;
        }
    }else
    {
        printf(DATA_USAGE);
        return 1;
    }
    int i = 0;
    for (; data != NULL; data = data -> hh.next)
    {
        if (data -> local_date_stamp == NULL)
            continue;
        i++;
        printf("\n\n%d\nBranch ID %d\nPHASE %d\nLocation ID %d\nLocal Date %s\nkw %f\nva %f\n\n",i,
                data -> branch_id, data -> phase, data -> location_id,data -> local_date_stamp,
                data -> kw, data -> va);
    }
    
    return 0;
}
