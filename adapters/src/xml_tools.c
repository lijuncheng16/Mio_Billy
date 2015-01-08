#include "xml_tools.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>


char* string_copy(const char* str)
{
    char* copy = (char*) malloc(strlen(str)+1);
    memcpy(copy,str,strlen(str));
    copy[strlen(str)] = 0;
    return copy;
}

int32_t parse_xml(char* xml_file,void (*start_handler)(void *data, const char *element, const char **attribute),
                  void (*end_handler)(void *data, const char *el))
{
    int32_t buff_size = 512, err, done = 0, break_out = 0;
    char buff[512], *ret;
    FILE *fp;
    fp = fopen(xml_file, "r");
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(parser, start_handler, end_handler);
    while (!done)
    {
        ret = fgets(buff, buff_size, fp);
        if (ret == NULL)
            break;
        if (feof(fp))
        {
            done = 1;
            break_out = 1;
        }
        if (XML_STATUS_ERROR == (XML_Parse(parser, buff, strlen(buff), done))) {
            err = errno;
            XML_ParserFree(parser);
            printf("XML_Parse Failed %s\n", XML_ErrorString(err));
            return 1;
        }
        if (break_out)
        {
            printf("BROKe\n");
            break;
        }
    }
    XML_ParserFree(parser);
    fclose(fp);
    return 0;
}
