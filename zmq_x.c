// zmq_x.c
//
#include "zmq_x.h"

#include <zmq.h>

#define MAX_ENDPOINTS 100

struct addr_endpoint_t {
    char addr[256];
    char endpoint[32];
};
static struct addr_endpoint_t s_endpoints[MAX_ENDPOINTS];
static int s_endpoints_size = 0;

static int bind_new_endpoint(void* s, const char* addr)
{
    int rc = 0;
    rc = zmq_bind(s, "tcp://*:*");  // use "tcp://" instead
    if (rc != 0)
        return rc;
    char last_endpoint[256];
    size_t len = 256;
    rc = zmq_getsockopt(s, ZMQ_LAST_ENDPOINT, last_endpoint, &len);
    if (rc != 0)
        return rc;
    last_endpoint[len] = 0;

    struct addr_endpoint_t* t = s_endpoints + s_endpoints_size;
    strcpy(t->addr, addr);
    strcpy(t->endpoint, last_endpoint);
    s_endpoints_size++;
    return 0;
}

static const char s_ipc_head[] = "ipc://";
static const char s_inproc_head[] = "inproc://";

static int is_x_addr(const char* addr)
{
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
    for (i = 0; i < s_endpoints_size; ++i) {
        if (!strcmp(addr, s_endpoints[i].addr)) {
            return s_endpoints[i].endpoint;
        }
    }
    return 0;
}

