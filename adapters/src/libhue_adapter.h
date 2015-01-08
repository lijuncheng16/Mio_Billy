#ifndef HUE_ADAPTER_H
#define HUE_ADAPTER_H
#include "mio.h"

void* hue_data(void* adapter);
void* hue_actuate(void* adapter);
void* hue_parser(void* adapter);

#endif
