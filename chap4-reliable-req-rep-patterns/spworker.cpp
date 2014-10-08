/**
 * @file spworker.cpp
 *
 * @breif Simple Pirate worker
 * Connect REQ socket to spbroker
 * Implement worker part of load-balancing
 */
#include <czmq.h>

#include "time_util.h"

namespace {

const char WORKER_READY[] = "\001";  // Signals worker is ready

}

int main()
{
    zctx_t* ctx = zctx_new();
    void* worker = zsocket_new(ctx, ZMQ_REQ);

    // Set random identity to make tracing easier
    srandom((unsigned int)time(0));
    char identity[10];
    sprintf(identity, "%04x-%04x", randof(0x10000), randof(0x10000));
    zmq_setsockopt(worker, ZMQ_IDENTITY, identity, strlen(identity));
    zsocket_connect(worker, "tcp://127.0.0.1:5556");

    // Tell broker we're ready for work
    printf("I: (%s) worker ready\n", identity);
    zframe_t* frame = zframe_new(WORKER_READY, 1);
    zframe_send(&frame, worker, 0);

    int cycles = 0;
    while (true) {
        zmsg_t* msg = zmsg_recv(worker);
        if (!msg)
            break;  // Interrupted
        // Simulate various problems, after a few cycles
        cycles++;
        if (cycles > 3 && randof(5) == 0) {
            printf("I: (%s) simulating a crash\n", identity);
            zmsg_destroy(&msg);
            break;
        } else if (cycles > 3 && randof(5) == 0) {
            printf("I: (%s) simulating CPU overload\n", identity);
            msleep(3000);
        } else {
            printf("I: (%s) normal reply\n", identity);
            msleep(1000);
        }
        if (msg)
            zmsg_send(&msg, worker);
    }

    zctx_destroy(&ctx);
    return 0;
}
