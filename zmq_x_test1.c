// zmq_x_test1.c
//
#include <zmq.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>

#include "time_util.h"
#include "zmq_x.h"

static const char s_server_addr[] = "ipc:///tmp/echo-server.ipc";

static void* server_task(void* arg)
{
    void* ctx = zmq_ctx_new();
    void* server = zmq_socket(ctx, ZMQ_REP);
    int rc = zmq_bind(server, s_server_addr);
    assert(rc == 0);
    printf("S: echo server bound at '%s'\n", s_server_addr);
    char buf[256] = {0};
    int len = 0;
    while (1) {
        len = zmq_recv(server, buf, sizeof(buf), 0);
        if (len == -1)
            break;  // Interrupted
        buf[len] == 0;
        printf("S: message received '%s'\n", buf);
        len = zmq_send(server, buf, strlen(buf), 0);
        if (len == -1)
            break;  // Interrupted
    }
    zmq_close(server);
    zmq_ctx_destroy(&ctx);
    return NULL;
}

int main(int argc, char* argv[])
{
    void* ctx = zmq_ctx_new();

    // create server task
    pthread_t server_th;
    pthread_create(&server_th, NULL, server_task, NULL);
    printf("C: press 'ENTER' when server is ready: ");
    getchar();

    void* client = zmq_socket(ctx, ZMQ_REQ);
    int rc = zmq_connect(client, s_server_addr);
    assert(rc == 0);
    char buf[256] = {0};
    int len = 0;
    int req_count = 0;
    while (1) {
        sprintf(buf, "#%03d hello", req_count++);
        printf("C: sending message '%s'...\n", buf);
        len = zmq_send(client, buf, strlen(buf), 0);
        if (len == -1)
            break;  // Interrupted
        len = zmq_recv(client, buf, sizeof(buf), 0);
        if (len == -1)
            break;  // Interrupted
        buf[len] = 0;
        printf("C: message received '%s'\n", buf);
        msleep(1000);
    }
    pthread_join(server_th, NULL);

    zmq_close(client);
    zmq_ctx_destroy(&ctx);

    return 0;
}
