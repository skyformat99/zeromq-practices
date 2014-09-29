// czmq_x_test.c
//
#include <czmq.h>
#include "czmq_x.h"

int main()
{
    zctx_t* ctx = zctx_new();
    void* server = zsocket_new(ctx, ZMQ_REP);

    const char addr[] = "ipc://hello-server.ipc";
    int port = zsocket_bind(server, addr);
    printf("bound port: %d\n", port);

    zctx_destroy(&ctx);
}
