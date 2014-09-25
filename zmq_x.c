// zmq_x.c
//
#include "zmq_x.h"

#include <zmq.h>
#include <pthread.h>
#include <assert.h>

#define MAX_ENDPOINTS 100

struct addr_endpoint_t {
    char addr[256];
    char endpoint[32];
};
static struct addr_endpoint_t s_endpoints[MAX_ENDPOINTS];
static int s_endpoints_size = 0;

static const char s_ipc_endpoint[] = "tcp://127.0.0.1:5555";
static void* s_ipc_server = NULL;
static void* s_ipc_client = NULL;

static void* ipc_server_task(void* arg)
{
    void* ctx = arg;
    char addr[256] = {0};
    int len = 0;
    while (1) {
        len = zmq_recv(s_ipc_server, addr, sizeof(addr), 0);
        if (len == -1)
            break;  // Interrupted
        addr[len] = 0;
        const char* endpoint = zmq_endpoint_x(addr);
        assert(endpoint);
        printf("D: resolve '%s' as '%s'\n", addr, endpoint);
        len = zmq_send(s_ipc_server, endpoint, strlen(endpoint), 0);
        if (len == -1)
            break;  // Interrupted
    }
    zmq_close(s_ipc_server);
    if (ctx)
        zmq_ctx_destroy(&ctx);
    return NULL;
}

static const char* insert_endpoint_item(const char* addr, const char* endpoint)
{
    struct addr_endpoint_t* t = s_endpoints + s_endpoints_size;
    strcpy(t->addr, addr);
    strcpy(t->endpoint, endpoint);
    s_endpoints_size++;
    return t->endpoint;
}

static int bind_new_endpoint(void* s, const char* addr)
{
    int rc = 0;
    rc = zmq_bind(s, "tcp://127.0.0.1:*");  // use "tcp://" instead
    if (rc != 0)
        return rc;
    char last_endpoint[256];
    size_t len = 256;
    rc = zmq_getsockopt(s, ZMQ_LAST_ENDPOINT, last_endpoint, &len);
    if (rc != 0)
        return rc;
    last_endpoint[len] = 0;
    insert_endpoint_item(addr, last_endpoint);
    return 0;
}

static int is_x_addr(const char* addr)
{
    static const char s_ipc_head[] = "ipc://";
    static const char s_inproc_head[] = "inproc://";
    return (strstr(addr, s_ipc_head) == addr ||
           strstr(addr, s_inproc_head) == addr);
}

// Interfaces
//
int zmq_bind_x(void* s, const char* addr)
{
    const char* endpoint = 0;
    if (is_x_addr(addr)) {
        endpoint = zmq_endpoint_x(addr);
        if (!endpoint)
            return bind_new_endpoint(s, addr);
        else
            return zmq_bind(s, endpoint);
    } else {
        return zmq_bind(s, addr);
    }
}

int zmq_unbind_x(void* s, const char* addr)
{
    const char* endpoint = 0;
    if (is_x_addr(addr)) {
        endpoint = zmq_endpoint_x(addr);
        if (!endpoint)
            return -1;  // not bound yet
        else
            return zmq_unbind(s, endpoint);
    } else {
        return zmq_unbind(s, addr);
    }
}

int zmq_connect_x(void* s, const char* addr)
{
    const char* endpoint = 0;
    if (is_x_addr(addr)) {
        endpoint = zmq_endpoint_x(addr);
        if (!endpoint)
            return -1;  // not bound yet
        else
            return zmq_connect(s, endpoint);
    } else {
        return zmq_connect(s, addr);
    }
}

int zmq_disconnect_x(void* s, const char* addr)
{
    const char* endpoint = 0;
    if (is_x_addr(addr)) {
        endpoint = zmq_endpoint_x(addr);
        if (!endpoint)
            return -1;  // not bound yet
        else
            return zmq_disconnect(s, endpoint);
    } else {
        return zmq_disconnect(s, addr);
    }
}

int zmq_socket_monitor_x(void* s, const char* addr, int events)
{
    //TODO
    return -1;
}

const char* zmq_endpoint_x(const char* addr)
{
    if (!is_x_addr(addr))
        return addr;

    int i = 0;
    for (; i < s_endpoints_size; ++i) {
        if (!strcmp(addr, s_endpoints[i].addr)) {
            return s_endpoints[i].endpoint;
        }
    }

    if (!s_ipc_client)
        return NULL;

    // send resolve request to ipc server
    char endpoint[256] = {0};
    int len = zmq_send(s_ipc_client, addr, strlen(addr), 0);
    if (len == -1)
        return NULL;  // Interrupted
    len = zmq_recv(s_ipc_client, endpoint, sizeof(endpoint), 0);
    if (len == -1)
        return NULL;  // Interrupted
    endpoint[len] = 0;
    // update local addr-to-endpoint cache
    return insert_endpoint_item(addr, endpoint);
}

int zmq_init_x(void* ctx)
{
    _zmq_init_x(ctx, 0);
}
int _zmq_init_x(void* ctx, int clean)
{
    s_ipc_server = zmq_socket(ctx, ZMQ_REP);
    int rc = zmq_bind(s_ipc_server, s_ipc_endpoint);
    if (rc == -1) {
        if (zmq_errno() == EADDRINUSE) {
            zmq_close(s_ipc_server);
            s_ipc_server = NULL;
            s_ipc_client = zmq_socket(ctx, ZMQ_REQ);
            rc = zmq_connect(s_ipc_client, s_ipc_endpoint);
            if (rc == -1)
                printf("E: failed to connect to '%s' - %s\n", s_ipc_endpoint,
                        strerror(zmq_errno()));
            return rc;
        } else {
            printf("E: failed to bind to '%s' - %s\n", s_ipc_endpoint,
                    strerror(zmq_errno()));
            return -1;
        }
    }
    // start ipc server to resolve ipc addresses
    pthread_t ipc_server_th;
    if (clean)
        pthread_create(&ipc_server_th, NULL, ipc_server_task, ctx);
    else
        pthread_create(&ipc_server_th, NULL, ipc_server_task, NULL);

    return 0;
}

