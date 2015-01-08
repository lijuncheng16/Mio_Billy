#ifndef LIBPUP_H
#define	LIBPUP_H

#include <stdint.h>
#include <sys/time.h>
#include <pthread.h>
#include "uthash.h"

#define BYTE 					8

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define PUP_MASTER 				1
#define PUP_SLAVE 				0

#define PUP_BROADCAST_ADDRESS 	0xFFFF

// PUP DATATYPES
#define SIGNED_TEN_DIGIT		0xFF
#define UNSIGNED_TEN_DIGIT		0xFE
#define SIGNED_NINE_ONE_DIGIT		0xFD
#define UNSIGNED_NINE_ONE_DIGIT		0xFC
#define SIGNED_EIGHT_TWO_DIGIT		0xFB
#define UNSIGNED_EIGHT_TWO_DIGIT	0xFA
#define SIGNED_SEVEN_THREE_DIGIT	0xF9
#define UNSIGNED_SEVEN_THREE_DIGIT 	0xF8
#define SIGNED_SIX_FOUR_DIGIT		0xF7
#define UNSIGNED_SIX_FOUR_DIGIT 	0xF6
#define SIGNED_FIVE_FIVE_DIGIT		0xF5
#define UNSIGNED_FIVE_FIVE_DIGIT 	0xF4
#define SIGNED_FOUR_SIX_DIGIT		0xF3
#define UNSIGNED_FOUR_SIX_DIGIT 	0xF2
#define SIGNED_THREE_SEVEN_DIGIT	0xF1
#define UNSIGNED_THREE_SEVEN_DIGIT 	0xF0
#define SIGNED_TWO_EIGHT_DIGIT		0xEF
#define UNSIGNED_TWO_EIGHT_DIGIT 	0xEE
#define SIGNED_ONE_NINE_DIGIT		0xED
#define UNSIGNED_ONE_NINE_DIGIT 	0xEC
#define SIGNED_ZERO_TEN_DIGIT		0xEB
#define UNSIGNED_ZERO_TEN_DIGIT 	0xEA
#define BITMAP						0xE9
#define UBITMAP						0xE8
#define BCD_LONG_TIME				0xE7
#define BCD_SHORT_TIME				0xE6
#define BCD_PACKED					0xE5
#define BCD_DATE					0xE4
#define BINARY_DATE					0xE3
#define FLOATING_POINT_THIRTY_TWO	0xE0
#define HEXADECIMAL_BYTE			0x00
#define HEXADECIMAL_WORD			0x01
#define HEXADECIMAL_DOUBLE_WORD		0x02

// ERROR CODES
#define NAK_RESPONSE				0xFFFF
#define UNSUPPORTED_PUP_COMMAND		0xFFFE
#define NO_SUCH_CHANNEL				0xFFFD
#define NO_SUCH_ATTRIBUTE			0xFFFC
#define INVALID_ATTRIBUTE_VALUE		0xFFFB
#define INSUFFICIENT_BUFFER_SPACE	0xFFFA
#define NO_SUCH_REGION				0xFFF9
#define REGION_IN_USE				0xFFF8
#define INVALID_REGION_OFFSET		0xFFF7
#define BUFFER_OVERRUN				0xFFF6

#define ERROR_STRING_SIZE			20

// PUP Schema definition
typedef struct pup_attribute
{
  char attribute_name[3];
  char *type;
  uint8_t* value;
  int32_t update;
  uint8_t display;
  UT_hash_handle hh;
} pup_attribute_t;

typedef struct pup_channel
{
  uint16_t channel;
  char *name;
  pup_attribute_t *attributes;
  UT_hash_handle hh;
} pup_channel_t;

typedef struct pup_device
{
  uint16_t addr;
  char* name;
  char node_id[50];
  char *location;
  char *type;
  pup_channel_t *channels;
  uint8_t* mem_block;
  int32_t mem_size;
  UT_hash_handle hh;
} pup_device_t;

// PUP data structs
typedef struct pup_context
{
  char* name;
  uint16_t addr;
  
  pthread_mutex_t* serial_mutex;
  char* serial_path;
  int32_t fd;
  int32_t baud_rate;

  pup_device_t *next_master;
  int32_t token_pass_timeout;
  pup_device_t *devices;
  UT_hash_handle hh;
} pup_context_t;

typedef struct pup_frame
{
  int16_t addr;
  int16_t length;
  uint8_t extended;
  int16_t command;
  uint8_t* data;
  int32_t index;
  UT_hash_handle hh;
} pup_frame_t;

void print_pup_frame(pup_frame_t* frame);
// ENUM DEFINITIONS
typedef enum pup_exception
{
  pup_no_exception 		= 0
} pup_exception_t;

typedef enum pup_command_length
{
  pup_sync_time_date_length		= 12,
  pup_read_attribute_length		= 9,
  pup_write_attribute_length 	=13,
  pup_pass_token_length			= 7,
  pup_say_hello_length			= 5
} pup_command_length_t;

typedef enum pup_command 
{
  pup_sync_time_date	= 0,
  pup_read_attribute	= 1,
  pup_write_attribute 	= 2,
  pup_pass_token		= 3,
  pup_say_hello			= 4
} pup_command_t;

typedef enum pup_response
{
  pup_error					= 0x80,
  pup_general_acknowledge 	= 0x81,
  pup_numeric_data			= 0x82,
  pup_text_data				= 0x83,
  pup_invalid_frame			= 0xFF
} pup_response_t;

// struct initialization and destruction functions
pup_context_t*  new_pup_context(uint16_t addr, char* serial_file, int32_t baud_rate, pup_device_t* devices, int32_t timeout);
pup_device_t* new_pup_device(uint16_t addr, char* name, char* device_type, pup_channel_t *channels);
pup_channel_t* new_pup_channel(uint16_t channel_addr, char* name, pup_attribute_t* attributes);
pup_attribute_t* new_pup_attribute(char* attribute_name, char* attribute_descriptor, uint8_t* value, int32_t value_length);
pup_frame_t* new_pup_frame(uint16_t addr, uint16_t command, uint16_t length, uint8_t* value);

void free_pup_frame(pup_frame_t* frame);
void free_pup_attribute(pup_attribute_t* attribute);
void free_pup_device(pup_device_t* device);
void free_pup_channel(pup_channel_t* channel);
void free_pup_attribute(pup_attribute_t* attribute);
void free_pup_context(pup_context_t* context);

void print_pup_context(pup_context_t* context);
void print_pup_device(pup_device_t* device);
void print_pup_channel(pup_channel_t* channel);
void print_pup_attribute(pup_attribute_t *attribute);

int32_t connect_pup_context(pup_context_t* context);
int32_t disconnect_pup_context(pup_context_t* context);

char* data_string(uint8_t type, uint32_t data);
uint8_t* generate_frame_stream(pup_frame_t* frame);
pup_frame_t* write_frame(int fd, uint8_t* frame_stream, int32_t response);
pup_frame_t* read_frame(int fd);

// master commands
int32_t read_attribute(pup_context_t* context, uint16_t dev_addr, 
											uint16_t channel_id, pup_attribute_t *attribute);
int32_t write_attribute(pup_context_t* context, uint16_t dev_addr, 
											uint16_t channel_id, pup_attribute_t *attribute);
int32_t pass_token(pup_context_t* context, int32_t should_block);
int32_t say_hello(pup_context_t* context, uint16_t addr);

/*int32_t synchronize_time_date();
int32_t read_region_data();
int32_t write_region_data();
int32_t create_named_region();
int32_t lookup_named_region();
int32_t report_exception_message();
int32_t exchange_data_virtual_terminal();
int32_t retransmit_virtual_printout();
int32_t virtual_terminal_open();
int32_t virtual_terminal_close();
int32_t acknowledge_transaction();
int32_t change_operation_mode();
int32_t declare_exception_message();
int32_t read_address();
int32_t write_address();
int32_t free_region();
int32_t write_attribute_text();
int32_t write_zone_attribute();
int32_t flash_upgrade();
int32_t get_command(int32_t blocking);
int32_t error_response();
int32_t general_acknowledge_response();
int32_t numeric_data_response();
int32_t text_data_response();
int32_t region_data_response();
int32_t exception_message_response();
int32_t virtual_printout_response();
int32_t region_name_response();*/
#endif
