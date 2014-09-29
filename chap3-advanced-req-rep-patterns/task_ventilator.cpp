// task_ventilator.cpp
#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[])
{
    void* context = zmq_ctx_new();
    // Socket to send messages/tasks on
    void* sender = zmq_socket(context, ZMQ_PUSH);
    zmq_bind(sender, "tcp://*:5557");
    // Socket to send start of batch message on
    void* sink = zmq_socket(context, ZMQ_PUSH);
    zmq_connect(sink, "tcp://localhost:5558");

    printf("Press Enter when all workers are ready: ");
    getchar();
    printf("Sending tasks to workers...\n");

    srand(time(0));

    // First message sent to sink, to indicate start of the batch
    zmq_send(sink, "0", 1, 0);
    // Send 100 tasks to workers
    int total_workload = 0;  // millis
    char msg[10];
    for (int t = 0; t < 100; ++t) {
        int workload = rand() % 100 + 1;
        total_workload += workload;
        sprintf(msg, "%d", workload);
        zmq_send(sender, msg, strlen(msg), 0);
    }
    printf("Total workload: %d msec\n", total_workload);

    zmq_close(sink);
    zmq_close(sender);
    zmq_ctx_destroy(context);
    return 0;
}

