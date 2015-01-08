#include "libfuse.h"
#include <string.h>
#include <curl/curl.h>
#define AUTHENTICATE_USAGE "./enfuse_authenticate -u [username] -p [password]\n"

typedef struct auth_args 
{
    char* user;
    char* password;
} auth_args_t;

auth_args_t* parse_authenticate_args(int argc, char** args)
{
    int i;
    auth_args_t *auth = (auth_args_t*) malloc(sizeof(auth_args_t));;
    memset(auth,0x0,sizeof(auth_args_t));

    for (i = 1; i < argc; i+=2)
    {
        if (strcmp(args[i], "-p") == 0)
            auth -> password = args[i+1];
         else if (strcmp(args[i], "-u") == 0)
            auth -> user = args[i+1];
    }
    if ((auth -> user == NULL) || (auth -> password == NULL))
    {
      free(auth);
      auth = NULL;
    }
    return auth;
}

int main(int argc ,char** args)
{
    auth_args_t* auth = parse_authenticate_args(argc,args);
    if (auth == NULL)
    {
        printf(AUTHENTICATE_USAGE);
        return 1;
    }
    curl_global_init(CURL_GLOBAL_ALL);
    enfuse_session_t* session = enfuse_authenticate(auth-> user, auth -> password);
    if (session == NULL)
    {
        printf("error in enfuse auth\n");
        curl_global_cleanup();
    } else
    {
        printf("SessionID: %s\nUserName: %s\n",session -> session_id, session -> username);

        curl_global_cleanup();
        release_enfuse_session(session);
    }
    return 0;
}
