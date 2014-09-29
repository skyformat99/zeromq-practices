// task_sink.cpp
//
#include <zmq.h>
#include <zmq_utils.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    void* context = zmq_ctx_new();
    void* sink = zmq_socket(context, ZMQ_PULL);
    zmq_bind(sink, "tcp://*:5558");
    char buf[10];
    // Wait for start of batch
    zmq_recv(sink, buf, 10, 0);
    // Timing
    void* stopwatch = zmq_stopwatch_start();
    // Process 100 confirmations
    for (int t = 0; t < 100; ++t) {
        zmq_recv(sink, buf, 10, 0);
        if (t % 10 == 0)
            printf(":");
        else
            printf(".");
        fflush(stdout);
    }
    unsigned long elapsed_micros = zmq_stopwatch_stop(stopwatch);
    // Calc and report
    printf("Total elapsed time: %.2f ms\n", elapsed_micros*1.0/1000);

    zmq_close(sink);
    zmq_ctx_destroy(context);
    return 0;
}

