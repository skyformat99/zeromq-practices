/**
 * @file task_flow.cpp
 *
 * @breif Prototype for the task flow(local and cloud), with ROUTER-to-ROUTER
 * pattern applied.
 */
#include <czmq.h>
#include <stdlib.h>

#include "endpoint.h"
#include "time_util.h"

namespace {

const int NBR_CLIENTS = 10;
const int NBR_WORKERS = 3;
const char WORKER_READY_MSG[] = "\001";  // The worker ready signal

char* self; // Name of the current node, to be configured per node in practice

}

// The client task does a request-reply dialog using a standard sychronous
// REQ socket:

static void* client_task(void* arg)
{
    zctx_t* ctx = zctx_new();
    void* client = zsocket_new(ctx, ZMQ_REQ);
    zsocket_connect(client, localfe_endpoint(self));
    
    while (true) {
        // Send request,  get reply
        zstr_sendf(client, "%s:%08x hello", self, client);
        char* reply = zstr_recv(client);
        if (!reply)
            break; // Interrupted
        printf("I: client %08x got reply: %s\n", client, reply);
        free(reply);
        msleep(1000);  // One request per second
    }
    zctx_destroy(&ctx);
    return NULL;
}

// The worker task plugs into the load-balancer using a REQ socket:

static void* worker_task(void* arg)
{
    zctx_t* ctx = zctx_new();
    void* worker = zsocket_new(ctx, ZMQ_REQ);
    zsocket_connect(worker, localbe_endpoint(self));

    // Tell broker we're ready to work
    zframe_t* frame = zframe_new(WORKER_READY_MSG, 1);
    zframe_send(&frame, worker, 0);
    
    // Process messages(requests) as they arrive
    while (true) {
        zmsg_t* msg = zmsg_recv(worker);
        if (!msg)
            break; // Interrupted
        zframe_print(zmsg_last(msg), "Worker: ");
        char buf[32];
        sprintf(buf, "%s:%08x ok", self, worker);
        zframe_reset(zmsg_last(msg), buf, strlen(buf));
        zmsg_send(&msg, worker);
    }
    zctx_destroy(&ctx);
    return NULL;
}

// The main task begins by setting up its frontend and backend sockets and
// then starting its client and worker tasks:

int main(int argc, char* argv[])
{
    // The first argument is this broker's name, or localfe's port
    // Other arguments are our peers' names
    if (argc < 2) {
#ifdef WIN32
        printf("syntax: task_flow me_port {other_port}...\n");
#else
        printf("syntax: task_flow me {other}...\n");
#endif // WIN32
        return 0;
    }
    self = argv[1];
    printf("I: preparing broker at %s...\n", self);
    srand((unsigned)time(0));
    
#ifdef WIN32
    zsys_init();
#endif // WIN32
    zctx_t* ctx = zctx_new();
    // Bind cloud frontend
    void* cloudfe = zsocket_new(ctx, ZMQ_ROUTER);
    zsocket_set_identity(cloudfe, self);
    zsocket_bind(cloudfe, cloud_endpoint(self));

    // Connect cloud backend to all peers
    void* cloudbe = zsocket_new(ctx, ZMQ_ROUTER);
    zsocket_set_identity(cloudbe, self);
    for (int i = 2; i < argc; ++i) {
        char* peer = argv[i];
        printf("I: connecting to cloud frontend at '%s'...\n", peer);
        zsocket_connect(cloudbe, cloud_endpoint(peer));
    }
    
    // Prepare local frontend and backend
    void* localfe = zsocket_new(ctx, ZMQ_ROUTER);
    void* localbe = zsocket_new(ctx, ZMQ_ROUTER);
    zsocket_bind(localfe, localfe_endpoint(self));
    zsocket_bind(localbe, localbe_endpoint(self));

    // Let the user to tell us when to start
    printf("Press 'Enter' when all brokers are started: ");
    getchar();
    
    // Start local workers and clients
    for (int i = 0; i < NBR_WORKERS; ++i) {
        zthread_new(worker_task, NULL);
    }
    for (int i = 0; i < NBR_CLIENTS; ++i) {
        zthread_new(client_task, NULL);
    }
    
    // Here we handle the request-reply flow.
    // We are using load-balancing to poll workers at all times, and clietns
    // only when there are available workers
    
    // Least recently used queue of available workers
    int capacity = 0;
    zlist_t* workers = zlist_new();
    
    while (true) {
        // First, route any waiting replies from workers
        zmq_pollitem_t backends[] = {
            { localbe, 0, ZMQ_POLLIN, 0 },
            { cloudbe, 0, ZMQ_POLLIN, 0 }
        };
        // Wait indefinitely if there are no workers available
        int rc = zmq_poll(backends, 2, capacity ? 1000 * ZMQ_POLL_MSEC : -1);
        if (rc == -1)
            break;  // Interrupted

        zmsg_t* msg = NULL;
        // Handle reply from local worker
        if (backends[0].revents & ZMQ_POLLIN) {
            msg = zmsg_recv(localbe);
            if (!msg)
                break; // Interrupted
            zframe_t* identity = zmsg_unwrap(msg);
            zlist_append(workers, identity);  // Update availabe workers
            capacity++;
            
            // If it's READY message, don't route the message any further
            zframe_t* frame = zmsg_first(msg);
            //zframe_print(frame, "Incoming worker reply: ");
            if (memcmp (zframe_data(frame), WORKER_READY_MSG, 1) == 0) {
                //zframe_print(identity, "Worker ready: ");
                zmsg_destroy(&msg);
            }
        }
        // Or handle reply from other peer broker
        else if (backends[1].revents & ZMQ_POLLIN) {
            msg = zmsg_recv(cloudbe);
            if (!msg)
                break;
            // We don't use peer broker identity for anything
            zframe_t* identity = zmsg_unwrap(msg);
            zframe_destroy(&identity);
        }
        // Route reply to cloud if it's addressed to a broker
        for (int i = 2; msg && i < argc; ++i) {
            char* peer = argv[i];
            char* data = (char*)zframe_data(zmsg_first(msg));
            size_t size = zframe_size(zmsg_first(msg));
            if (size == strlen(peer) && memcmp(data, peer, size) == 0) {
                zmsg_send(&msg, cloudfe);
                break;  // Actually no need, cause msg would be set to NULL
            }
        }
        // Route reply to client if still needed
        if (msg) {
            //zframe_print(zmsg_last(msg), "Reply: ");
            zmsg_send(&msg, localfe);
        }
        
        // Now we route as many client requests as we have worker capacity for.
        // We may re-route requests from our local frontend, but not from the
        // cloud frontend. We re-route randomly now, just to test things out.
        // In the next version, we'll do this properly by calculating cloud
        // capacity:
        
        while (capacity) {
            zmq_pollitem_t frontends[] = {
                { localfe, 0, ZMQ_POLLIN, 0 },
                { cloudfe, 0, ZMQ_POLLIN, 0 }
            };
            int rc = zmq_poll(frontends, 2, 0);
            assert(rc >= 0);
            int reroutable = 0;
            // We'll do peer borkers first, to prevent starvation
            zmsg_t* msg = NULL;
            if (frontends[1].revents & ZMQ_POLLIN) {
                msg = zmsg_recv(cloudfe);
                reroutable = 0;
            } else if (frontends[0].revents & ZMQ_POLLIN) {
                msg = zmsg_recv(localfe);
                reroutable = 1;
            } else {
                break;  // No work, go back to backends
            }
            
            // If reroutable, send to cloud 20% of the time
            // Here we'd normally use cloud status information
            if (reroutable && argc > 2 && rand() % 5 == 0) {
                // Route to random peer broker
                char* peer = argv[rand() % (argc - 2) + 2];
                zmsg_pushmem(msg, peer, strlen(peer));
                zmsg_send(&msg, cloudbe);
            } else {
                // Route to least-recently-used worker, wrap the selected
                // worker identity in the front of the message to be sent,
                // to instruct the localbe(ROUTER socket) route the message
                // to the specified worker properly.
                //
                // Remeber to update the worker queue and capacity properly
                zframe_t* frame = (zframe_t*)zlist_pop(workers);
                zmsg_wrap(msg, frame);
                zmsg_send(&msg, localbe);
                capacity--;
            }
        }
    }
    
    // When we're done, cleanup properly
    while (zlist_size(workers)) {
        zframe_t* frame = (zframe_t*)zlist_pop(workers);
        zframe_destroy(&frame);
    }
    zlist_destroy(&workers);
    zctx_destroy(&ctx);
    return EXIT_SUCCESS;
}
