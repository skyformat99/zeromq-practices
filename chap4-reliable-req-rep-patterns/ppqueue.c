/**
 * @file ppqueue.c
 *
 * @breif Paranoid Pirate queue
 * Load-balancing broker with heartbeat
 */
#include <czmq.h>
#include <assert.h>

static const int HEARTBEAT_LIVENESS = 3;     // 3-5 is reasonable
static const int HEARTBEAT_INTERVAL = 1000;  // msecs
// Paranoid Pirate Protocol constants
static const char PPP_READY[]     = "\001";  // Signals worker is ready
static const char PPP_HEARTBEAT[] = "\002";  // Signals worker heartbeat

// Here we define the worker class; a structure and a set of functions that
// act as constructor, destructor, and methods on worker objects:

typedef struct {
    zframe_t* identity;  // Identity frame of worker
    char* id_string;     // Printable identity
    uint64_t expiry;     // Expires at this time
} worker_t;

// Construct new worker
static worker_t* s_worker_new(zframe_t* identity)
{
    worker_t* self = (worker_t*)zmalloc(sizeof(worker_t));
    assert(self);
    self->identity = identity;
    self->id_string = zframe_strdup(identity);
    self->expiry = zclock_time() + HEARTBEAT_LIVENESS * HEARTBEAT_INTERVAL;
    return self;
}
// Destroy specified worker object, including identity frame
static void s_worker_destroy(worker_t** self_p)
{
    assert(self_p);
    if (*self_p) {
        worker_t* self = *self_p;
        zframe_destroy(&self->identity);
        free(self->id_string);
        free(self);
        *self_p = NULL;
    }
}

// The ready method puts a worker to the end of the ready list, to mimic a
// LRU queue of workers; remove the existing worker from the queue first if
// necessary:

static void s_worker_ready(worker_t* self, zlist_t* workers)
{
    worker_t* worker = (worker_t*)zlist_first(workers);
    while (worker) {
        if (streq(worker->id_string, self->id_string)) {
            zlist_remove(workers, worker);
            s_worker_destroy(&worker);
            break;
        }
        worker = (worker_t*)zlist_next(workers);
    }
    zlist_append(workers, self);
}

// The next method returns the next available worker identity:

static zframe_t* s_workers_next(zlist_t* workers)
{
    worker_t* worker = (worker_t*)zlist_pop(workers);
    zframe_t* identity = worker->identity;
    worker->identity = NULL;  // make sure the returned identity frame do not
                              // get destroied in 's_worker_destroy'
    s_worker_destroy(&worker);
    return identity;
}

// The purge method searchers for and kills expired workers. Workers are
// stored in a LRU-like queue, so we stop at the first alive worker:
//
static void s_workers_purge(zlist_t* workers)
{
    worker_t* worker = (worker_t*)zlist_first(workers);
    while (worker) {
        if (zclock_time() < worker->expiry)
            break;
        zlist_remove(workers, worker);
        s_worker_destroy(&worker);
        worker = (worker_t*)zlist_next(workers);
    }
}


// The main task is a load-balancer with heartbeating on workers so that we
// can detect crashed or blocked worker tasks:
//
int main()
{
    zctx_t* ctx = zctx_new();
    void* backend = zsocket_new(ctx, ZMQ_ROUTER);
    void* frontend = zsocket_new(ctx, ZMQ_ROUTER);
    zsocket_bind(backend, "tcp://*:5556");   // For workers
    zsocket_bind(frontend, "tcp://*:5555");  // For clients

    // List of available workers
    zlist_t* workers = zlist_new();
    // Send out heartbeat at regular interval
    uint64_t heartbeat_at = zclock_time() + HEARTBEAT_INTERVAL;

    while (1) {
        zmq_pollitem_t items[] = {
            { backend, 0, ZMQ_POLLIN, 0 },
            { frontend, 0, ZMQ_POLLIN, 0 }
        };
        int rc = zmq_poll(items, zlist_size(workers) ? 2 : 1,
                HEARTBEAT_INTERVAL * ZMQ_POLL_MSEC);
        if (rc == -1)
            break;  // Interrupted

        if (items[0].revents & ZMQ_POLLIN) {
            zmsg_t* msg = zmsg_recv(backend);
            if (!msg)
                break;  // Interrupted
            zframe_t* identity = zmsg_unwrap(msg);
            worker_t* worker = s_worker_new(identity);
            s_worker_ready(worker, workers);

            if (zmsg_size(msg) == 1) {
                zframe_t* frame = zmsg_first(msg);
                if (memcmp(zframe_data(frame), PPP_READY, 1) != 0 &&
                        memcmp(zframe_data(frame), PPP_HEARTBEAT, 1) != 0) {
                    printf("E: invalid control message from worker");
                    zmsg_dump(msg);
                }
                zmsg_destroy(&msg);
            } else {
                zmsg_send(&msg, frontend);
            }
        }
        if (items[1].revents & ZMQ_POLLIN) {
            // Get the next client request, route to next worker
            zmsg_t* msg = zmsg_recv(frontend);
            if (!msg)
                break;  // Interrupted
            zmsg_push(msg, s_workers_next(workers));
            zmsg_send(&msg, backend);
        }

        // Handle heartbeating after any socket activity. First, send heartbeat
        // signal to any idle workers if it's time. Then, purge any dead
        // workers:
        if (zclock_time() >= heartbeat_at) {
            worker_t* worker = (worker_t*)zlist_first(workers);
            while (worker) {
                printf("I: heartbeat to worker (%s)\n", worker->id_string);
                zframe_send(&worker->identity, backend,
                        ZFRAME_REUSE + ZFRAME_MORE);
                zframe_t* frame = zframe_new(PPP_HEARTBEAT, 1);
                zframe_send(&frame, backend, 0);
                worker = (worker_t*)zlist_next(workers);
            }
            heartbeat_at = zclock_time() + HEARTBEAT_INTERVAL;
        }
        s_workers_purge(workers);
    }
    // Clean up properly when we're done
    while (zlist_size(workers)) {
        worker_t* worker = (worker_t*)zlist_pop(workers);
        s_worker_destroy(&worker);
    }
    zlist_destroy(&workers);

    zctx_destroy(&ctx);
    return 0;
}
