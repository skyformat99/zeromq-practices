// czmq_x.c
//
#include "czmq_x.h"

#include <czmq.h>
#include <stdio.h>
#include <stdarg.h>

#include "zmq_x.h"

// Interfaces
//
int zsocket_bind_x(void* self, const char* format, ...)
{
    char addr[256];
    va_list argptr;
    va_start(argptr, format);
    int addr_len = vsnprintf(addr, 256, format, argptr);
    va_end(argptr);

    const char* endpoint = zmq_endpoint_x(addr);
    if (endpoint == addr) {  // not x addr
        return zsocket_bind(self, addr);
    } else if (!endpoint) {  // not mapped yet
        int rc = zmq_bind_x(self, addr);
        if (rc != 0)
            return -1;
        endpoint = zmq_endpoint_x(addr);
        return atoi(strrchr(endpoint, ':') + 1);
    } else {  // bound already
        return -1;
    }
}

int zsocket_unbind_x(void* self, const char* format, ...)
{
    char addr[256];
    va_list argptr;
    va_start(argptr, format);
    int addr_len = vsnprintf(addr, 256, format, argptr);
    va_end(argptr);

    const char* endpoint = zmq_endpoint_x(addr);
    if (endpoint == addr)  // not x addr
        return zsocket_unbind(self, addr);
    else if (!endpoint)    // not mapped yet
        return -1;
    else
        return zmq_unbind_x(self, addr);
}

int zsocket_connect_x(void* self, const char* format, ...)
{
    char addr[256];
    va_list argptr;
    va_start(argptr, format);
    int addr_len = vsnprintf(addr, 256, format, argptr);
    va_end(argptr);

    const char* endpoint = zmq_endpoint_x(addr);
    if (endpoint == addr)  // not x addr
        return zsocket_connect(self, addr);
    else if (!endpoint)    // not mapped yet
        return -1;
    else
        return zmq_connect_x(self, addr);
}

int zsocket_disconnect_x(void* self, const char* format, ...)
{
    char addr[256];
    va_list argptr;
    va_start(argptr, format);
    int addr_len = vsnprintf(addr, 256, format, argptr);
    va_end(argptr);

    const char* endpoint = zmq_endpoint_x(addr);
    if (endpoint == addr)  // not x addr
        return zsocket_disconnect(self, addr);
    else if (!endpoint)    // not mapped yet
        return -1;
    else
        return zmq_disconnect_x(self, addr);
}

int czmq_init_x(zctx_t* ctx)
{
    //void* context = (void*)(*((int*)ctx));
    //return zmq_init_x(context);
    void* context = zmq_ctx_new();
    return _zmq_init_x(context, 1);
}
