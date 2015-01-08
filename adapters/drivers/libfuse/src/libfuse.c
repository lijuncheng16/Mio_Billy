#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <uthash.h>

#include <jsmn.h>
#include "libfuse.h"
#include <time.h>
#include "curl/curl.h"

// Local declearations

// Number of JSON tokens to begin initialize with 
#define N_TOKENS    512
#define SSID_LENGTH 512

// Max Length of user data in URL
#define USERDATA_LENGTH 512

// Authentication POST interface
#define AUTHENTICATE_URL        "https://enfuseapi.inscopeenergy.com/api/auth/credentials?UserName=%s&Password=%s"

// Branch information GET interface
#define GET_BRANCH_URL          "https://enfuseapi.inscopeenergy.com/api/branch/%d"
#define GET_BRANCH_LOCATION_URL "https://enfuseapi.inscopeenergy.com/api/branches/%d"
#define GET_BRANCH_LOCATION_PANEL_URL "https://enfuseapi.inscopeenergy.com/api/branches/%d/%d"

// Location information GET interface
#define GET_LOCATIONS_URL "https://enfuseapi.inscopeenergy.com/api/locations"
#define GET_LOCATION_URL "https://enfuseapi.inscopeenergy.com/api/location/%d"

// Panel information GET interface
#define GET_PANEL_URL "https://enfuseapi.inscopeenergy.com/api/panel/%d"
#define GET_PANELS_URL "https://enfuseapi.inscopeenergy.com/api/panels/%d"

// Presence query interface
#define PING_URL "https://enfuseapi.inscopeenergy.com/api/ping"

// Enfuse Status GET interface
#define GET_STATUS_URL "https://enfuseapi.inscopeenergy.com/api/status"
#define GET_VERSION_URL "https://enfuseapi.inscopeenergy.com/api/version"

// Usage data GET interface
#define GET_USAGE_DATA_START "https://enfuseapi.inscopeenergy.com/api/json/reply/GetUsageData?LocationId=%d&StartDate=%s"
#define GET_USAGE_DATA_START_END "https://enfuseapi.inscopeenergy.com/api/json/reply/GetUsageData?LocationId=%d&StartDate=%s&EndDate=%s"
#define GET_USAGE_DATA_BRANCH "https://enfuseapi.inscopeenergy.com/api/json/reply/GetUsageData?LocationId=2&StartDate=%s&EndDate=%s&BranchIds=%d"

// Enfuse Logs GET interface
#define GET_REQUEST_LOGS "https://enfuseapi.inscopeenergy.com/api/requestlogs"





// local CURL Interface

typedef struct _response_buf
{
    char* buffer;
    size_t buffer_size;
} _response_buf_t;

static _response_buf_t* _new_response_buf();
static void _release_response_buf(_response_buf_t* resp);
static int _receive_data(void *buffer, size_t size, size_t n_memb, void *userdata);
static _response_buf_t* _post_request(enfuse_session_t *session,char *json_url);


// local JSON Utilities
static char* _get_tok_string(char* json, jsmntok_t *tokens);

// Enfuse parsing
//static enfuse_branch_t* _branches_parse(_response_buf_t* response);
static enfuse_location_t* _location_parse(_response_buf_t* response);
static enfuse_panel_t* _panel_parse(_response_buf_t* response);
static enfuse_status_t* _status_parse(_response_buf_t* response);
static enfuse_data_t* _data_parse(_response_buf_t* response);


char* _string_copy( char* src)
{
    int src_len;
    char *dest;
    if (src == NULL) 
         return NULL;
    src_len = strlen(src);
    dest = malloc(src_len + 1);
    strcpy(dest,src);
    return dest;
}

// local struct management declarations
_response_buf_t* _new_response_buf()
{
    _response_buf_t *buf = (_response_buf_t*) malloc(sizeof(_response_buf_t));
    memset(buf,0x0,sizeof(_response_buf_t));
    return buf;
}

void _release_response_buf(_response_buf_t* resp)
{
    if (resp == NULL)
        return;
    if ( resp -> buffer != NULL)
        free(resp ->buffer);
    free(resp);
}

enfuse_session_t* new_enfuse_session()
{
    enfuse_session_t* session = (enfuse_session_t*) malloc(sizeof(enfuse_session_t));
    memset(session,0x0,sizeof(enfuse_session_t));
    return session;
}

void release_enfuse_session(enfuse_session_t* info)
{
    if (info == NULL)
        return;
    if (info -> session_id != NULL)
        free(info -> session_id);
    if (info -> username != NULL)
        free(info -> username);
    if (info -> url != NULL)
        free(info -> url);
    if (info -> session_id != NULL)
        free(info -> session_id);
    free(info);
}

enfuse_response_status_t* new_enfuse_response_status()
{
    enfuse_response_status_t* status = 
        (enfuse_response_status_t*) malloc(sizeof(enfuse_response_status_t));
    memset(status,0x0,sizeof(enfuse_response_status_t));
    return status;
}
void release_enfuse_response_status(enfuse_response_status_t *status)
{
     enfuse_response_status_t *tmp_status;
     while (status)
     {
         if (status == NULL)
             return; 
         if (status -> error_code)
             free(status -> error_code);
         if (status -> message)
             free(status -> message);
         if (status -> stack_trace)
             free(status -> stack_trace);
         if (status -> errors)
             release_enfuse_error(status -> errors);
      
         tmp_status = status -> hh.next;
         free(status);
         status = tmp_status;
     }
}

enfuse_error_t* new_enfuse_error(){
    enfuse_error_t* error = (enfuse_error_t*) malloc(sizeof(enfuse_error_t));
    memset(error,0x0,sizeof(enfuse_error_t));
    return error;
}

void release_enfuse_error(enfuse_error_t *error)
{
    enfuse_error_t *error_tmp;
    while( error) 
    {
        if(error -> error_code)
            free(error -> error_code);
        if (error -> field_name)
            free(error -> field_name);
        if (error -> message) 
            free(error -> message);
        error_tmp = error -> hh.next;
        free(error);
        error = error_tmp; 
    }
}

enfuse_branch_t* new_enfuse_branch()
{
    enfuse_branch_t* branch = (enfuse_branch_t*) malloc(sizeof(enfuse_branch_t));
    memset(branch,0x0,sizeof(enfuse_branch_t));
    return branch;
}

void release_enfuse_branch(enfuse_branch_t *branch)
{
    enfuse_branch_t *branch_tmp;
    while (branch)
    {
         if (branch -> label)
            free(branch -> label);
         if (branch -> name)
            free(branch -> name);
         if (branch -> node_id)
            free(branch -> node_id);
         branch_tmp = branch -> hh.next;
         free(branch);
         branch = branch_tmp;
    }
}
enfuse_panel_t* new_enfuse_panel()
{
    enfuse_panel_t*  panel= (enfuse_panel_t*) malloc(sizeof(enfuse_panel_t));
    memset(panel,0x0,sizeof(enfuse_panel_t));
    return panel;
}

enfuse_location_t* new_enfuse_location()
{
    enfuse_location_t*  location = (enfuse_location_t*) malloc(sizeof(enfuse_location_t));
    memset(location,0x0,sizeof(enfuse_location_t));
    return location;
}

void release_enfuse_location(enfuse_location_t *location)
{
    enfuse_location_t *location_tmp;
    while (location)
    {
       if (location -> name) 
          free(location -> name);  
       if (location -> address1)
          free(location -> address1);
       if (location -> address2)
          free(location -> address2);
       if (location -> address3)
          free(location -> address3);
       if (location -> city)
          free(location -> city);
       if (location -> state)
          free(location -> state);
       if (location -> zipcode)
          free(location -> zipcode);
       if (location -> timezoneID)
          free(location -> timezoneID);
       location_tmp = location -> hh.next;
       free(location);
       location = location_tmp;
    }
}

enfuse_data_t* new_enfuse_data()
{
    enfuse_data_t*  data = (enfuse_data_t*) malloc(sizeof(enfuse_data_t));
    memset(data,0x0,sizeof(enfuse_data_t));
    return data;
}

void release_enfuse_data(enfuse_data_t *data)
{
    enfuse_data_t *data_tmp;
    while (data)
    {
        if (data -> local_date_stamp)
            free(data -> local_date_stamp); 
        if (data -> utc_date_stamp)
            free(data -> utc_date_stamp);
        data_tmp = data -> hh.next;
        free(data);
        data = data_tmp;
     }
}

    
enfuse_status_t* new_enfuse_status()
{
    enfuse_status_t*  status = (enfuse_status_t*) malloc(sizeof(enfuse_status_t));
    memset(status,0x0,sizeof(enfuse_status_t));
    return status;
}

// curl JSON Utility Functions

// Data receive function used by for curl requests
static int _receive_data(void *buffer, size_t size, size_t nmemb, void *userdata)
{
    _response_buf_t *buf = ((_response_buf_t*)userdata);
    size_t realsize = size * nmemb;
    printf("%s\n",(char*)buffer);
    buf ->buffer = realloc(buf->buffer, buf->buffer_size + realsize + 1);
    if(buf->buffer == NULL) {
        /* out of memory! */ 
        return 0;
    }
    // add to response buffer     
    memcpy(&(buf->buffer[buf->buffer_size]), buffer, realsize);
    buf->buffer_size += realsize;
    buf->buffer[buf->buffer_size] = 0;
    return realsize;
}

// Performs a post request to an enfuse session using a specific url
_response_buf_t* _post_request(enfuse_session_t* session, char* json_url)
{
    CURL *curl;
    CURLcode curl_status;
    _response_buf_t* response = NULL;
    struct curl_slist *headers = NULL;
    char buffer[SSID_LENGTH];
    int err;
    curl = curl_easy_init();
    if (curl != NULL) {
        // set the command path and url
        curl_easy_setopt(curl, CURLOPT_URL, json_url);

        headers = curl_slist_append(headers, "Accept: application/json");
        if (headers == NULL)
        {
            return NULL;
        }
        // set headers
        if (session != NULL) 
        {
            memset(buffer,0x0,SSID_LENGTH);
            err = snprintf(buffer,SSID_LENGTH,"Cookie: ss-id=%s", session -> session_id);
            if (err < 0)
            {
                fprintf(stderr,"snprintf failed with %d\n",err);
                return NULL;
            }
            if ((headers = curl_slist_append(headers,buffer)) == NULL)
            {
                fprintf(stderr,"curl_slist_append failed\n");
                return NULL;
            }
            
        }

        if ((err = curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers)) 
            != CURLE_OK)
        {
            fprintf(stderr,"Curl coud no set http Headers\n");
            return NULL;
        }

        // set response_handlers
        response = _new_response_buf();

        // install write function
        //curl_setopt(curl, CURLOPT_FRESH_CONNECT, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _receive_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
        curl_easy_setopt(curl, CURLOPT_VERBOSE,1);
        curl_status = curl_easy_perform(curl);

        curl_easy_reset(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        if (curl_status != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() returned %d: %s\n",
                    curl_status,curl_easy_strerror(curl_status));
            _release_response_buf(response);
            return NULL;
        }
    } else {
        fprintf(stderr, "Could not perform curl_easy_setup\n");
        return NULL;
    }
    return response;
}


// JSON Portal API


// Json parser state enumeration
typedef enum { START, ARRAY_OBJ, KEY, VALUE, SKIP, STOP } _parse_state;

// tokenizes json text into jsmntok_t structure
jsmntok_t* _json_tokenize(char* json)
{
    int jsmn_result, n_tokens = N_TOKENS;
    jsmn_parser parser;
    jsmntok_t *tokens;
    if ((tokens = malloc(n_tokens*sizeof(jsmntok_t))) == NULL)
    {
        fprintf(stderr,"Could not allocate tokens\n");
        return NULL;
    }
    jsmn_init(&parser);
    jsmn_result = jsmn_parse(&parser,json,tokens,n_tokens);

    while (jsmn_result == JSMN_ERROR_NOMEM)
    {
        n_tokens = n_tokens*2;
        tokens = realloc(tokens,sizeof(jsmntok_t)*n_tokens);
        if (tokens == NULL)
        {
           fprintf(stderr, "could not realloc tokens\n");
           return NULL;
        }
        jsmn_result = jsmn_parse(&parser,json,tokens,n_tokens);
    }
    if (jsmn_result < 0 )
    {
         fprintf(stderr, "jsmn_parse failed %d\n", jsmn_result);
         if (tokens != NULL)
             free(tokens);
         return NULL;
    }
    return tokens;
}

// returns the section of the string that jsmnotok_t refers to
char* _get_tok_string(char* json, jsmntok_t *token)
{
    int length;
    char* keyString;    
    if (token == NULL)
        return NULL;
    length = token->end - token->start;
    if (length < 0)
    {
        fprintf(stderr,"invalid jsmnotk_t\n");
        return NULL;
    }
    keyString = malloc(length + 1);
    if (keyString == NULL)
    {
        fprintf(stderr, "could not allocate string\n");
        return NULL;
    }
    memcpy(keyString, &json[token->start], length);
    keyString[length] = '\0';
    return keyString;
}

// strcmps the two strings 
int _tok_strcmp(char* json, jsmntok_t *tok, char* str)
{
    int str_len;
    int tok_len;
    if ( (json == NULL) || (tok == NULL) || (str == NULL))
       return 1;
    str_len = strlen(str);
    tok_len = tok -> end - tok -> start;
    if (str_len == (size_t) tok_len)
    { 
        return 1;
    }

    if (tok_len > 0)
        return (strncmp(json + tok->start, str, tok->end - tok->start) == 0);
    else 
        return 1;
}

/*enfuse_branch_t* _branches_parse(_response_buf_t* response);
enfuse_status_t* _status_parse(_response_buf_t* response);
enfuse_data_t* _data_parse(_response_buf_t* response);*/

// Parses the Authenticate response from enfuse server returning session information
enfuse_session_t* _authenticate_parse(_response_buf_t* response)
{
    if ((response == NULL) || (response -> buffer == NULL))
    {
        printf("response is NULL\n");
        return NULL;
    }

    int sub_tokens, state = START;
    char *key,*val;
    enfuse_session_t *session;
    jsmntok_t *t, *tokens; // a number >= total number of tokens
    int tok, j; 
    if ( (session = new_enfuse_session()) == NULL)
    {
        return NULL;
    }

    if((tokens = _json_tokenize(response->buffer)) == NULL)
        return NULL;
      
    for (tok = 0, j = 1; j > 0; tok++, j--)
    {
        t = &tokens[tok];
 
        if (t == NULL) 
            continue;

        if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
            j += t->size;

        switch (state)
        {
            case START:
                sub_tokens = t -> size;
                if (sub_tokens == 0)
                    state = STOP;
                else 
                    state = KEY;
                break;
            case KEY:
                sub_tokens--;
                if (t -> type != JSMN_STRING)
                {
                     state = STOP;
                     break;
                }
                state = VALUE;
                key = _get_tok_string(response -> buffer, t);
                break;
            case VALUE:
                sub_tokens--;

                if ( (val = _get_tok_string(response->buffer,t)) == NULL)
                { 
                     state = KEY;
                     if (key)
                         free(key);
                     break;
                }
               
                if (strcmp(key,"sessionId") == 0)
                {
                    session -> session_id = val;
                } else {
                   if (val)
                       free(val);
                 }
                 if (key != NULL)
                    free(key);
                if (sub_tokens == 0)
                    state = STOP;
                else 
                    state = KEY;
                break; 
            
            case STOP:
                break;
        }
    }
    if ((session != NULL) && (session -> session_id == NULL))
    {
          release_enfuse_session(session);
          session = NULL;
    }
    free(tokens);
    return session;
}

// enfuse_authenticate authenticates a username with a password return
// must call global_init prior to use
//  session key information
enfuse_session_t* enfuse_authenticate(char* username, char* password)
{
    _response_buf_t *response;
    enfuse_session_t *session;
    int url_len;
    char *auth_url; 
    
    if ((username == NULL) || (password == NULL))
        return NULL;
    url_len = strlen(AUTHENTICATE_URL) + USERDATA_LENGTH;

    // create url for authentication
    if ((auth_url = malloc(url_len + 1)) == NULL)
         return NULL;
    snprintf(auth_url, url_len, AUTHENTICATE_URL, username, password);
   
    response = _post_request(NULL, auth_url);

    if (response == NULL)
    {
        fprintf(stderr, "response to authenticate request null\n");
        free(auth_url);
        return NULL;    
    }
    session = _authenticate_parse(response);

    if (session != NULL)
    {
        session -> username = _string_copy(username);
        session -> password = _string_copy(password);
    }

    _release_response_buf(response);
    free(auth_url);
    return session;
}

// Parses the json response message into a enfuse_location struct
enfuse_location_t* _location_parse(_response_buf_t* response)
{

    int sub_tokens = 0, state = START;
    char *key,*val;
    enfuse_location_t *location;
    enfuse_location_t *tmp_location;
    jsmntok_t *t, *tokens; // a number >= total number of tokens
    int tok,j; 

    if ((response == NULL) || (response -> buffer == NULL))
        return NULL;

    tokens  = _json_tokenize(response->buffer);
    for (tok = 0, j = 1; j > 0; tok++, j--)
    {
        t = &tokens[tok];
        if (t == NULL)
            continue;

        if ((t -> type == JSMN_ARRAY) || (t -> type == JSMN_OBJECT))
        {
        	j += t->size;
                sub_tokens += t-> size;
        }

        switch (state)
        {
            case START:
                if (sub_tokens == 0)
                    state = STOP;
                else 
                    state = KEY;
                break;
            case KEY:
                    sub_tokens--;
                if (t -> type == JSMN_OBJECT)
                {
                    state = KEY;
                    tmp_location = new_enfuse_location();
                    location -> hh.next = tmp_location;
                    location = tmp_location;
                }else if (t -> type == JSMN_ARRAY)
                {
                    state = ARRAY_OBJ;
                } else 
                {
                    key = _get_tok_string(response -> buffer, t);
                    state = VALUE;
                }
                break;
            case ARRAY_OBJ:
                sub_tokens--;
                state = KEY;
                location = new_enfuse_location();
                break; 
            case VALUE:
                sub_tokens--;
                val = _get_tok_string(response->buffer,t);
                if (key == NULL)
                   free(val);
                else if (strcmp(key,"name") == 0)
                {
                  location -> name = val;

                } else if (strcmp(key,"id") == 0)
                {
                    location -> id = atoi(val);
                    free(val);
                } else if (strcmp(key,"address1") == 0)
                {
                    location -> address1 = val;
                } else if (strcmp(key,"address2") == 0)
                {
                    location -> address2 = val;
                } else if (strcmp(key,"address3") == 0)
                {
                    location -> address3 = val;
                } else if (strcmp(key,"city") == 0)
                {
                    location -> city = val;
                } else if (strcmp(key,"state") == 0)
                {
                    location -> city = val;
                } else if (strcmp(key,"zipCode") == 0)
                {
                    location -> zipcode = val;
                } else if (strcmp(key,"timeZoneId") == 0)
                {
                    location -> timezoneID = val;
                } else if (strcmp(key,"latitude") == 0)
                {
                    location -> latitude = atof(val);
                    free(val);
                } else if (strcmp(key,"longitude") == 0)
                {
                    location -> longitude = atof(val);
                    free(val);
                } else 
                    free(val);
                
                free(key);

                if (sub_tokens == 0)
                    state = STOP;
 		else if (t -> type == JSMN_OBJECT)
                    state = KEY;
 		else if (t -> type == JSMN_ARRAY)
                    state = ARRAY_OBJ;
                else 
                    state = KEY;
                break; 
            
            case STOP:
                break;
           
        }
    }
    free(tokens);
    return location;
}

// Gets information about a speccific location through a previously authenticated session
enfuse_location_t* enfuse_get_location(enfuse_session_t* session, int location_id)
{
    _response_buf_t *response;
    enfuse_location_t *location = NULL;
    int url_size = strlen(GET_LOCATION_URL) + USERDATA_LENGTH;
    char* buffer = malloc(url_size+1);
    if (buffer == NULL)
    { 
        return NULL;
    } 
    memset(buffer,0x0,url_size);
    if (snprintf(buffer,url_size,GET_LOCATION_URL,location_id) < 0)
    {
        free(buffer);
        return NULL;
    }
    response = _post_request(session,buffer);
    location = _location_parse(response);
    free(buffer);
    _release_response_buf(response);
    return location;
}

// Gets all locations associated with user through a previously authenticated session
enfuse_location_t* enfuse_get_locations(enfuse_session_t* session)
{
    _response_buf_t *response;
    enfuse_location_t *location = NULL;
    int url_len = strlen(GET_LOCATIONS_URL) + USERDATA_LENGTH;
    char* location_url = (char*) malloc(url_len+1);
    if (location_url == NULL)
        return NULL;
    if (snprintf(location_url,url_len,GET_LOCATIONS_URL) < 0)
    {
       free(location_url);
       return NULL;
    }
    response = _post_request(session,location_url);
    location = _location_parse(response);
    free(location_url);
    _release_response_buf(response);
    return location;
}


// Parses the JSON response from a get Branch information request
enfuse_branch_t* _branch_parse(_response_buf_t* response){
    int sub_tokens = 0, state = START;
    char *key,*val;
    enfuse_branch_t *tmp_branch = NULL;
    enfuse_branch_t *branch = NULL;
    jsmntok_t *t, *tokens;

    if ((response == NULL) || (response -> buffer == NULL))
        return NULL;
    tokens = _json_tokenize(response->buffer); // a number >= total number of tokens
    if (tokens == NULL)
        return NULL;
    int tok,j;
    for (tok = 0, j = 1; j > 0; tok++, j--)
    {
        t = &tokens[tok];
        if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
        {
            j += t->size;
            sub_tokens += t->size;
        }
        switch (state)
        {
            case START:
                state = KEY;
                if (sub_tokens == 0)
                    state = STOP;
                break;
            case KEY:
                sub_tokens--;
                if (t -> type == JSMN_OBJECT)
                {
                    state = KEY;
                    tmp_branch = new_enfuse_branch();
                    tmp_branch -> hh.next = branch;
                    branch = tmp_branch;
                } else
                {
                    state = VALUE;
                    key = _get_tok_string(response -> buffer, t);
                }
                break;
            case ARRAY_OBJ:
                if (branch == NULL)
                 	branch = new_enfuse_branch();
                else 
                {
                    tmp_branch = new_enfuse_branch();
                    branch -> hh.next = tmp_branch;
                    branch = tmp_branch;
                }
                state = KEY;
                sub_tokens--;
                break; 
            case VALUE:
                sub_tokens--;
               
                if ((val = _get_tok_string(response->buffer,t)) == NULL){}
                else if (key == NULL)
                    free(val);
                else if (strcmp(key,"branchId") == 0)
                {
                    branch -> branch_id = atoi(val);
                    free(val);
                } else if (strcmp(key,"panelId") == 0)
                {
                    branch -> panel_id = atoi(val);
                } else if (strcmp(key,"label") == 0)
                {
                    branch -> label = val;
                } else if (strcmp(key,"name") == 0)
                {
                    branch -> name = val;
                } else if (strcmp(key,"ratedAmps") == 0)
                {
                    branch -> rated_amps = atoi(val);
                    free(val);
                } else if (strcmp(key,"breakerPosition") == 0)
                {
                    branch -> breaker_position = atoi(val);
                    free(val);
                } else if (strcmp(key,"numPhases") == 0)
                {
                    branch -> n_phases = atoi(val);
                    free(val);
                } else 
                    free(val);
                
                if (key)
                    free(key);
           
                if (sub_tokens == 0)
                    state = STOP;
                else if (t -> type == JSMN_ARRAY)
                {
                    state = ARRAY_OBJ;
                } else 
                    state = KEY;
                break; 
            
            case STOP:
                break;
           
        }
    }
    free(tokens);
    return branch;
}


enfuse_branch_t* enfuse_get_branch(enfuse_session_t* session, int branch_id)
{
    _response_buf_t *response;
    enfuse_branch_t *branches = NULL;
    int url_len = strlen(GET_BRANCH_URL) + USERDATA_LENGTH;
    char* branches_url = (char*) malloc(url_len);
    if (branches_url == NULL)
        return NULL;
    memset(branches_url,0x0,url_len);
    if ( snprintf(branches_url, url_len,GET_BRANCH_URL,branch_id) < 0)
    {
        free(branches_url);
        return NULL;
    }
    response = _post_request(session,branches_url);
    branches = _branch_parse(response);
    free(branches_url);
    _release_response_buf(response);
    return branches;
}

enfuse_branch_t* enfuse_get_location_branches(enfuse_session_t* session, int location_id)
{
    _response_buf_t *response;
    enfuse_branch_t *branches = NULL;
    int url_len = strlen(GET_BRANCH_LOCATION_URL) + USERDATA_LENGTH;
    char* branches_url = (char*) malloc(url_len);
    if (branches_url == NULL)
        return NULL;
    memset(branches_url,0x0,url_len);
    if (snprintf(branches_url,url_len,GET_BRANCH_LOCATION_URL, location_id) < 0)
    {
        free(branches_url);
        return NULL;
    }
    response = _post_request(session,branches_url);
    branches = _branch_parse(response);
    free(branches_url);
    _release_response_buf(response);
    return branches;
}

enfuse_branch_t* enfuse_get_panel_branches(enfuse_session_t* session, int location_id, int panel_id)
{
    _response_buf_t *response;
    enfuse_branch_t *branches = NULL;
    int url_len = strlen(GET_BRANCH_LOCATION_PANEL_URL) + USERDATA_LENGTH;
    char* branches_url = (char*) malloc(url_len);
    if (branches_url == NULL)
        return NULL;
    memset(branches_url,0x0,url_len);
    if (snprintf(branches_url, url_len,GET_BRANCH_LOCATION_PANEL_URL, location_id,panel_id) < 0)
    {
        free(branches_url);
        return NULL;
    }
    response = _post_request(session,branches_url);
    branches = _branch_parse(response);
    _release_response_buf(response);
    return branches;
}

enfuse_panel_t* _panel_parse(_response_buf_t* response){
    int sub_tokens = 0, state = START;
    char *key,*val;
    enfuse_panel_t *tmp_panel = NULL;
    enfuse_panel_t *panel = NULL;
    jsmntok_t *t;
    jsmntok_t *tokens;
    int tok, j;

    if ((response == NULL) || (response -> buffer == NULL))
        return NULL;

    tokens = _json_tokenize(response->buffer); // a number >= total number of tokens
    if (tokens == NULL) 
        return NULL;
    for (tok = 0, j = 1; j > 0; tok++, j--)
    {
        t = &tokens[tok];
        if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
        {
            j += t->size;
            sub_tokens += t->size;
        }
        switch (state)
        {
            case START:
                if (t -> type != JSMN_OBJECT)
                    printf("error\n");
                state = KEY;
                if (sub_tokens == 0)
                    state = STOP;
                if (sub_tokens % 2 != 0)
                    printf("error\n");
                break;
            case KEY:
                sub_tokens--;
                if (t -> type == JSMN_OBJECT)
                {
                    state = KEY;
                    tmp_panel = new_enfuse_panel();
                    tmp_panel -> hh.next = panel;
                    panel = tmp_panel;
                    break;
                }
                if (t -> type != JSMN_STRING)
                {
                    printf("herror\n");
                }
                state = VALUE;
                key = _get_tok_string(response -> buffer, t);
                break;
            case ARRAY_OBJ:
                state = KEY;
                    tmp_panel = new_enfuse_panel();
                    tmp_panel -> hh.next = panel;
                    panel = tmp_panel;
                sub_tokens--;
                break; 
            case VALUE:
                sub_tokens--;
               
                val = _get_tok_string(response->buffer,t);
                if (val == NULL)
                { //fall through
                } else if (key == NULL)
                { 
                   free(val);
                } else if (strcmp(key,"panelId") == 0)
                {
                    panel -> panel_id = atoi(val);
                    free(val);
                } else if (strcmp(key,"locationId") == 0)
                {
                    panel -> location_id = atoi(val);
                } else if (strcmp(key,"label") == 0)
                {
                    panel -> label = val;
                } else if (strcmp(key,"name") == 0)
                {
                    panel -> name = val;
                } else 
                    free(val);
                
                if (key != NULL) 
                    free(key);
                if (sub_tokens == 0)
                    state = STOP;
                else if (t -> type == JSMN_ARRAY)
                {
                    state = ARRAY_OBJ;
                } else 
                    state = KEY;
                break; 
            
            case STOP:
                break;
           
        }
    }
    free(tokens);
    return panel;
}


enfuse_panel_t* enfuse_get_panel(enfuse_session_t* session, int panel_id)
{
    _response_buf_t *response;
    enfuse_panel_t *panel = NULL;
    int url_len = sizeof(GET_PANEL_URL) + USERDATA_LENGTH;
    char* panel_url = (char*) malloc(url_len);
    if (panel_url == NULL)
        return NULL;
    memset(panel_url, 0x0, url_len);
    if(sprintf(panel_url,GET_PANEL_URL,panel_id) < 0)
        return NULL;
    response = _post_request(session,panel_url);
    panel = _panel_parse(response);
    if (response != NULL)
    { 
       free(response -> buffer);
       free(response);
    }
    return panel;
}
enfuse_panel_t* enfuse_get_panels(enfuse_session_t* session, int location_id)
{
    _response_buf_t *response;
    enfuse_panel_t *panel = NULL;
    int url_len = strlen(GET_PANELS_URL) + USERDATA_LENGTH;
    char* panel_url = (char*) malloc(url_len);
    if (panel_url == NULL)
        return NULL;
    if (sprintf(panel_url,GET_PANELS_URL, location_id) < 0)
        return NULL;
    response = _post_request(session,panel_url);
    panel = _panel_parse(response);
    _release_response_buf(response);
    return panel;
}

enfuse_status_t* _status_parse(_response_buf_t* response){
    int sub_tokens = 0, state = START;
    char *key,*val;
    enfuse_status_t *tmp_status  = NULL;
    enfuse_status_t *status = NULL;
    jsmntok_t *t, *tokens; // a number >= total number of tokens
    int tok,j;

    if ((response == NULL) || (response -> buffer == NULL))
        return NULL;
    tokens = _json_tokenize(response->buffer);
    for (tok = 0, j = 1; j > 0; tok++, j--)
    {
        t = &tokens[tok];
        if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
        {
            j += t->size;
            sub_tokens += t->size;
        }
        switch (state)
        {
            case START:
                state = KEY;
                if (sub_tokens == 0)
                    state = STOP;
                break;
            case KEY:
                sub_tokens--;
                if (t -> type == JSMN_OBJECT)
                {
                    state = KEY;
                    tmp_status = new_enfuse_status();
                    tmp_status -> hh.next = status;
                    status = tmp_status;
                } else
                {
                
                state = VALUE;
                key = _get_tok_string(response -> buffer, t);
                }
                break;
            case ARRAY_OBJ:
                state = KEY;
                tmp_status = new_enfuse_status();
                    tmp_status -> hh.next = status;
                    status = tmp_status;

                sub_tokens--;
                break; 
            case VALUE:
                sub_tokens--;
               
                val = _get_tok_string(response->buffer,t);
                if (val == NULL) {}
                else if (key == NULL)
                {
                    free(val);
                } if (strcmp(key,"version") == 0)
                {
                    status -> version = val;
                } else if (strcmp(key,"startedAt") == 0)
                {
                    status -> started_at = (val);
                } else if (strcmp(key,"sessionId") == 0)
                {
                    status -> session_id = val;
                } else if (strcmp(key,"createdAt") == 0)
                {
                    status -> created_at = val;
                } else if (strcmp(key,"userName") == 0)
                {
                    status -> user_name = val;
                } else if (strcmp(key,"customer") == 0)
                {
                    status -> customer = val;
                } else if (strcmp(key,"email") == 0)
                {
                    status -> email = val;
                } else if (strcmp(key,"firstName") == 0)
                {
                    status -> first_name = val;
                } else if (strcmp(key,"lastName") == 0)
                {
                    status -> last_name = val;
                } else if (strcmp(key,"email") == 0)
                {
                    status -> email = val;
                } else if (strcmp(key,"isAdmin") == 0)
                {
                    status -> is_admin = val;
                }
                else 
                    free(val);
                
                if (key != NULL)
                   free(key);

                if (sub_tokens == 0)
                    state = STOP;
                else if (t -> type == JSMN_ARRAY)
                {
                    state = ARRAY_OBJ;
                } else 
                    state = KEY;
                break; 
            
            case STOP:
                break;
           
        }
    }
    free(tokens);
    return status;
}
enfuse_status_t* enfuse_get_status(enfuse_session_t* session)
{
    _response_buf_t *response;
    enfuse_status_t *status = NULL;
    char stat_url[] = GET_STATUS_URL;
    response = _post_request(session,stat_url);
    status = _status_parse(response);
    _release_response_buf(response);
    return status;
}

enfuse_status_t* enfuse_get_version(enfuse_session_t* session)
{
    _response_buf_t *response;
    enfuse_status_t *status = NULL;
    char stat_url[] = GET_VERSION_URL;
    response = _post_request(session,stat_url);
    status = _status_parse(response);
    _release_response_buf(response);
    return status;
}

enfuse_status_t* enfuse_ping(enfuse_session_t* session)
{
    _response_buf_t *response;
    enfuse_status_t *status = NULL;
    char stat_url[] = PING_URL;
    response = _post_request(session,stat_url);
    status = _status_parse(response);
    _release_response_buf(response);

    return status;
}

enfuse_data_t* _data_parse(_response_buf_t* response)
{
    int sub_tokens = 0, state = START;
    char *key,*val;
    enfuse_data_t *tmp_data = NULL;
    enfuse_data_t *data = new_enfuse_data();
    jsmntok_t *t, *tokens; 
    int tok,j;

    if ((response == NULL) || (response -> buffer == NULL))
        return NULL;
    if ((tokens = _json_tokenize(response->buffer)) == NULL)
        return NULL;

    for (tok = 0, j = 1; j > 0; tok++, j--)
    {
        t = &tokens[tok];
        if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
        {
            j += t->size;
            sub_tokens += t->size;
        }
        switch (state)
        {
            case START:
                if (t -> type != JSMN_OBJECT)
                    printf("herror\n");
                state = ARRAY_OBJ;
                if (sub_tokens == 0)
                    state = STOP;
                break;
            case KEY:
                sub_tokens--;
                if (t -> type == JSMN_OBJECT)
                {
                    state = KEY;
                    tmp_data = new_enfuse_data();
                    tmp_data -> hh.next = data;
                    data = tmp_data;
                } else if (t -> type == JSMN_ARRAY) 
                {
                    state = ARRAY_OBJ;
                } else 
                {
                  state = VALUE;
                  key = _get_tok_string(response -> buffer, t);
                  if (strcmp(key,"[]") == 0)
                      return NULL; 
                }
                break;
            case ARRAY_OBJ:
                state = KEY;
                sub_tokens--;
                break; 
            case VALUE:
                sub_tokens--;
                val = _get_tok_string(response->buffer,t);
                if (val == NULL) {}
                else if (key == NULL) 
                {
                    free(val);
                } else if (strcmp(key,"branchId\0") == 0)
                {
                    data -> branch_id = atoi(val);
                    free(val);
                } else if (strcmp(key,"phase\0") == 0)
                {
                    data -> phase = atoi(val);
                    free(val);
                } else if (strcmp(key,"locationId\0") == 0)
                {
                    data -> location_id = atoi(val);
                    free(val);
                } else if (strcmp(key,"dateStampLocal\0") == 0)
                {
                    data -> local_date_stamp = val;
                } else if (strcmp(key,"dateStampUTC\0") == 0)
                {
                    data -> utc_date_stamp = (val);
                } else if (strcmp(key,"kw") == 0)
                {
                    data -> kw = atof(val);
                    free(val);
                } else if (strcmp(key,"va") == 0)
                {
                    data -> va = atof(val);
                    free(val);
                } else if (strcmp(key,"wattMinute") == 0)
                {
                    data -> watt_minute = atof(val)/100.0;
                    free(val);
                } else if (strcmp(key,"vaMinute") == 0)
                {
                    data -> va_minute = atoi(val);
                    free(val);
                } else if (strcmp(key,"minCurrent") == 0)
                {
                    data -> min_current = atof(val)/100.0;
                    free(val);
                } else if (strcmp(key,"maxCurrent") == 0)
                {
                    data -> max_current = atoi(val)/100.0;
                    free(val);
                } else if (strcmp(key,"minVoltage") == 0)
                {
                    data -> min_voltage = atof(val)/100.0;
                    free(val);
                } else if (strcmp(key,"maxVoltage") == 0)
                {
                    data -> max_voltage = atoi(val)/100.0;
                    free(val);
                } 
                
                if (key)
                   free(key);

                if (sub_tokens == 0)
                    state = STOP;
                else if (t -> type == JSMN_ARRAY)
                {
                    state = ARRAY_OBJ;
                } else 
                    state = KEY;
                break; 
            
            case STOP:
                break;
           
        }
    }
    free(tokens);
    return data;
}

enfuse_data_t* enfuse_get_data(enfuse_session_t* session, 
    int location_id, char* start_date)
{
    _response_buf_t *response;
    enfuse_data_t *data;
    int url_len = strlen(GET_USAGE_DATA_START) + USERDATA_LENGTH;
    char * url = (char*) malloc(url_len); 
    if (snprintf(url,url_len,GET_USAGE_DATA_START,location_id,start_date) < 0)
    {
        free(url);
        return NULL;
    }
    response=_post_request(session,url);
    data = _data_parse(response);
    _release_response_buf(response);
    free(url);
    return data;    
}

enfuse_data_t* enfuse_get_data_range(enfuse_session_t* session, 
    int location_id, char* start_date, char* end_date)
{
    _response_buf_t *response;
    enfuse_data_t *data;
    int url_len = strlen(GET_USAGE_DATA_START_END) + USERDATA_LENGTH;
    char * url = (char*) malloc(url_len); 
    memset(url,0x0,url_len);
    if (snprintf(url,url_len,GET_USAGE_DATA_START_END,location_id,start_date,end_date) < 0)
    {
         free(url);    
         return NULL;
    }
    response=_post_request(session,url);
    data = _data_parse(response);
    free(url);
    _release_response_buf(response);
    return data;
}

enfuse_data_t* enfuse_get_branch_data(enfuse_session_t* session,
     int location_id,  char* start_date, char* end_date, int branch_id)
{
    _response_buf_t *response;
    enfuse_data_t *data;
    int url_len = strlen(GET_USAGE_DATA_BRANCH) + USERDATA_LENGTH; 
    char * url = (char*) malloc(url_len+1); 
    memset(url,0x0,url_len);
    if (snprintf(url,url_len+1,GET_USAGE_DATA_BRANCH,start_date,end_date,branch_id) < 0)
    {
        free(url);
        return NULL;
    }
    response=_post_request(session,url);
    data = _data_parse(response);
    free(url);
    _release_response_buf(response);
    return data;
}


