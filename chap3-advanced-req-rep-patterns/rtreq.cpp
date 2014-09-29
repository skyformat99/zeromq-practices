// rtreq.cpp
//
// ROUTER-to-REQ example
//
#include "zhelpers.h"
#include <pthread.h>

const int k_worker_num = 10;
#ifdef USE_RANDOM_IDENTITY
pthread_mutex_t rand_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif // USE_RANDOM_IDENTITY

static void* worker_task(void* data)
{
    void* context = zmq_ctx_new();
    void* worker = zmq_socket(context, ZMQ_REQ);
#ifdef USE_RANDOM_IDENTITY
    //@note lock needed, because 'rand' is not reentrant and thread-safe
    pthread_mutex_lock(&rand_mutex);
    s_set_id(worker);
    pthread_mutex_unlock(&rand_mutex);
#else
    int id = (int)data;
    char buf[16];
    sprintf(buf, "%d", id);
    zmq_setsockopt(worker, ZMQ_IDENTITY, &buf, strlen(buf));
#endif // USE_RANDOM_IDENTITY

    zmq_connect(worker, "tcp://localhost:5671");
    char identity[16] = {0};
    size_t len = 16;
    zmq_getsockopt(worker, ZMQ_IDENTITY, identity, &len);
    identity[len] = '\0';
    printf("#worker %s up and running\n", identity);

    int tasks = 0;
    while (1) {
        // Tell broker wh're ready
        s_send(worker, "Hi Boss");
        // Get workload from broker, until finished
        char* workload = s_recv(worker);
        printf("#worker %s received workload: %s\n", identity, workload);
        int finished = strcmp(workload, "Fired!") == 0;
        free(workload);
        if (finished) {
            printf("Worker %s completed: %d tasks\n", identity, tasks);
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
    //srand(time(0));

    // Start 10 workers
#ifdef USE_RANDOM_IDENTITY
    pthread_mutex_init(&rand_mutex, NULL);
#endif // USE_RANDOM_IDENTITY
    for (int i = 0; i < k_worker_num; ++i) {
        pthread_t worker;
#ifdef USE_RANDOM_IDENTITY
        pthread_create(&worker, NULL, worker_task, NULL);
#else
        pthread_create(&worker, NULL, worker_task, (void*)i);
#endif // USE_RANDOM_IDENTITY
    }
    // Run for 5 seconds and then tell workers to end
    int64_t end_time = s_clock() + 5000;
    int workers_fired = 0;
    while (1) {
        char* identity = s_recv(broker);
        printf("#reported worker: %s\n", identity);
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

#ifdef USE_RANDOM_IDENTITY
    pthread_mutex_destroy(&rand_mutex);
#endif // USE_RANDOM_IDENTITY
    zmq_close(broker);
    zmq_ctx_destroy(context);
    return 0;
}
