#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
     struct thread_data* data = (struct thread_data*) thread_param;

    if (!data) return NULL;

    // Step 1: initial wait
    usleep(data->wait_to_obtain_ms * 1000);

    // Step 2: try to lock mutex
    if (pthread_mutex_lock(data->mutex) != 0) {
        ERROR_LOG("Failed to lock mutex\n");
        data->thread_complete_success = false;
        return data;
    }

    // Step 3: hold lock for given ms
    usleep(data->wait_to_release_ms * 1000);

    // Step 4: unlock
    if (pthread_mutex_unlock(data->mutex) != 0) {
        ERROR_LOG("Failed to unlock mutex\n");
        data->thread_complete_success = false;
        return data;
    }

    data->thread_complete_success = true;
    return data;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    struct thread_data* data = malloc(sizeof(struct thread_data));
    if (!data) {
        ERROR_LOG("Failed to allocate memory for thread_data\n");
        return false;
    }

    data->mutex = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->thread_complete_success = false;

    int rc = pthread_create(thread, NULL, threadfunc, data);
    if (rc != 0) {
        ERROR_LOG("pthread_create failed\n");
        free(data);
        return false;
    }

    return true;
}



