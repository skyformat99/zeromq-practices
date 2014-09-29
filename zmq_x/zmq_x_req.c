// zmq_x_req.c
//
#include <zmq.h>
#include <stdio.h>

#include "zmq_x.h"
#include "time_util.h"

int main(int argc, char* argv[])
{
    char addr[] = "ipc://echo-server.ipc";
    void* ctx = zmq_ctx_new();
    zmq_init_x(ctx);

    void* client = zmq_socket(ctx, ZMQ_REQ);
    zmq_connect(client, addr);
    printf("I: connecting to echo-server at '%s'(real endpoint '%s')...\n",
            addr, zmq_endpoint_x(addr));
    char buf[256];
    int len = 0;
    int req_count = 0;
    while (1) {
        sprintf(buf, "#%03d hello", req_count++);
        printf("I: request - '%s'\n", buf);
        len = zmq_send(client, buf, strlen(buf), 0);
        if (len == -1)
            break;  // Interrupted
        len = zmq_recv(client, buf, sizeof(buf), 0);
        if (len == -1)
            break;  // Interrupted
        buf[len] = 0;
        printf("I: reply - '%s'\n", buf);
        msleep(1000);
    }
    zmq_close(client);
    zmq_ctx_destroy(&ctx);

    return 0;
}
