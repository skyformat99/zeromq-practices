// cluster/endpoints.cpp
//
#include "endpoints.h"

#include <stdio.h>

struct endpoint_t {
    char name[32];
    int tcp_port_prefix;
};

enum endpoint_type {
    endpoint_type_state = 0
};

static char endpoint_buf[256] = {0};
static struct endpoint_t endpoint_map[] =
{
    { "state", 0 } // state
};

static const char* endpoint(endpoint_type type, const char* self, char* buf=0)
{
    if (!buf)
        buf = endpoint_buf;
    struct endpoint_t* ptr = &endpoint_map[type];
#ifndef WIN32  // ZeroMQ for Windows does not support IPC endpoints
    sprintf(buf, "ipc:///tmp/%s-%s.ipc", self, ptr->name);
#else  // use TCP instead
    sprintf(buf, "tcp://127.0.0.1:%d%s\n", ptr->tcp_port_prefix, self);
#endif // WIN32
    return buf;
}

#define MAKE_ENDPOINT(xxx) \
const char* xxx(const char* self, char* buf) \
{ \
    return endpoint(endpoint_type_##xxx, self, buf); \
}

namespace endpoints {

MAKE_ENDPOINT(state)

}
