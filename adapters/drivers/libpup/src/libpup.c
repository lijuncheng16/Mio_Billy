#include <stdlib.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "libpup.h"

// Useful defines for message indexing
#define START_MESSAGE           0
#define ADDRESS_LOWER           1
#define ADDRESS_HIGHER          2
#define LENGTH                  3
#define COMMAND                 4
#define DATA_START              5
#define EXTENDED_LENGTH_LOWER   5
#define EXTENDED_LENGTH_UPPER   6
#define EXTENDED_DATA_START     7

// pup header length definitions
#define HEADER_LENGTH		    5
#define EXTENDED_HEADER_LENGTH  7

// generates pup checkbyte for frame stream
static uint8_t generate_checkbyte(uint8_t* frame);

// prints error report to stderr
static void printerror(const char* err_str, int32_t err);

// sets the serial file to blocking or non blocking input
static int32_t set_blocking(int32_t fd, int32_t should_block);

// concatenates nbyte to frame.
int32_t append_read_byte(pup_frame_t* frame, uint8_t nbyte, int32_t count);


// prints error to std error
// takes error string describing error, and errno
static void printerror(const char* err_str, int32_t err)
{
  fprintf(stderr,"Error: %s, %d\n",err_str,err);
}

// returns the value of the checkbyte for the frame stream
uint8_t generate_checkbyte(uint8_t* frame)
{
  uint32_t counter,length;
  uint8_t check_byte;
		      
  // get the length of the frame from frame contents
  if (frame[LENGTH] == 0)     
	length  =  (frame[EXTENDED_LENGTH_UPPER] << BYTE) + frame[EXTENDED_LENGTH_LOWER];
  else if (frame[LENGTH] < HEADER_LENGTH)
	return 0;
  else 
	length = frame[LENGTH];
					      
  // iterate over frame generating checkbyte
  check_byte = 0;
  for (counter = 0; counter < length; counter++) 
  {
	 check_byte = check_byte ^ frame[counter];
     check_byte += 1;
  }
  return check_byte;
}

// appends
// arguments - frame is a frame struct filled in by # count bytes, nbyte is the byte to add to frame 
// returns - 1 on succesful packet formed, -1 on invalid packet, 0 on packet valid so far 
int32_t append_read_byte(pup_frame_t* frame, uint8_t nbyte, int32_t count)
{
  switch(count)
  {
	case START_MESSAGE:
	  return 0;
	case ADDRESS_LOWER:
	  frame -> addr = nbyte & 0xFF;
	  return 0;
	case ADDRESS_HIGHER:
	  frame -> addr += (nbyte & 0xFF) << BYTE;
	  return 0;
	case LENGTH:
	  frame -> length = nbyte;
	  if (frame -> length == 0)
		frame -> extended = TRUE;
	  if (frame -> length < HEADER_LENGTH)
		return -1;
	  else
	  { 
		frame -> extended = 0;
		frame -> data = (uint8_t*) malloc(frame -> length - HEADER_LENGTH);
	  }
	  return 0;
	case COMMAND:
	  frame -> command = nbyte;
	  return 0;
	default:
	  if (frame -> extended && (count <= EXTENDED_LENGTH_UPPER))
	  {
		if (count == EXTENDED_LENGTH_UPPER)
		{
		  frame -> length = nbyte & 0xFF; 
		  frame -> data = (uint8_t*) malloc(frame -> length - EXTENDED_HEADER_LENGTH);
		} else 
		  frame -> length = (nbyte & 0xFF) >> BYTE; 
	  } else 
	  {
		int32_t index = 0;
		if (count == frame -> length)
  		{
			uint8_t* frame_stream = generate_frame_stream(frame);
			uint8_t checkbyte = frame_stream[frame -> length];
			free(frame_stream);
			if (checkbyte == nbyte)
	  			return 1;
			else 
	  			return -1;
  		}else if (frame -> extended == TRUE)
		  index = count - EXTENDED_HEADER_LENGTH;
		else 
		  index = count - HEADER_LENGTH;
		frame -> data[index] = nbyte;
	  }
  }	   

  return 0;
}

// converts frame struct into a serialized byte array
// input - filled pup_frame_t
// output - the byte stream which represents that packet
uint8_t* generate_frame_stream(pup_frame_t* frame)
{
  uint8_t *frame_stream = (uint8_t*) malloc(frame -> length + 1); 
  if (frame -> length > 255)
	frame -> extended = TRUE;
  frame_stream[START_MESSAGE] = 0x02;
  frame_stream[ADDRESS_LOWER] = frame -> addr & 0xFF;
  frame_stream[ADDRESS_HIGHER] = (frame -> addr >> BYTE) & 0xFF;
  frame_stream[COMMAND] = frame -> command;
  if (frame -> extended)
  {
	frame_stream[LENGTH] = 0;
	frame_stream[EXTENDED_LENGTH_LOWER] =  frame -> length & 0xFF;
	frame_stream[EXTENDED_LENGTH_UPPER] = (frame -> length >> BYTE) & 0xFF;
	memcpy(&frame_stream[EXTENDED_DATA_START], frame -> data, 
								  frame -> length - EXTENDED_HEADER_LENGTH); 
  }
  else 
  {
	frame_stream[LENGTH] = frame -> length;
	memcpy(&frame_stream[DATA_START], frame -> data, frame -> length - HEADER_LENGTH);
  }
  frame_stream[frame -> length] = generate_checkbyte(frame_stream); 
  return frame_stream;
}

// writes a frame the pup bus, optionally waits for a response. This command blocks if
// response is true.
// arguments - fd, file descriptor of the serial port. frame_stream
//      byte stream of the frame. response, if true wait for response to packet
// returns - the response frame if response set to true, NULL otherwise
pup_frame_t* write_frame(int fd, uint8_t* frame_stream, int32_t response)
{
  int32_t err = set_blocking(fd,0);
  int32_t length;
  if (err == -1)
	return NULL;
  if (frame_stream[LENGTH] != 0)
    length = frame_stream[LENGTH];
  else 
    length = (frame_stream[EXTENDED_LENGTH_UPPER] << 8) + frame_stream[EXTENDED_LENGTH_LOWER];
  if (set_blocking(fd,0))
    return NULL;
  err = write(fd,frame_stream,length+1);
  if (err <= 0)
  {
    err = errno;
    printerror("Write failed",err);
    return NULL;
  }
  err = tcdrain(fd);
  if (err == -1) 
  {
	err = errno;
	printerror("didn't drain serial\n",err);
	return NULL;
  }
  if (response)
	return read_frame(fd);
  return NULL;
}

// reads a frame from the pup_bus. This command blocks.
// arguments - file descriptor of serial port
// returns - pup_frame read from, NULL on error
pup_frame_t* read_frame(int fd)
{
  pup_frame_t* frame,*list=NULL,*tmp,*prev,*next;
  uint8_t byte;
  int32_t in_buff,err;
  if (set_blocking(fd,1))
	return NULL;
  while ((in_buff = read(fd,&byte,1)) != 0) 
  {
      // check if byte read
    if (in_buff != 1)
	  	continue;
    // if byte is 0x02, it is the possible start of a frame
	if (byte == 0x02)
	{
		frame = new_pup_frame(0,0,0,NULL);
		frame -> index = 0;
		frame -> hh.next = list;
		list = frame;
	}
    // iterate over possible frames
	for (frame = list; frame; frame = frame -> hh.next) 
	{
		err = append_read_byte(frame,byte,frame -> index);
        // increment index into pup_frame
		frame -> index ++;

        // err 1 indicates found valid packet
		if (err == 1)
		{
			// free frames which were not completed
			tmp = list;
			while (tmp)
			{
				next = tmp -> hh.next;
				if (tmp != frame)
					free_pup_frame(tmp);
				tmp = next;
			}
            // return valid frame
			return frame;
		}
        // err as -1 indicates the frame is invalid, remove frame 
        // from linked list
		else if (err == -1)
		{
			tmp = list;
			while (tmp)
			{
				if (tmp == frame)
				{
					if (tmp == list)
						list = list -> hh.next;
					else 
						prev -> hh.next = tmp -> hh.next;
					frame = tmp -> hh.next;
					free_pup_frame(tmp);
					break;
				}
				prev = tmp;
				tmp = tmp -> hh.next;
			}
			if (frame == NULL)
				break;
		}
	}
  }
  if (err < 0)
  {
    err = errno;
    printerror("Reading from serial failed", err);
  }
  return NULL;
}

// implements the read_attribute pup command, this function blocks 
// arguments - pup_context, device address of queried attribute, channel id of attribtue, destination attribute struct 
// returns - 0 on success -1 on failure
int32_t read_attribute(pup_context_t* context, uint16_t dev_addr, uint16_t channel_id, pup_attribute_t* attribute)
{
  uint8_t *frame_stream;
  int32_t retval = 0;

  pup_frame_t* frame = new_pup_frame(dev_addr,pup_read_attribute,pup_read_attribute_length,NULL);

  frame -> data = (uint8_t*) malloc(
				  (pup_command_length_t) pup_read_attribute_length - HEADER_LENGTH);
  frame -> data[0] = channel_id & 0xFF;
  frame -> data[1] = (channel_id >> BYTE) & 0xFF;
  frame -> data[2] = attribute -> attribute_name[0];
  frame -> data[3] = attribute -> attribute_name[1];
  frame_stream = generate_frame_stream(frame);
  free_pup_frame(frame);
  frame = write_frame(context->fd,frame_stream,1);
  free(frame_stream);
  if (frame == NULL)
	retval = -1;
  else
  {
	if (frame -> command == 0x80)
	{
		printf("PUP error read frame #%d\n",frame ->data[HEADER_LENGTH] + (frame -> data[HEADER_LENGTH +1] << 8));
		if (frame -> length > HEADER_LENGTH + 2)
		printf("%s\n",(char*)&frame -> data[HEADER_LENGTH + 2]);
	}
     if (frame -> command != pup_numeric_data) 
     { 	
	printf("%d\n", frame -> command);
	printf("not numeric data\n");
	return -1; 
     }
     attribute -> value = (uint8_t*) malloc(4);
     attribute -> display = frame -> data[2];
     memcpy(attribute -> value, &frame -> data[3], 4); 
     free(frame);
  }
  return retval;
}

// implements the pup defined write_attribute command
// arguments - pup_context, destination device address, destination channel id, attribute to be written
// returns - 0 on success -1 on error
int32_t write_attribute(pup_context_t* context, uint16_t dev_addr,uint16_t channel_id,pup_attribute_t *attribute)
{
  uint8_t *frame_stream;
  pup_frame_t* frame = new_pup_frame(dev_addr,pup_write_attribute,pup_write_attribute_length,NULL);
  int32_t retval = 0;

  frame -> addr = dev_addr;
  frame -> command = (pup_command_t) pup_write_attribute;
  frame -> length = (pup_command_length_t) pup_write_attribute_length;
  frame -> data = (uint8_t*) malloc(
				  (pup_command_length_t) pup_write_attribute_length - HEADER_LENGTH);
  frame -> data[0] = channel_id & 0xFF;
  frame -> data[1] = (channel_id >> BYTE) & 0xFF;
  frame -> data[2] = attribute -> attribute_name[0];
  frame -> data[3] = attribute -> attribute_name[1];

  frame -> data[4] = attribute -> value[0];
  frame -> data[5] = attribute -> value[1];
  frame -> data[6] = attribute -> value[2];
  frame -> data[7] = attribute -> value[3];

  frame_stream = generate_frame_stream(frame);
  free_pup_frame(frame);
  frame = write_frame(context->fd,frame_stream, 1);
  free(frame_stream);
  if (frame == NULL)
	retval = -1;
  free(frame);
  return retval;
}

// Passes token, optionally blocks until receives token back
// arguments - pup context, the context to pass token on. should block, if true will block until
//              token receieved
// returns - 0 on success -1 on failure
int32_t pass_token(pup_context_t* context, int32_t should_block)
{
  uint8_t *frame_stream;
  pup_device_t *device = context -> next_master;
  if (device == NULL)
  {
        printf("next master is NULL\n");
        return -1;
  }
  pup_frame_t* frame = new_pup_frame(device -> addr, pup_pass_token, pup_pass_token_length, NULL);
  frame -> data = (uint8_t*) malloc(pup_pass_token_length - HEADER_LENGTH);
  frame -> data[0] = frame -> addr & 0xFF;
  frame -> data[1] = (frame -> addr >> BYTE) & 0xFF;
  frame_stream = generate_frame_stream(frame);	
  write_frame(context->fd,frame_stream,0);
  free_pup_frame(frame);
  free(frame_stream);
  while (should_block)
  {
	frame = read_frame(context -> fd);
	if ((frame != NULL) && (frame -> command == pup_pass_token) && (frame -> addr == context -> addr))
        {
 	  free_pup_frame(frame);
	  break;
	}
 	free_pup_frame(frame);
  }
  return 0;
}

int32_t say_hello(pup_context_t* context, uint16_t addr)
{
  pup_frame_t* frame = new_pup_frame(addr,pup_say_hello,pup_say_hello_length,NULL);
  uint8_t *frame_stream;
  int32_t retval = 0;
  frame_stream = generate_frame_stream(frame);	
  free_pup_frame(frame);
  frame = write_frame(context -> fd, frame_stream, 1);
  if ((frame == NULL) || (frame -> command != pup_general_acknowledge))
	retval = -1;
  free_pup_frame(frame);
  return retval;
}

// connects the pup_context to the serial path set in context
// arguments - pup_context, should not be connected already
// returns  - 0 on success -1 on failure
int32_t connect_pup_context(pup_context_t* context)
{
  struct termios t;
  int ret,err;
  context -> fd = open(context -> serial_path, O_RDWR | O_NOCTTY | O_NDELAY);
  if (context -> fd == -1) {
	err = errno;
	printerror("Could not open serial file",err);
	return err;
  }
  ret = tcgetattr(context -> fd, &t);
  if (ret == -1) {
	err = errno;
	printerror("Could not get attributes for serial path",err);
	return err;
  }
  cfmakeraw(&t);
  cfsetispeed(&t, B9600);
  cfsetospeed(&t, B9600);
  tcflush(context -> fd, TCIFLUSH);
  t.c_iflag |= IGNPAR;
  t.c_cflag &= ~CSTOPB;
  ret = tcsetattr(context -> fd, TCSANOW, &t);
  if (ret == -1) {
	err = errno;
	printerror("error setting terminal attributes",err);
	return ret;
  }
  pthread_mutex_unlock(context -> serial_mutex);
  return 0;
}

// closses serial file
// arguments - pup context with connected fd
// returns - error of close statement
int32_t disconnect_pup_context(pup_context_t* context)
{
  int32_t err = close(context -> fd);
  return err;
}

// sets file descriptor's settings to blocking or not blocking
// arguments - fd, filedescriptor of serial file. should_block, 1 if should block 0 otherwise
// returns - 0 on success -1 on failure
int32_t set_blocking(int32_t fd, int32_t should_block)
{
    int opts;

    opts = fcntl(fd,F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    if (should_block)
	opts = opts & ~O_NONBLOCK;
    else 
        opts = (opts | O_NONBLOCK);
    if (fcntl(fd,F_SETFL,opts) < 0) {
        perror("fcntl(F_SETFL)");
	    return -1;
    }
    return 0;
}

// creates a new pup_device_t struct
// arguments - pup address of device, device name and type, and initialized pup_channels  
// returns - pup_device_t struct with provided parameters
pup_device_t* new_pup_device(uint16_t addr, char* name, char* device_type, pup_channel_t *channels)
{
  pup_device_t *device = (pup_device_t*) malloc(sizeof(pup_device_t));
  memset(device,0x0,sizeof(pup_device_t));
  device -> mem_size = 0;
  device -> addr = addr;
  device -> name = name;
  device -> type = device_type;
  device -> channels = channels;
  return device;
}

// creates a new pup_channel_t struct
// arguments - address of channel, channel name, and associated pup attributes attributes
// returns - initialized channel struct
pup_channel_t* new_pup_channel(uint16_t channel_addr, char* name, pup_attribute_t *attributes)
{
  pup_channel_t *channel = (pup_channel_t*) malloc(sizeof(pup_channel_t));
  memset(channel,0x0,sizeof(pup_channel_t));
  channel -> channel = channel_addr;
  channel -> name = name;
  channel -> attributes = attributes;
  return channel;
}

// creates a new pup_channel_t struct
// arguments - attribute name is the 2 character name of attribute, type is a descriptor of the attribute,
//          value points to either a string or a uint32_t value. value_length is number of bytes in value
// returns - initialized pup_attribute_t struct
pup_attribute_t* new_pup_attribute(char* attribute_name, char* type, uint8_t* value, int32_t value_length)
{
  pup_attribute_t *attribute = (pup_attribute_t*) malloc(sizeof(pup_attribute_t));
  memset(attribute,0x0,sizeof(pup_attribute_t));
  if (attribute_name != NULL)
    strcpy(attribute -> attribute_name,attribute_name);
  else
  	attribute -> attribute_name[0] = '\0';
  attribute -> type = type;
  attribute -> value = value;
  return attribute;
}

// generates a new pup_context connected to serial_file if provided
// arguments - addr is the address of generated pup_context, serial_file is the path to tty serial file
//          baud_rate is the baudrate for the serial connection, devices are the devices associated with 
//          the context's pup network, timeout is the maximum time to wait for a response
// returns - an initialized pup_context connected to serial with given parameters is serial_file provided
pup_context_t*  new_pup_context(uint16_t addr, char* serial_file, int32_t baud_rate, pup_device_t* devices, int32_t timeout)
{
  int32_t err;
  pup_context_t* context = (pup_context_t*) malloc(sizeof(pup_context_t));
  memset(context,0,sizeof(pup_context_t));
  context -> addr = addr;
  context -> serial_path = serial_file;
  context -> baud_rate = baud_rate;
  context -> fd = 0;
  context -> devices = devices;
  context -> token_pass_timeout = timeout;
  context -> serial_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)); 

  // setup serial file and mutex
  if (serial_file != NULL)
    connect_pup_context(context); 
  else
      return context;
  if (pthread_mutex_init(context -> serial_mutex, NULL) != 0)
  {
	  err = errno; 
	  printerror("Mutex initialization failed\n",err);
  	  return NULL;
  } else 
      pthread_mutex_unlock(context -> serial_mutex);

  context -> next_master = NULL;
  return context;
}

// generates a pup frame with given parameters
// arguments - addr is the destination pup address, command is the 16bit command sequence
//      length is the length of the header + value, value is a byte array of data
// returns - pup_frame struct initialized with the given values
pup_frame_t* new_pup_frame(uint16_t addr, uint16_t command,uint16_t length, uint8_t* value)
{
  pup_frame_t* frame = (pup_frame_t*) malloc(sizeof(pup_frame_t));
  frame -> addr = addr;
  frame -> command = command;
  frame -> length = length;
  frame -> data = NULL;
  if (0xFF00 & frame -> length)
  {
	if (value != NULL)
	{
	  frame -> data = (uint8_t*) malloc(length - EXTENDED_HEADER_LENGTH);
	  memcpy(frame -> data,value, length - EXTENDED_HEADER_LENGTH);
	}
	frame -> extended = TRUE;
  } else
  {
	if (value != NULL)
	{
	  frame -> data = (uint8_t*) malloc(length - HEADER_LENGTH);
	  memcpy(frame -> data,value, length - EXTENDED_HEADER_LENGTH);
	}
	frame -> extended = FALSE;
  }

  return frame;
}

// frees memory associated with a pup frame
void free_pup_frame(pup_frame_t* frame)
{ 
  if (frame == NULL)
	return;
  if (frame -> data != NULL)
	free(frame -> data);
  free(frame);
}

// frees memory associated with a pup attribute
void free_pup_attribute(pup_attribute_t* attribute)
{
  free(attribute);
}

// frees memory associated with a pup device
void free_pup_device(pup_device_t* device)
{
  free(device);
}

// frees memory associated with a pup channel
void free_pup_channel(pup_channel_t* channel)
{
  free(channel);
}

// frees memory associated with a pup context
void free_pup_context(pup_context_t* context)
{
  free(context);
}

// prints to stdout the values associated with a pup context 
void print_pup_context(pup_context_t* context)
{
	pup_device_t* device;
    if (context -> name == NULL)
        printf("Context: (NULL)\n");
    else
        printf("CONTEXT: %s\n",context -> name);
    printf("Device Address %d\n",context -> addr);
    printf("FD : %d\n", context -> fd);
    printf("Serial Path: %s\n",context -> serial_path);
    printf("Baud Rate: %d\n",context -> baud_rate);
    if (context -> next_master != NULL)
    	printf("Next Master: %d\n",0xFFFF & context -> next_master -> addr);
    else 
    	printf("Next Master: NULL\n");
    printf("Token Timeout: %d\n", context -> token_pass_timeout);

    printf("\n");
    printf("Devices in context:\n");
    printf("\n");

    if (context -> devices == NULL)
    {
	printf("No Devices\n");
	return;
    }
    for (device = context -> devices; device; device = device -> hh.next)
    {
        print_pup_device(device);
        printf("\n");
    }
    return;
}

// prints to stdout the values associated with a pup context 
void print_pup_device(pup_device_t* device)
{
    pup_channel_t* channel;
    printf("DEVICE: %s\n", device -> name);
    printf("Address: %d\n", device -> addr);
    printf("ID: %s\n",device -> node_id);
    printf("Location: %s\n", device -> location);
    printf("Type: %s\n",device -> type);
    printf("Memmory Size: %d\n", device -> mem_size);
    printf("\nDevice Channels:\n\n");
    for (channel = device -> channels; channel; channel = channel -> hh.next)
    {
        print_pup_channel(channel);
        printf("\n");
    }

    return;
}

// prints to stdout the values associated with a pup channel
void print_pup_channel(pup_channel_t* channel)
{
    pup_attribute_t *attribute;
    printf("CHANNEL: %s\n", channel -> name);
    printf("ID 0x%x\n", channel -> channel);
    printf("\nChannel Attributes:\n");
    for (attribute = channel -> attributes; attribute; attribute = attribute -> hh.next)
    {
        print_pup_attribute(attribute);
        printf("\n");
    }
}

// prints to stdout the values associated with a pup attribute
void print_pup_attribute(pup_attribute_t *attribute)
{
    printf("ATTRIBUTE: %s\n",attribute -> attribute_name);
    if (attribute -> type != NULL) 
    	printf("Attribute Type: %s\n", attribute -> type); 
    else 
    	printf("Attribute Type (null)\n");
    printf("Update %d\n", attribute -> update);
    if (attribute -> value != NULL)
    {
        // print value
	char* val_str = data_string(attribute -> display, *((uint32_t*)attribute -> value));
        printf("value %s\n", val_str);
    } else 
        printf("value (NULL)\n");
}

// prints the values associated with a pup_frame
void print_pup_frame(pup_frame_t* frame)
{
	if (frame == NULL)
		printf("frame is null\n");
	else
		printf("Frame:addr: %d length %d extended: %d command : %d\n", frame -> addr, frame -> length, frame -> extended, frame -> command);
}

// returns a formatted string of data 
// type is the data type of the uint32_t 
// data is the raw data transfered over pup
char* data_string(uint8_t type, uint32_t data)
{
	char* str=(char*)malloc(15);
	switch(type) 
	{
	case SIGNED_TEN_DIGIT:
		sprintf(str,"%d",(int32_t)data);
		break;
	case UNSIGNED_TEN_DIGIT:
		sprintf(str,"%d",data);
		break;
	case SIGNED_NINE_ONE_DIGIT:
		sprintf(str,"%d.%d",(int32_t)data/10, (int32_t)data%10);
		break;
	case UNSIGNED_NINE_ONE_DIGIT:
		sprintf(str,"%d.%d\n",data/10, data%10);
		break;
	case SIGNED_EIGHT_TWO_DIGIT:
		sprintf(str,"%d.%d\n",(int32_t)data/100, (int32_t)data%100);
		break;
	case UNSIGNED_EIGHT_TWO_DIGIT:
		sprintf(str,"%d.%d\n",data/100, data%100);
		break;
	case SIGNED_SEVEN_THREE_DIGIT:
		sprintf(str,"%d.%d\n",(int32_t)data/1000, (int32_t)data%1000);
		break;
	case UNSIGNED_SEVEN_THREE_DIGIT:
		sprintf(str,"%d.%d\n",data/1000, data%1000);
		break;
	case SIGNED_SIX_FOUR_DIGIT:
		sprintf(str,"%d.%d\n",(int32_t)data/10000, (int32_t)data%10000);
		break;
	case UNSIGNED_SIX_FOUR_DIGIT:
		sprintf(str,"%d.%d\n",data/10000, data%10000);
		break;
	case SIGNED_FIVE_FIVE_DIGIT:
		sprintf(str,"%d.%d\n",(int32_t)data/100000, (int32_t)data%100000);
		break;
	case UNSIGNED_FIVE_FIVE_DIGIT:
		sprintf(str,"%d.%d\n",data/100000, data%100000);
		break;
	case SIGNED_FOUR_SIX_DIGIT:
		sprintf(str,"%d.%d\n",(int32_t)data/1000000, (int32_t)data%1000000);
		break;
	case UNSIGNED_FOUR_SIX_DIGIT: 
		sprintf(str,"%d.%d\n",data/1000000, data%1000000);
		break;
	case SIGNED_THREE_SEVEN_DIGIT:
		sprintf(str,"%d.%d\n",(int32_t)data/10000000, (int32_t)data%10000000);
		break;
	case UNSIGNED_THREE_SEVEN_DIGIT:
		sprintf(str,"%d.%d\n",data/10000000, data%10000000);
		break;
	case SIGNED_TWO_EIGHT_DIGIT:
		sprintf(str,"%d.%d\n",(int32_t)data/100000000, (int32_t)data%100000000);
		break;
	case UNSIGNED_TWO_EIGHT_DIGIT:
		sprintf(str,"%d.%d\n",data/100000000, data%100000000);
		break;
	case SIGNED_ONE_NINE_DIGIT:
		sprintf(str,"%d.%d\n",(int32_t)data/1000000000, (int32_t)data%1000000000);
		break;
	case UNSIGNED_ONE_NINE_DIGIT:
		sprintf(str,"%d.%d\n",data/1000000000, data%1000000000);
		break;
	case SIGNED_ZERO_TEN_DIGIT:
		sprintf(str,".%d\n",(int32_t)data);
		break;
	case UNSIGNED_ZERO_TEN_DIGIT:
		sprintf(str,".%d\n",data);
		break;
	case BITMAP:
	case UBITMAP:
	case BCD_LONG_TIME:
		sprintf(str,"%d:%d:%d\n",0xFF&data, 0xFF & (data >> 8), 0xFF & (data >> 16)); 
		break;
	case BCD_SHORT_TIME: 
		sprintf(str,"%d:%d\n",0xFF&data, 0xFF & (data >> 8)); 
		break;
	case BCD_PACKED:
		sprintf(str,"%d %d %d %d %d %d %d %d\n",0xF&data, 0xF & (data >> 4), 0xF & (data >> 8), 0xF & (data >> 8), 0xF & (data >> 16),0xF & (data >> 20), 0xF & (data >> 24), 0xF & (data >> 28)); 
		break;
	case BCD_DATE:
		//printf("%d/%d/%d\n",);
		break;
	case BINARY_DATE:
		//printf("%d/%d/%d\n",);
		break;
	case FLOATING_POINT_THIRTY_TWO:
		// warning, should be 32 bit float check with compiler
		sprintf(str,"%f\n", (float) data);
		break;
	case HEXADECIMAL_BYTE:
	case HEXADECIMAL_WORD:
	case HEXADECIMAL_DOUBLE_WORD:
	default: 
		printf("a %d type\n",type);
    		str = NULL;
	}
	return str;
}
