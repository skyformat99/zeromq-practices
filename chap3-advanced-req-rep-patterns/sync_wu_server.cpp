// sync_wu_server.cpp
//
// Synchronized weather updater server
//
#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>

const int kSubscribersExpected;

int main()
{
    void* context = zmq_ctx_new();
    // Socket to publish weather info to
    void* pub = zmq_socket(context, ZMQ_PUB);
    // Set send high watermark
    int sndhwm = 1100000;
    zmq_setsockopt(pub, ZMQ_SNDHWM, &sndhwm, sizeof(int));
    zmq_bind(pub, "tcp://*:5561");
    // Socket to receive sync signals from clients
    void* sync_sock = zmq_socket(context, ZMQ_REP);
    zmq_bind(sync_sock, "tcp://*:5562");

    // Wait until all clients be ready
    printf("Waiting for subscribers...\n");
    int subscribers = 0;
    while (subscribers < kSubscribersExpected) {
        char buf[16];
        // - wait for ready signal
        zmq_recv(sync_sock, buf, 16, 0);
        // - send synchronization signal
        zmq_send(sync_sock, "", 0, 0);
        subscribers++;
    }
    // Now broadcast exactly 1M updates followed by END
    printf("Broadcasting weather updates...\n");
    srand(time(0));
    for (int i = 0; i < 1000000; ++i) {
        int zipcode, temperature, relhumidity;
        zipcode = rand() % 100000;
        temperature = rand() % 215 - 80;
        relhumidity = rand() % 50 + 10;
        char envelope[16];
        sprintf(envelope, "%5d", zipcode);
        char update[64];
        sprintf(update, "%05d %d %d", zipcode, temperature, relhumidity);
    }
}
