/**
 * @file asyncsrv.cpp
 *
 * @breif Asynchronous client-to-server (DEALER to ROUTER)
 */
#include <czmq.h>
#include <zmq.hpp>

// This is our client task
// It connects to the server, and then sends a request every onces per second.
// It collects responses as they arrive, and prints them out. We will run
// several client tasks in parallel, each with a different ID(thread id used).
static void* client_task(void* arg)
{
    zmq::context_t ctx;
    zmq::socket_t client(ctx, ZMQ_DEALER);

    // Set indentity to current thread id
    char identity[10];
    sprintf(identity, "%x", zthread_id());
    client.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
    client.connect("tcp://localhost:5570");

    zmq::pollitem_t items[] = {
        { (void*)client, 0, ZMQ_POLLIN, 0 }
    };
    int request_num = 0;
    while (true) {
        // Tick once per second, pulling in arriving messages
        int centitick;
        for (centitick = 0; centitick < 100; ++centitick) {
            zmq::poll(items, 1, 10 * ZMQ_POLL_MSEC);
            if (items[0].revents & ZMQ_POLLIN) {
                zmsg_t *msg = zmsg_recv((void*)client);
                zframe_print(zmsg_last(msg), identity);
                zmsg_destroy(&msg);
            }
        }
        char buf[30];
        sprintf(buf, "request #%d", ++request_num);
        client.send(buf, strlen(buf));
    }
    return NULL;
}


// This is our server task.
// It uses the multithreaded server model to deal requests out to a set of
// workers and route replies back to clients.One worker can handle one request
// at a time but one client can talk to multiple workers at once.
static void server_worker(void* arg, zctx_t* ctx, void* pipe);
static void* server_task(void* arg)
{
    // Frontend socket talks to clients over TCP
    zctx_t* ctx = zctx_new();
    void* frontend = zsocket_new(ctx, ZMQ_ROUTER);
    zsocket_bind(frontend, "tcp://*:5570");
    // Backend socket talks to workers over inproc
    void* backend = zsocket_new(ctx, ZMQ_DEALER);
    zsocket_bind(backend, "inproc://backend");
    // Launch pool of worker threads, exact number is not critical
    int thread_num;
    for (thread_num = 0; thread_num < 5; thread_num++) {
        zthread_fork(ctx, server_worker, NULL);
    }
    // Connect backend to frontend via a proxy
    zmq_proxy(frontend, backend, NULL);
    return NULL;
}

static void server_worker(void* arg, zctx_t* ctx, void* pipe)
{
    // Initialize 'srand' for each thread with thread id
    unsigned int id = zthread_id();
    srand(id);

    void* worker = zsocket_new(ctx, ZMQ_DEALER);
    zsocket_connect(worker, "inproc://backend");
    while (true) {
        // The DEALER socket gives us the reply envelope and message
        zmsg_t* msg = zmsg_recv(worker);
        zframe_t* identity = zmsg_pop(msg);
        zframe_t* content = zmsg_pop(msg);
        // char* id = zframe_strdup(identity);
        // zframe_print(content, id);
        // free(id);
        assert(content);
        zmsg_destroy(&msg);
        // Send 0-4 replies back
        int reply, replies = rand() % 5;
        for (reply = 0; reply < replies; ++reply) {
            // Sleep for some fraction of a second
            zclock_sleep(rand() % 1000 + 1);
            zframe_send(&identity, worker, ZFRAME_REUSE + ZFRAME_MORE);
            zframe_send(&content, worker, ZFRAME_REUSE);
        }
        zframe_destroy(&identity);
        zframe_destroy(&content);
    }
}

// The main thread simply starts several clients and a server, then waits for
// the server to finish.
int main()
{
    zsys_init();
    zthread_new(client_task, NULL);
    zthread_new(client_task, NULL);
    zthread_new(client_task, NULL);
    zthread_new(server_task, NULL);
    zclock_sleep(5 * 1000); // Run for 5 seconds
    return 0;
}
