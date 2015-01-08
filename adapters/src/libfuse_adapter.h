#ifndef LIBFUSE_ADAPTER_H
#define LIBFUSE_ADAPTER_H
#include "adapter.h"
#include <libfuse.h>
#include <mio.h>

void* libfuse_data(void* adapter);
void* libfuse_parser(void*);

#endif
