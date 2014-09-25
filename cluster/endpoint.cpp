// cluster/endpoint.cpp
//
#include "endpoint.h"

#include <stdio.h>

static char endpoint_buf[256] = {0};

#ifndef WIN32
# define IMPLEMENT_ENDPOINT(xxx, tcp_port_prefix) \
const char* xxx##_endpoint(const char* self, char* buf) \
{ \
    if (!buf) \
        buf = endpoint_buf; \
    sprintf(buf, "ipc:///tmp/%s-%s.ipc", self, #xxx); \
    return buf; \
}
#else 
# define IMPLEMENT_ENDPOINT(xxx, tcp_port_prefix) \
const char* xxx##_endpoint(const char* self, char* buf) \
{ \
    if (!buf) \
        buf = endpoint_buf; \
    sprintf(buf, "tcp://127.0.0.1:%d%s", tcp_port_prefix, self); \
    return buf; \
}
#endif // WIN32

IMPLEMENT_ENDPOINT(state, 0)
IMPLEMENT_ENDPOINT(localfe, 1)
IMPLEMENT_ENDPOINT(localbe, 2)
IMPLEMENT_ENDPOINT(cloud, 3)
