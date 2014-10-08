/**
 * @file spqueue.cpp
 *
 * @breif Simple Pirate queue (broker)
 * This is identical to the load-balancing pattern, with no reliability
 * mechanisms. It depends on the client for recovery.
 * Runs forever.
 */
#include <czmq.h>
#include <stdio.h>
#include <string.h>

namespace {

const char WORKER_READY[] = "\001";  // Signals worker is ready

}

int main()
{
    zctx_t* ctx = zctx_new();
    void* frontend = zsocket_new(ctx, ZMQ_ROUTER);
    void* backend = zsocket_new(ctx, ZMQ_ROUTER);
    zsocket_bind(frontend, "tcp://*:5555");  // For clients
    zsocket_bind(backend, "tcp://*:5556");   // For workers

    // Queue of available workers
    zlist_t* available_workers = zlist_new();

    while (true) {
        zmq_pollitem_t items[] = {
            { backend, 0, ZMQ_POLLIN, 0 },
            { frontend, 0, ZMQ_POLLIN, 0 }
        };
        int rc = zmq_poll(items, zlist_size(available_workers) ? 2 : 1, -1);
        if (rc == -1)
            break;  // Interrupted

        if (items[0].revents & ZMQ_POLLIN) {
            zmsg_t* msg = zmsg_recv(backend);
            // Strip the identity frame added by REQ socket
            zframe_t* identity = zmsg_unwrap(msg);
            zlist_push(available_workers, identity);
            // Check the first frame for READY signal
            zframe_t* frame = zmsg_first(msg);
            if (memcmp(zframe_data(frame), WORKER_READY, 1) == 0)
                zmsg_destroy(&msg);
            else
                zmsg_send(&msg, frontend);
        } else if (items[1].revents & ZMQ_POLLIN) {
            zmsg_t* msg = zmsg_recv(frontend);
            if (msg) {
                zmsg_wrap(msg, (zframe_t*)zlist_pop(available_workers));
                zmsg_send(&msg, backend);
            }
        }
    }

    // Cleanup
    while (zlist_size(available_workers)) {
        zframe_t* identity = (zframe_t*)zlist_pop(available_workers);
        zframe_destroy(&identity);
    }
    zlist_destroy(&available_workers);
    zctx_destroy(&ctx);
    return 0;
}
