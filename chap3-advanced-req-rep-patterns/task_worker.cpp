// task_worker.cpp
//
#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#define MSLEEP(ms) Sleep(ms)
#else  // Li/U-nix
#include <unistd.h>
#define MSLEEP(ms) sleep(ms*1000)
#endif // _WIN32

int main(int argc, char* argv[])
{
    void* context = zmq_ctx_new();
    // Socket to read task from
    void* receiver = zmq_socket(context, ZMQ_PULL);
    zmq_connect(receiver, "tcp://localhost:5557");

    // Socket to send task result to
    void* sink = zmq_socket(context, ZMQ_PUSH);
    zmq_connect(sink, "tcp://localhost:5558");

    // Process tasks forever
    char buf[10];
    while (1) {
        int len = zmq_recv(receiver, buf, 10, 0);
        buf[len] = '\0';  // null-terminated the message
        printf("Message received: %s\n", buf);
        int workload = atoi(buf);
        MSLEEP(workload);
        zmq_send(sink, "", 0, 0);
    }

    zmq_close(receiver);
    zmq_close(sink);
    zmq_ctx_destroy(context);
    return 0;
}

