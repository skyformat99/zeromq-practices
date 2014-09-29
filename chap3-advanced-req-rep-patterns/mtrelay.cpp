// mtrelay.cpp
//
// Multithreaded replay, use PAIR socket with inproc transport to coordinate
// multiple threads.
//
#include <stdio.h>
#include <stdlib.h>
#include <zmq.h>
#include <pthread.h>

static void* step1(void* context) {
    // Connect to step2 and tell it we're ready
    void* xmitter = zmq_socket(context, ZMQ_PAIR);
    zmq_connect(xmitter, "inproc://step2");
    printf("Step 1 ready, signaling step 2\n");
    zmq_send(xmitter, "READY", 5, 0);
    zmq_close(xmitter);

    return NULL;
}

static void* step2(void* context) {
    // Bind inproc socket before starting step1
    void* receiver = zmq_socket(context, ZMQ_PAIR);
    zmq_bind(receiver, "inproc://step2");
    pthread_t th;
    pthread_create(&th, NULL, step1, context);
    // Wait for signal and pass it on
    char buf[16];
    zmq_recv(receiver, buf, 16, 0);
    zmq_close(receiver);
    // Connect to step3 and tell it we're ready
    void* xmitter = zmq_socket(context, ZMQ_PAIR);
    zmq_connect(xmitter, "inproc://step3");
    printf("Step 2 ready, signaling step 3\n");
    zmq_send(xmitter, "READY", 5, 0);
    zmq_close(xmitter);

    return NULL;
}

int main()
{
    void* context = zmq_ctx_new();
    // Bind inproc socket before starting step2
    void* receiver = zmq_socket(context, ZMQ_PAIR);
    zmq_bind(receiver, "inproc://step3");
    pthread_t th;
    pthread_create(&th, NULL, step2, context);
    // Wait for signal
    char buf[16];
    zmq_recv(receiver, buf, 16, 0);
    zmq_close(receiver);

    printf("Test successed!\n");
    zmq_ctx_destroy(context);
    return 0;
}

