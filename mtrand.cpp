/**
 * @file mtrand.cpp
 *
 * @breif Test the behavior of the `rand` function in the standard lib under
 * multi-threaded environment.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

const static int RANGE_MAX = 200;
//static pthread_mutex_t rand_mtx = PTHREAD_MUTEX_INITIALIZER;

static void* rand_task(void* data)
{
    unsigned int id = (unsigned int)pthread_self().p;
    // Init random seed using current thread id
    srand(id);

    for (int i = 0; i < 3; ++i) {
        //pthread_mutex_lock(&rand_mtx);
        int rand_num = rand() % RANGE_MAX;
        //pthread_mutex_unlock(&rand_mtx);
        printf("0x%x: #%d - %d\n", id, i, rand_num);
    }
    return 0;
}

int main()
{
    const int total_threads = 5;
    //pthread_mutex_init(&rand_mtx, NULL);

    pthread_t threads[total_threads] = {0};
    for (int t = 0; t < total_threads; ++t) {
        pthread_t th;
        pthread_create(&th, 0, rand_task, 0);
        threads[t] = th;
    }

    // Wait
    for (int t = 0; t < total_threads; ++t) {
        pthread_join(threads[t], 0);
    }

    //pthread_mutex_destroy(&rand_mtx);
    return 0;
}
