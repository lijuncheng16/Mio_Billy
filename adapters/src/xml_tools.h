#ifndef XML_TOOLS_H
#define XML_TOOLS_H

#include <expat.h>


char* string_copy(const char* str);
int32_t parse_xml(char* xml_file,void (*start_handler)(void *data, const char *element, const char **attribute),
                  void (*end_handler)(void *data, const char *el));

#endif
