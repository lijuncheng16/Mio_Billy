#ifndef LIBFUSE_H
#define LIBFUSE_H

#include <stdio.h>
#include <uthash.h>

typedef struct enfuse_error
{
    char* error_code;
    char* field_name;
    char* message;
    UT_hash_handle hh; 
} enfuse_error_t;

enfuse_error_t* new_enfuse_error();
void release_enfuse_error(enfuse_error_t* error);

typedef struct enfuse_response_status
{
    char* error_code;
    char* message;
    char* stack_trace;
    enfuse_error_t* errors;
    UT_hash_handle hh; 
} enfuse_response_status_t;

enfuse_response_status_t* new_response_status();
void release_response_status(enfuse_response_status_t* status);

typedef struct enfuse_session
{
    char* url;
    char* session_id;
    char* username;
    char* password;
} enfuse_session_t;

enfuse_session_t* new_enfuse_session();
void release_enfuse_session(enfuse_session_t* info);
typedef struct enfuse_location
{
    int id;
    char* name;
    char* address1;
    char* address2;
    char* address3;
    char* city;
    char* state;
    char* zipcode;
    char* timezoneID;
    double latitude;
    double longitude;
    UT_hash_handle hh; 
} enfuse_location_t;

enfuse_location_t* new_enfuse_location();
void release_enfuse_location(enfuse_location_t*);

typedef struct enfuse_panel 
{
    int panel_id;
    int location_id;
    char* label;
    char* name;
    UT_hash_handle hh; 

} enfuse_panel_t;

enfuse_panel_t* new_enfuse_panel();
void release_enfuse_panel(enfuse_panel_t* panel);

typedef struct enfuse_user
{
    char* user_name;
    char* customer;
    char* first_name;
    char* last_name;
    char* email;
    int is_admin;
    UT_hash_handle hh; 
} enfuse_user_t;

enfuse_user_t* new_enfuse_user();
void release_enfuse_user(enfuse_user_t* user);

typedef struct enfuse_status 
{
    char* version;
    char* started_at;
    char* session_id;
    char* created_at;
    char* user_name;
    char* customer;
    char* email;
    char* first_name;
    char* last_name;
    char* is_admin;
    UT_hash_handle hh; 
} enfuse_status_t;

enfuse_status_t* new_enfuse_status();
void release_enfuse_status(enfuse_status_t*);

typedef struct enfuse_data
{
    int branch_id;
    int phase;
    int location_id;
    char* local_date_stamp;
    char* utc_date_stamp;
    double kw;
    double va;
    double watt_minute;
    double va_minute;
    double min_current;
    double max_current;
    double min_voltage;
    double max_voltage;
    UT_hash_handle hh; 
} enfuse_data_t;

enfuse_data_t* new_enfuse_data();
void release_enfuse_data(enfuse_data_t* data);

typedef struct enfuse_branch {
    int branch_id;
    int panel_id;
    int location_id;
    char* label;
    char* name;
    char* node_id;
    int rated_amps;
    int breaker_position;
    int n_phases;
    enfuse_data_t* data;
    UT_hash_handle hh; 
} enfuse_branch_t;

enfuse_branch_t* new_enfuse_branch();
void release_branch(enfuse_branch_t* branch);


// JSON interface
enfuse_session_t* enfuse_authenticate(char* user, char* password);

enfuse_location_t* enfuse_get_location(enfuse_session_t* session, int location_id);
enfuse_location_t* enfuse_get_locations(enfuse_session_t* session);

enfuse_branch_t* enfuse_get_location_branches(enfuse_session_t* session, int location_id);
enfuse_branch_t* enfuse_get_panel_branches(enfuse_session_t* session, int location_id, int panel_id);
enfuse_branch_t* enfuse_get_branch(enfuse_session_t* session, int branch_id);

enfuse_panel_t* enfuse_get_panel(enfuse_session_t* session, int panel_id);
enfuse_panel_t* enfuse_get_panels(enfuse_session_t* session, int location_id);

enfuse_status_t* enfuse_get_status(enfuse_session_t* session);
enfuse_status_t* enfuse_ping(enfuse_session_t* session);
enfuse_status_t* enfuse_get_version(enfuse_session_t* session);

enfuse_data_t* enfuse_get_data(enfuse_session_t* session, int location_id, char* start_date);
enfuse_data_t* enfuse_get_data_range(enfuse_session_t* session, int location_id, char* start_date, char* end_date);
enfuse_data_t* enfuse_get_branch_data(enfuse_session_t* session, int location_id,char* start_date, char* end_date, int branch_id);

#endif
