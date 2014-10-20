// mdwrkapi.c
//
// Implements the MDP/Worker part of http://rfc.zeromq.org/spec:7.
//
#include "mdwrkapi.h"

#include <stdio.h>
#include <assert.h>

#include "mdp.h"

#define HEARTBEAT_LIVENESS 3
#define HEARTBEAT_INTERVAL 2000
#define RECONNECT_DELAY_INIT 2000
#define RECONNECT_DELAY_MAX  32000

struct _mdwrk_t {
    zctx_t* ctx;
    char* broker;
    char* service;
    void* worker;
    int verbose;
    //  heartbeat rel.
    uint64_t heartbeat_at;  // when to send heartbeat
    size_t liveness;        // how many attempt left
    int heartbeat_intv;
    int reconnect_delay;

    int expect_reply;
    zframe_t* reply_to;
};

// send message to broker
// if no msg msg is provide, creates new one internally
static
void s_mdwrk_send_to_broker(mdwrk_t* self, char* command, char* option, 
                            zmsg_t* msg)
{
    assert(self->worker);
    msg = msg ? zmsg_dup(msg) : zmsg_new();
    if (option)
        zmsg_pushstr(msg, option);
    zmsg_pushstr(msg, command);
    // stack protocol evelope to the start of message
    zmsg_pushstr(msg, MDPW_HEADER);
    zmsg_pushstr(msg, "");
    if (self->verbose) {
        zclock_log("I: sending %s to broker", mdps_commands[(int) *command]);
        zmsg_dump(msg);
    }
    zmsg_send(&msg, self->worker);
}

// connect or reconnect to broker
static
void s_mdwrk_connect_to_broker(mdwrk_t* self)
{
    if (self->worker)
        zsocket_destroy(self->ctx, self->worker);
    self->worker = zsocket_new(self->ctx, ZMQ_DEALER);
    if (self->verbose)
        zclock_log("I: connecting to broker at %s...", self->broker);
    zsocket_connect(self->worker, self->broker);
    // register service with broker
    s_mdwrk_send_to_broker(self, MDPW_READY, self->service, NULL);
    // if liveness hits zero, broker is considered disconnected
    self->liveness = HEARTBEAT_LIVENESS;
    self->heartbeat_at = zclock_time() + self->heartbeat_intv;
}

// constructor
mdwrk_t* mdwrk_new(const char* broker, const char* service, int verbose)
{
    assert(broker);
    assert(service);

    mdwrk_t* self = (mdwrk_t*)zmalloc(sizeof(mdwrk_t));
    self->ctx = zctx_new();
    self->broker = strdup(broker);
    self->service = strdup(service);
    self->verbose = verbose;
    self->heartbeat_intv = HEARTBEAT_INTERVAL;     // msec
    self->reconnect_delay = RECONNECT_DELAY_INIT;  // msec
    self->expect_reply = 0;
    self->reply_to = NULL;

    s_mdwrk_connect_to_broker(self);
    return self;
}

// destructor
void mdwrk_destroy(mdwrk_t** self_p)
{
    assert(self_p);
    if (*self_p) {
        mdwrk_t* self = *self_p;
        zctx_destroy(&self->ctx);
        free(self->broker);
        free(self->service);
        free(self);
        *self_p = NULL;
    }
}

// configure the worker
void mdwrk_set_heartbeat_intv(mdwrk_t* self, int heartbeat_intv)
{
    assert(self);
    self->heartbeat_intv = heartbeat_intv;
}
void mdwrk_set_reconnect_delay(mdwrk_t* self, int reconnect_delay)
{
    assert(self);
    self->reconnect_delay = reconnect_delay;
}

// flush last pending reply to broker, and receive a request
// @note this interface is a little misnamed
zmsg_t* mdwrk_recv(mdwrk_t* self, zmsg_t** reply_p)
{
    assert(self->worker);

    if (reply_p && *reply_p) {
        zmsg_t* reply = *reply_p;
        zmsg_wrap(reply, self->reply_to);
        s_mdwrk_send_to_broker(self, MDPW_REPLY, NULL, reply);
        zmsg_destroy(&reply);
    }

    while (!zsys_interrupted) {
        zmq_pollitem_t items[] = {
            { self->worker, 0, ZMQ_POLLIN, 0 }
        };
        int rc = zmq_poll(items, 1, self->heartbeat_intv * ZMQ_POLL_MSEC);
        if (rc == -1)
            break;

        if (items[0].revents & ZMQ_POLLIN) {
            zmsg_t* msg = zmsg_recv(self->worker);
            if (!msg)
                break;

            self->liveness = HEARTBEAT_LIVENESS;
            self->reconnect_delay = RECONNECT_DELAY_INIT;

            // protocol envelope check
            assert(zmsg_size(msg) >= 6);
            zframe_t* empty = zmsg_pop(msg);
            assert(zframe_size(empty) == 0);
            zframe_destroy(&empty);
            zframe_t* header = zmsg_pop(msg);
            assert(zframe_streq(header, MDPW_HEADER));
            zframe_destroy(&header);
            zframe_t* command = zmsg_pop(msg);
            if (zframe_streq(command, MDPW_REQUEST)) {
                zframe_destroy(&command);
                self->reply_to = zmsg_unwrap(msg);
                return msg;
            } else if (zframe_streq(command, MDPW_HEARTBEAT)) {
                // heartbeat from broker
            } else if (zframe_streq(command, MDPW_DISCONNECT)) {
                s_mdwrk_connect_to_broker(self);
            }
            zframe_destroy(&command);
            zmsg_destroy(&msg);

        } else if (--self->liveness) {
            if (self->verbose)
                zclock_log("W: no heartbeat received from broker within %dms",
                        self->heartbeat_intv);
        } else {
            if (self->verbose)
                zclock_log("E: broker considered offline, reconnect after %dms",
                        self->reconnect_delay);
            s_mdwrk_send_to_broker(self, MDPW_DISCONNECT, NULL, NULL);
            zclock_sleep(self->reconnect_delay);
            if (self->reconnect_delay < RECONNECT_DELAY_MAX)
                self->reconnect_delay *= 2;
            s_mdwrk_connect_to_broker(self);
        }

        // send heartbeat if it's time
        if (zclock_time() >= self->heartbeat_at) {
            s_mdwrk_send_to_broker(self, MDPW_HEARTBEAT, NULL, NULL);
            self->heartbeat_at = zclock_time() + self->heartbeat_intv;
        }
    }

    if (zsys_interrupted)
        printf("W: interrupt received, killing worker...\n");
    return NULL;
}
