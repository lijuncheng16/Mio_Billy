#include "multiplexer.h"

#define IPC_USAGE "./ipc "
int main(int argc, char **argv)
{
    char *jid = argv[1];
    char *pass = argv[2],c;
    int port = atoi(argv[3]);
    multiplexer_context_t* context = multiplexer_setup(jid, pass, port);
    while (1)
    {
    	c = getchar();
    	if (c == 'q')
    		break;
    }
    //multiplexer_stop(context);
    return 0;
}
