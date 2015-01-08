#ifndef POS_XML_H
#define POS_XML_H

#include "libpup.h"
#include "pos.h"

#define DEVICE_DEPTH        0
#define CHANNEL_DEPTH       1
#define ATTRIBUTE_DEPTH     2

pup_adapter_t* parse_adapter(char* pup_network_xml, char* device_xml, char* tid_xml);
#endif
