/**
 * @file prototype.cpp
 *
 * @breif Broker peering simulation (part 3, put them all together)
 * Prototype the full flow of status and tasks
 */
#include <czmq.h>
#include "czmq_fix.h"
#include <assert.h>
#include <stdlib.h>

#include "endpoint.h"
#include "time_util.h"

namespace {  // Internal

const int NBR_CLIENTS = 1;
const int NBR_WORKERS = 1;
const char WORKER_READY[] = "\001";  // Signal that worker is ready

char* self = 0;

//inline int randof(int n) { return rand() % n; }

// This is the client task. It issues a burst of requests and sleeps for a few
// seconds. This simulates sporadic activity: when a number of clients are
// active at once, local workers should be overloaded. The client uses a REQ
// socket for requests and also pushes statistics to the monitor socket:
void* client_task(void* arg)
{
    srand(zthread_id());

    zctx_t* ctx = zctx_new();
    void* client = zsocket_new(ctx, ZMQ_REQ);
    zsocket_connect(client, localfe_endpoint(self));
    void* monitor = zsocket_new(ctx, ZMQ_PUSH);
    zsocket_connect(monitor, monitor_endpoint(self));

    while (true) {
        msleep(randof(5) * 1000);
        int burst = randof(15);
        while (burst--) {
            char task_id[5];
            sprintf(task_id, "%04x", randof(0x10000));
            // Send request with random hex ID
            zstr_send(client, task_id);
            // Wait at most ten seconds for reply, then complain
            zmq_pollitem_t items[] = { { client, 0, ZMQ_POLLIN, 0 } };
            //int rc = zmq_poll(items, 1, 10 * ZMQ_POLL_MSEC * 1000);
            int rc = zmq_poll(items, 1, -1);
            if (rc == -1)
                break;  // Interrupted
            if (items[0].revents & ZMQ_POLLIN) {
                char* reply = zstr_recv(client);
                if (!reply)
                    break;  // Interrupted
                // Worker is supposed to answer the client with task id
                assert(streq(reply, task_id));
                zstr_sendf(monitor, "%s", reply);
                free(reply);
            } else {
                zstr_sendf(monitor, "E: CLIENT EXIT - lost task %s", task_id);
                return 0;
            }
        }
    }

    zctx_destroy(&ctx);
    return 0;
}

// This is the worker task, which uses a REQ socket to plug into the
// load-balancer.
void* worker_task(void* arg)
{
    srand(zthread_id());

    zctx_t* ctx = zctx_new();
    void* worker = zsocket_new(ctx, ZMQ_REQ);
    zsocket_connect(worker, localbe_endpoint(self));
    // Tell the broker that we're ready before serving any requests
    zframe_t* frame = zframe_new(WORKER_READY, 1);
    zframe_send(&frame, worker, 0);
    // Handle requests
    while (true) {
        zmsg_t* msg = zmsg_recv(worker);
        if (!msg)
            break;  // Interrupted
        // Sleep for 0 or 1 seconds, to simulate the real request handling
        // procedure
        msleep(randof(2) * 1000);
        zmsg_send(&msg, worker);
    }
    zctx_destroy(&ctx);
    return 0;
}

}

// The main task begins by setting up all its sockets.
// The local frontend talks to clients, and the local backend talks to
// workers.
// The cloud frontend talks to other peer brokers as if they were clients,
// and the cloud backend talks to peer brokers as if they were workers.
// The state backend publishes regular state messages, and the state backend
// subscribes to all other state backends to collect these messages.
// Finally, we use a PULL monitor socket to collect all printable messages
// from tasks:
int main(int argc, char* argv[])
{
    // First argument is name of this broker, other arguments are other
    // peers' names
    if (argc < 2) {
#ifndef WIN32
        printf("syntax: %s me {other}...\n", argv[0]);
#else
        printf("syntax: %s ne_port {other_port}...\n", argv[0]);
#endif //WIN32
        return 0;
    }
    self = argv[1];

    printf("I: preparing broker at %s...\n", self);
    srand((unsigned int)time(0));

    zctx_t* ctx = zctx_new();
    // Prepare local frontend and backend
    void* localfe = zsocket_new(ctx, ZMQ_ROUTER);
    zsocket_bind(localfe, localfe_endpoint(self));
    void* localbe = zsocket_new(ctx, ZMQ_ROUTER);
    zsocket_bind(localbe, localbe_endpoint(self));

    // Bind cloud frontend to endpoint
    void* cloudfe = zsocket_new(ctx, ZMQ_ROUTER);
    zsocket_set_identity(cloudfe, self);
    zsocket_bind(cloudfe, cloud_endpoint(self));
    // Connect cloud backend to all peers
    void* cloudbe = zsocket_new(ctx, ZMQ_ROUTER);
    zsocket_set_identity(cloudbe, self);
    for (int i = 2; i < argc; ++i) {
        const char* peer = argv[i];
        printf("I: connecting to cloud frontend at '%s'...\n", peer);
        zsocket_connect(cloudbe, cloud_endpoint(peer));
    }

    // Bind state backend to endpoint
    void* statebe = zsocket_new(ctx, ZMQ_PUB);
    zsocket_connect(statebe, state_endpoint(self));
    // Connect state frontend to all peers
    void* statefe = zsocket_new(ctx, ZMQ_SUB);
    zsocket_set_subscribe(statefe, "");
    for (int i = 2; i < argc; ++i) {
        const char* peer = argv[i];
        printf("I: connecting to state backend at '%s'...\n", peer);
        zsocket_connect(statefe, state_endpoint(self));
    }

    // Prepare monitor socket
    void* monitor = zsocket_new(ctx, ZMQ_PULL);
    zsocket_bind(monitor, monitor_endpoint(self));


    // After binding and connecting all our sockets, start the child tasks -
    // workers and clients:

    for (int i = 0; i < NBR_WORKERS; ++i)
        zthread_new(worker_task, 0);
    for (int i = 0; i < NBR_CLIENTS; ++i)
        zthread_new(client_task, 0);

    // Queue of available workers
    int local_capacity = 0;
    int cloud_capacity = 0;
    zlist_t* available_workers = zlist_new();

    // The main loop has two parts.
    // First, we poll workers and our two service sockets(statefe and
    // monitor), in any case. If we have no ready workers, then there's
    // no point in looking at incoming requests.
    while (true) {
        zmq_pollitem_t primary[] = {
            { localbe, 0, ZMQ_POLLIN, 0 },
            { cloudbe, 0, ZMQ_POLLIN, 0 },
            { statefe, 0, ZMQ_POLLIN, 0 },
            { monitor, 0, ZMQ_POLLIN, 0 }
        };
        // Wait indefintely if there's no available local workers,
        // otherwise, block for at most 1 second
        int rc = zmq_poll(primary, 4,
                local_capacity ? 1000 * ZMQ_POLL_MSEC: -1);
        if (rc == -1)
            break;  // Interrupted

        // Track whether local capacity changes during this iteration
        int previous_capacity = local_capacity;
        zmsg_t* msg = 0;

        // Reply from local worker
        if (primary[0].revents & ZMQ_POLLIN) {
            msg = zmsg_recv(localbe);
            if (!msg)
                break;  // Interrupted
            zframe_t* identity = zmsg_unwrap(msg);
            // The worker must be available after this reply, so add it to
            // the available worker list
            zlist_append(available_workers, identity);
            local_capacity++;

            // Do not route the message further if it's READY message,
            // by destroying it
            zframe_t* frame = zmsg_first(msg);
            if (!memcmp(zframe_data(frame), WORKER_READY, 1))
                zmsg_destroy(&msg);
        }
        // Reply from peer broker
        else if (primary[1].revents & ZMQ_POLLIN) {
            msg = zmsg_recv(cloudbe);
            if (!msg)
                break;  // Interrupted
            // Remove the identity frame added by cloud backend, to route
            // the message back to client frontend
            zframe_t* identity = zmsg_unwrap(msg);
            zframe_destroy(&identity);
        }
        // Route reply to cloud if it's addressed to a peer broker
        for (int i = 2; msg && i < argc; ++i) {
            const char* peer = argv[i];
            char* data = (char*)zframe_data(zmsg_first(msg));
            int size = zframe_size(zmsg_first(msg));
            if (size == strlen(peer) && memcmp(data, peer, size) == 0)
                zmsg_send(&msg, cloudfe);
        }
        // Route reply to client if still need to
        if (msg)
            zmsg_send(&msg, localfe);

        // If we have input messages on statefe or monitor socket, process
        // them immediately
        if (primary[2].revents & ZMQ_POLLIN) {
            char* peer = zstr_recv(statefe);
            char* status = zstr_recv(statefe);
            cloud_capacity = atoi(status);
            free(peer);
            free(status);
        }
        if (primary[3].revents & ZMQ_POLLIN) {
            char* info = zstr_recv(monitor);
            printf("M: %s\n", info);
            free(info);
        }

        // Now route as many clients requests as we can handle. If we have
        // local capacity, then poll both localfe and cloudfe, 'cause
        // requests from cloudfe should only be routed to local workers.
        // If we have cloud capacity only, the poll just localfe.
        // Route requests locally if possible, otherwise, route to cloud
        // peers.
        while (local_capacity + cloud_capacity) {
            zmq_pollitem_t secondary[] = {
                { localfe, 0, ZMQ_POLLIN, 0 },
                { cloudfe, 0, ZMQ_POLLIN, 0 }
            };
            int rc = zmq_poll(secondary, local_capacity ? 2 : 1, 0);
            assert(rc >= 0);

            if (secondary[0].revents & ZMQ_POLLIN)
                msg = zmsg_recv(localfe);
            else if (secondary[1].revents & ZMQ_POLLIN)
                msg = zmsg_recv(cloudfe);
            else
                break;  // No work, go back to primary

            if (local_capacity) {
                // Route to local worker
                zframe_t* worker = (zframe_t*)zlist_pop(available_workers);
                local_capacity--;
                zmsg_wrap(msg, worker);
                zmsg_send(&msg, localbe);
            } else {
                // Route to random broker peer
                const char* rand_peer = argv[randof(argc - 2) + 2];
                zmsg_pushmem(msg, rand_peer, strlen(rand_peer));
                zmsg_send(&msg, cloudbe);
            }
        }

        // Broadcast capacity messages to other peers; to reduce chatter,
        // do this only if local capacity changed
        if (local_capacity != previous_capacity) {
            // Stick our own identity to the envelope
            zstr_sendm(statebe, self);
            // Broadcast the new capacity
            zstr_sendf(statebe, "%d", local_capacity);
        }
    }

    // Clean up when we're done
    while (zlist_size(available_workers)) {
        zframe_t* frame = (zframe_t*)zlist_pop(available_workers);
        zframe_destroy(&frame);
    }
    zlist_destroy(&available_workers);
    zctx_destroy(&ctx);

    return EXIT_SUCCESS;
}
