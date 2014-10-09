/**
 * @file ppworker.c
 *
 * @breif Paranoid Pirate worker
 * With heartbeat and queue reconnect
 */
#include <czmq.h>

static const int HEARTBEAT_LIVENESS = 3;     // 3-5 is reasonable
static const int HEARTBEAT_INTERVAL = 1000;  // msecs
static const int INTERVAL_INIT = 1000;   // Initial reconnect interval
static const int INTERVAL_MAX = 32000;   // After exponential backoff

// Paranoid Pirate Protocol constants
static const char PPP_READY[] = "\001";  // Signals worker is ready
static const char PPP_HEARTBEAT[] = "\002";  // Signals worker heartbeat

static char identity[10] = {0};

// Helper function that returns a ready worker socket
static void* s_worker_socket(zctx_t* ctx)
{
    void* worker = zsocket_new(ctx, ZMQ_DEALER);

    if (!identity[0])
        sprintf(identity, "%04x-%04x", randof(0x10000), randof(0x10000));

    zmq_setsockopt(worker, ZMQ_IDENTITY, identity, strlen(identity));
    zsocket_connect(worker, "tcp://127.0.0.1:5556");

    // Tell the queue that we're ready
    zframe_t* ready = zframe_new(PPP_READY, 1);
    zframe_send(&ready, worker, 0);

    return worker;
}

int main()
{
    zctx_t* ctx = zctx_new();
    srandom((unsigned int)time(0));
    void* worker = s_worker_socket(ctx);
    printf("I: (%s) worker ready\n", identity);

    int liveness = HEARTBEAT_LIVENESS;
    int interval = INTERVAL_INIT;

    int cycles = 0;
    uint64_t heartbeat_at = zclock_time() + HEARTBEAT_INTERVAL;
    while (1) {
        zmq_pollitem_t items[] = { { worker, 0, ZMQ_POLLIN, 0 } };
        int rc = zmq_poll(items, 1, HEARTBEAT_INTERVAL * ZMQ_POLL_MSEC);
        if (rc == -1)
            break;  // Interrupted

        if (items[0].revents & ZMQ_POLLIN) {
            zmsg_t* msg = zmsg_recv(worker);
            if (!msg)
                break;

            // Message from queue
            // 3-part envelope + content -> request
            // 1-part HEARTBEAT -> heartbeat
            if (zmsg_size(msg) == 3) {
                cycles++;
                if (cycles > 3 && randof(5) == 0) {
                    printf("I: simulating a crash\n");
                    zmsg_destroy(&msg);
                    break;
                } else if (cycles > 3 && randof(5) == 0) {
                    printf("I: simulating CPU overload\n");
                    zclock_sleep(3000);
                }
                printf("I: normal reply\n");
                zclock_sleep(1000);
                zmsg_send(&msg, worker);
                liveness = HEARTBEAT_LIVENESS;
            } else if (zmsg_size(msg) == 1) {
                zframe_t* frame = zmsg_first(msg);
                if (memcmp(zframe_data(frame), PPP_HEARTBEAT, 1) == 0) {
                    liveness = HEARTBEAT_LIVENESS;
                } else {
                    printf("E: invalid control message\n");
                    zmsg_dump(msg);
                }
                zmsg_destroy(&msg);
            } else {
                printf("E: invalid message\n");
                zmsg_dump(msg);
                zmsg_destroy(&msg);
            }
            interval = INTERVAL_INIT;
        } else
        // If the queue hasn't sent us heartbeats for a while, destroy the
        // socket and reconnect. This is the simplest and most brutal way
        // of discarding any messages we might have sent in the meantime:
        if (--liveness == 0) {
            printf("W: heartbeat failure, can't reach queue\n");
            printf("W: reconnecting in %d msec...\n", interval);
            zclock_sleep(interval);

            if (interval < INTERVAL_MAX)
                interval *= 2;
            zsocket_destroy(ctx, worker);
            worker = s_worker_socket(ctx);
            liveness = HEARTBEAT_LIVENESS;
        }
        // Send heartbeat to the queue if it's time
        if (zclock_time() > heartbeat_at) {
            heartbeat_at = zclock_time() + HEARTBEAT_INTERVAL;
            printf("I: worker heartbeat\n");
            zframe_t* heartbeat = zframe_new(PPP_HEARTBEAT, 1);
            zframe_send(&heartbeat, worker, 0);
        }
    }

    zctx_destroy(&ctx);
    return 0;
}
