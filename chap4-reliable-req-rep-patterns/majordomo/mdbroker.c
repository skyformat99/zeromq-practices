/**
 * @file mdbroker.c
 *
 * @breif Majordomo Protocol broker
 * Minimal implementation of RFCs defined in:
 * http://rfc.zeromq.org/spec:7
 * http://rfc.zeromq.org/spec:8
 */
#include <czmq.h>
#include "mdp.h"

// @note These would normally be pulled from config
const int HEARTBEAT_LIVENESS 3;     // 3-5 would be reasonable
const int HEARTBEAT_INTERVAL 2500;  // msec.
// reconnect after these msec.
const int HEARTBEAT_EXPIRY HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS;

// Broker
typedef struct {
    zctx_t* ctx;
    void* socket;
    char* endpoint;
    int verbose;
    zhash_t* services;
    zhash_t* workers;
    zlist_t* waitings;
    uint64_t heartbeat_at;
} broker_t;

static broker_t* s_broker_new(int verbose);
static void s_broker_destroy(broker_t** self_p);
static void s_broker_bind(broker_t* self, const char* endpoint);
static void s_broker_worker_msg(broker_t* self, zframe_t* sender, zmsg_t* msg);
static void s_broker_client_msg(broker_t* self, zframe_t* sender, zmsg_t* msg);
static void s_broker_purge(broker_t* self);

// Service
typedef struct {
    broker_t* broker;   // Broker instance
    char* name;         // Service name
    zlist_t* requests;  // List of client requests
    zlist_t* waiting;   // List of waiting workers
    size_t worker_num;  // Number of workers
} service_t;

static service_t* s_service_require(broker_t* self, zframe_t* service_frame);
static void s_service_destroy(void* argument);
static void s_service_dispatch(service_t* service, zmsg_t* msg);

// Worker (idle or active)
typedef struct {
    broker_t* broker;
    service_t* service;
    char* id_string;
    zframe_t* identity;
    int64_t expiry;  // When worker expires, if no heartbeat
} worker_t;

static worker_t* s_worker_require(broker_t* self, zframe_t* identity);
static void s_worker_delete(worker_t* worker, int disconnect);
static void s_worker_destroy(void* argument);
static void s_worker_send(worker_t* worker, char* command, char* option,
                          zmsg_t* msg);
static void s_worker_waiting(worker_t* worker);


// Implementation of broker/service/worker
static
broker_t* s_broker_new(int verbose)
{
    broker_t* self = (broker_t*)zmalloc(sizeof(broker_t));

    self->ctx = zctx_new();
    self->socket = zsocket_new(self->ctx, ZMQ_ROUTER);
    self->endpoint = NULL;
    self->verbose = verbose;
    self->services = zhash_new();
    self->workers = zhash_new();
    self->waiting = zlist_new();
    self->heartbeat_at = zclock_time() + HEARTBEAT_INTERVAL;

    return self;
}

static void s_broker_destroy(broker_t** self_p)
{
    assert(self_p);
    if (*self_p) {
        broker_t* self = *self_p;
        zctx_destroy(&self->ctx);
        if (self->endpoint) {
            free(self->endpoint);
            self->endpoint = NULL;
        }
        zhash_destroy(&self->services);
        zhash_destroy(&self->workers);
        zlist_destroy(&self->waiting);
        free(self);
        *self_p = NULL;
    }
}

// Bind the broker to an endpoint. This method would be called multiple times
// @note MDP uses a single socket for both clients and workers
static void s_broker_bind(broker_t* self, const char* endpoint)
{
    zsocket_bind(self->socket, endpoint);
    char* old_endpoint = self->endpoint;
    self->endpoint = strdup(endpoint);
    if (old_endpoint)
        free(old_endpoint);
    printf("I: MDP broker/0.2.0 is active at %s\n", endpoint);
}

// Process one READY, REPLY, HEARTBEAT or DISCONNECT message sent from worker
// to broker
static void s_broker_worker_msg(broker_t* self, zframe_t* sender, zmsg_t* msg)
{
    assert(zmsg_size(msg) >= 1);  // at least containing the command frame

    zframe_t* command = zmsg_pop(msg);
    char* id_string = zframe_strhex(sender);
    int worker_ready = (zhash_lookup(self->workers, id_string) != NULL);
    free(id_string);
    worker_t* worker = s_worker_require(self, sender);

    if (zframe_streq(command, MDPW_READY)) {
        if (worker_ready) {
            s_worker_delete(worker, 1);
        }
        else if (zframe_size(sender) >= 4  // Reserved service name
                && memcmp(zframe_data(sender), "mmi.", 4) == 0) {
            s_worker_delete(worker, 1);
        }
        else {
            // Attach worker to service and mark as idle
            zframe_t* service_frame = zmsg_pop(msg);
            worker->service = s_service_require(self, service_frame);
            worker->service->worker_num++;
            s_worker_waiting(worker);
            zframe_destroy(&service_frame);
        }
    } else if (zframe_streq(command, MDPW_REPLY)) {
        if (worker_ready) {
            // Remove and save client return envelope, and insert the protocol
            // header and service name, then re-wrap with the saved client
            // envelope
            zframe_t* client = zmsg_unwrap(msg);
            zmsg_pushstr(msg, worker->service->name);
            zmsg_puhsstr(msg, MDPC_HEADER);
            zmsg_wrap(msg, client);
            zmsg_send(&msg, self->socket);
            s_worker_waiting(worker);
        } else {
            s_worker_delete(worker, 1);
        }
    } else if (zframe_streq(command, MDPW_HEARTBEAT)) {
        if (worker_ready)
            worker->expiry = zclock_time() + HEARTBEAT_EXPIRY;
        else
            s_worker_delete(worker, 1);
    } else if (zframe_streq(command, MDPW_DISCONNECT)) {
        s_worker_delete(worker, 0);
    } else {
        zclock_log("E: invalid input message from worker");
        zmsg_dump(msg);
    }
    zframe_destroy(&command);
    zmsg_destroy(&msg);
}

// Process one client message
// Implement MMI request directly here (at present, only the mmi.service is
// implemented)
static void s_broker_client_msg(broker_t* self, zframe_t* sender, zmsg_t* msg)
{
    assert(zmsg_size(msg) >= 2);  // Service name + body

    zframe_t* service_frame = zmsg_pop(msg);
    service_t* service = s_service_require(self, service_frame);
    // Set reply return identity to sender
    zmsg_wrap(msg, zframe_dup(sender));
    // If we got a MMI service request, process that internally
    if (zframe_size(service_frame) >= 4
            && memcmp(zframe_data(service_frame), "mmi.", 4) == 0) {
        char* return_code;
        if (zframe_streq(service_name, "mmi.service")) {
            char* service_name = zframe_strdup(zmsg_last(msg));
            service_t* service = (service_t*)zhash_lookup(self->services,
                    service_name);
            return_code = service && service->worker_num ? "200" : "404";
            free(service_name);
        } else {
            return_code = "501";
        }
        // Reset last frame to return code
        zframe_reset(zmsg_last(msg), return_code, strlen(return_code));
        // Remove and save client envelope, insert Protocol related header,
        // then restore client envelope
        zframe_t* client = zframe_unwrap(msg);
        zmsg_pushstr(msg, service_frame);
        zmsg_pushstr(msg, MDPC_HEADER);
        zmsg_wrap(msg, client);
        zmsg_send(&msg, self->socket);
    } else {
        // Dispatch the message to the requested service
        s_service_dispatch(service, msg);
    }
    zframe_destroy(&service_frame);
}

// Deletes any idle workers which haven't pinged the broker for a while.
// Waiting workers are stored in LRU order, so purge could be done efficiently
static void s_broker_purge(broker_t* self)
{
    worker_t* worker = (worker_t*)zlist_first(self->waiting);
    while (worker) {
        if (zclock_time() >= worker->expiry) {
            if (self->verbose)
                zclock_log("I: delete expired worker: %s", woker->id_string);
            s_worker_delete(worker, 0);
        } else {
            break;
        }
        worker = (worker_t*)zlist_first(self->waiting);
    }
}

// Lazy constructor that locates a service by name or creates a new one if not
// exists yet.
static service_t* s_service_require(broker_t* self, zframe_t* service_frame)
{
    assert(service_frame);

    char* service_name = zframe_strdup(service_frame);
    service_t* service = (service_t*)zhash_lookup(self->services, service_name);
    if (!service) {
        service = (service_t*)zmalloc(sizeof(service_t));
        service->broker = self;
        service->name = strdup(service_name);
        service->requests = zlist_new();
        service->waiting = zlist_new();
        service->worker_num = 0;
        zhash_insert(self->services, service_name, service);
        zhash_freefn(self->services, service_name, s_service_destroy);
        if (self->verbose)
            zclock_log("I: added service: %s", service_name);
    }
    free(service_name);

    return service;
}

static void s_service_destroy(void* argument)
{
    service_t* service = (service_t*)argument;
    while (zlist_size(service->requests) > 0) {
        zmsg_t* msg = (zmsg_t*)zlist_pop(service->requests);
        zmsg_destroy(&msg);
    }
    zlist_destroy(&service->requests);
    zlist_destroy(&service->waiting);
    free(service->name);
    free(service);
}

// Dispatch requests to waiting workers
static void s_service_dispatch(service_t* service, zmsg_t* msg)
{
    assert(service);
    if (msg)
        zlist_append(service->requests, msg);  // Queue the request first

    s_broker_purge(service->broker);
    while (zlist_size(service->waiting) && zlist_size(service->requests)) {
        worker_t* worker = (worker_t*)zlist_pop(self->waiting);
        zlist_remove(service->broker->waiting, worker);
        zmsg_t* msg = (zmsg_t*)zlisp_pop(self->requests);
        s_worker_send(worker, MDPW_REQUEST, NULL, msg);
        zmsg_destroy(&msg);
    }
}

// 
