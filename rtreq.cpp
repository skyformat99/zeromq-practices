// rtreq.cpp
//
// ROUTER-to-REQ example
//
#include "zhelpers.h"
#include <pthread.h>

const int k_worker_num = 10;

static void* worker_task(void* data)
{
    void* context = zmq_ctx_new();
    void* worker = zmq_socket(context, ZMQ_REQ);
    s_set_id(worker);
    zmq_connect(worker, "tcp://localhost:5671");

    int tasks = 0;
    while (1) {
        // Tell broker wh're ready
        s_send(worker, "Hi Boss");
        // Get workload from broker, until finished
        char* workload = s_recv(worker);
        int finished = strcmp(workload, "Fired!") == 0;
        free(workload);
        if (finished) {
            printf("Completed: %d tasks\n", tasks);
            break;
        }
        tasks++;
        // Do some random work
        s_sleep(randof(500) + 1);
    }
    zmq_close(worker);
    zmq_ctx_destroy(context);
    return NULL;
}

int main()
{
    void* context = zmq_ctx_new();
    void* broker = zmq_socket(context, ZMQ_ROUTER);
    zmq_bind(broker, "tcp://*:5671");
    srand((unsigned) time(NULL));

    // Start 10 workers
    for (int i = 0; i < k_worker_num; ++i) {
        pthread_t worker;
        pthread_create(&worker, NULL, worker_task, NULL);
    }
    // Run for 5 seconds and then tell workers to end
    int64_t end_time = s_clock() + 5000;
    int workers_fired = 0;
    while (1) {
        char* identity = s_recv(broker);
        s_sendmore(broker, identity);
        free(identity);
        free(s_recv(broker));  // Envelope delimiter
        free(s_recv(broker));  // Response from worker
        s_sendmore(broker, ""); // Envelope delimiter for response
        if (s_clock() < end_time) {
            s_send(broker, "Work harder");
        } else {
            s_send(broker, "Fired!");
            if (++workers_fired == k_worker_num)
                break;
        }
    }

    zmq_close(broker);
    zmq_ctx_destroy(context);
    return 0;
}
