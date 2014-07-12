// task_sink.cpp
//
#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <boost/chrono.hpp>

using namespace boost::chrono;

int main(int argc, char* argv[])
{
    void* context = zmq_ctx_new();
    void* sink = zmq_socket(context, ZMQ_PULL);
    zmq_bind(sink, "tcp://*:5558");
    char buf[10];
    // Wait for start of batch
    zmq_recv(sink, buf, 10, 0);
    // Timing
    system_clock::time_point start = system_clock::now();
    // Process 100 confirmations
    for (int t = 0; t < 100; ++t) {
        zmq_recv(sink, buf, 10, 0);
        if (t % 10 == 0)
            printf(":");
        else
            printf(".");
        fflush(stdout);
    }
    duration<double> elapsed = system_clock::now() - start;
    // Calc and report
    printf("Total elapsed time: %.2f ms\n", elapsed.count()*1000);

    zmq_close(sink);
    zmq_ctx_destroy(context);
    return 0;
}

