// mdcliapi.c
//
// mdcliapi class - Majordomo Protocol Client API
// Implements the MDP/Client spec at http://rfc.zeromq.org/spec:7.
//
#include "mdcliapi.h"

#include <czmq.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MDPC_CLIENT "MDPC01"

struct _mdcli_t {
    zctx_t* ctx;
    char* broker;
    void* client;
    int verbose;
    int timeout;
    int retries;
};

static
void* s_mdcli_connect_to_broker(mdcli_t* self)
{
    if (self->client)
        zsocket_destroy(self->ctx, self->client);
    self->client = zsocket_new(self->ctx, ZMQ_REQ);
    zsocket_connect(self->client, self->broker);
    if (self->verbose)
        zclock_log("I: connecting to broker at %s...", self->broker);
}

mdcli_t* mdcli_new(const char* broker, int verbose)
{
    assert(broker);
    mdcli_t* self = (mdcli_t*)zmalloc(sizeof(mdcli_t));
    self->ctx = zctx_new();
    self->broker = strdup(broker);
    self->client = NULL;
    self->verbose = verbose;
    self->timeout = 2500;
    self->retries = 3;

    s_mdcli_connect_to_broker(self);
    return self;
}

void mdcli_destroy(mdcli_t** self_p)
{
    assert(self_p);
    if (*self_p) {
        mdcli_t* self = *self_p;
        zctx_destroy(&self->ctx);
        free(self->broker);
        free(self);
        *self_p = NULL;
    }
}

void mdcli_set_timeout(mdcli_t* self, int timeout)
{
    assert(self);
    self->timeout = timeout;
}

void mdcli_set_retries(mdcli_t* self, int retries)
{
    assert(self);
    self->retries = retries;
}

zmsg_t* mdcli_send(mdcli_t* self, const char* service, zmsg_t** request_p)
{
    assert(self);
    assert(request_p);
    zmsg_t* request = *request_p;

    // Prefix request with protocol frames
    // Frame 1: "MDPCxy" (six bytes, MDP/Client x.y)
    // Frame 2: Service name (printable string)
    zmsg_pushstr(request, MDPC_CLIENT);
    zmsg_pushstr(request, service);
    if (self->verbose) {
        zclock_log("I: sending request to '%s' service...", service);
        zmsg_dump(request);
    }

    int retries_left = self->retries;
    // Poll for reply
    while (retries_left && !zsys_interrupted) {
        zmsg_t* msg = zmsg_dup(request);
        zmsg_send(&msg, self->client);

        zmq_pollitem_t items[] = {
            { self->client, 0, ZMQ_POLLIN, 0 }
        };
        int rc = zmq_poll(items, 1, self->timeout * ZMQ_POLL_MSEC);
        if (rc == -1)
            break;  // Interrupted

        if (items[0].revents & ZMQ_POLLIN) {
            zmsg_t* reply = zmsg_recv(self->client);
            if (self->verbose) {
                zclock_log("I: received reply:");
                zmsg_dump(reply);
            }
            // Protocol check
            assert(zmsg_size(reply) >= 3);
            zframe_t* header = zmsg_pop(reply);
            assert(zframe_streq(header, MDPC_CLIENT));
            zframe_destroy(&header);

            zframe_t* reply_service = zmsg_pop(reply);
            assert(zframe_streq(reply_service, service));
            zframe_destroy(&reply_service);

            zmsg_destroy(&request);
            return reply;
        } else if (--retries_left) {
            if (self->verbose)
                zclock_log("W: no reply within %dms, reconnecting...", self->timeout);
            s_mdcli_connect_to_broker(self);
        } else {
            if (self->verbose)
                zclock_log("E: permanent error, abandoning");
            break;
        }
    }
    if (zctx_interrupted)
        printf("W: interrupt received, killing client...\n");

    zmsg_destroy(&request);
    return NULL;
}
