// zmq_x_rep.c
//
#include <zmq.h>
#include <stdio.h>

#include "zmq_x.h"

int main(int argc, char* argv[])
{
    char addr[] = "ipc://echo-server.ipc";
    void* ctx = zmq_ctx_new();
    zmq_init_x(ctx);

    void* server = zmq_socket(ctx, ZMQ_REP);
    zmq_bind(server, addr);
    printf("I: binding echo-server at '%s'(real endpoint '%s')\n",
            addr, zmq_endpoint_x(addr));
    char buf[256];
    int len = 0;
    while (1) {
        len = zmq_recv(server, buf, sizeof(buf), 0);
        if (len == -1)
            break;  // Interrupted
        buf[len] = 0;
        printf("I: receive message '%s'\n", buf);
        len = zmq_send(server, buf, strlen(buf), 0);
        if (len == -1)
            break;  // Interrupted
    }
    zmq_close(server);
    zmq_ctx_destroy(&ctx);
    return 0;
}
