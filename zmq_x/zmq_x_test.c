// ipc_test.c
//
#include <zmq.h>
#include "zmq_x.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main()
{
    void* ctx = zmq_ctx_new();
    zmq_init_x(ctx);

    void* rep = zmq_socket(ctx, ZMQ_REP);

    const char addr[] = "ipc://hello-server.ipc";
    int rc = zmq_bind_x(rep, addr);
    assert(rc == 0);
    char last_endpoint[256];
    size_t len = 256;
    rc = zmq_getsockopt(rep, ZMQ_LAST_ENDPOINT, last_endpoint, &len);
    assert(rc == 0);
    last_endpoint[len] = 0;
    printf("last endpoint: %s\n", last_endpoint);
    const char* endpoint = zmq_endpoint_x(addr);
    printf("internal endpoint: %s\n", endpoint);

    zmq_close(rep);
    zmq_ctx_destroy(ctx);
}
