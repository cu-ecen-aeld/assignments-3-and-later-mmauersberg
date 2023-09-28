#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...) printf("threading DEBUG: " msg "\n", ##__VA_ARGS__)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TASK: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    // cast thread_param to thread_data threadData structure
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // wait before obtaining mutex
    DEBUG_LOG("waiting to obtain mutex for thread %p", thread_func_args);
    usleep (thread_func_args->wait_to_obtain_ms);

    // obtain mutex
    DEBUG_LOG("obtaining mutex for thread %p", thread_func_args);
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        ERROR_LOG("could not obtain mutex for thread %p", thread_func_args);
        thread_func_args->thread_complete_success = false;
        pthread_exit(thread_func_args);
    }

    // wait for defined hold time
    DEBUG_LOG("waiting to release mutex for thread %p", thread_func_args);
    usleep (thread_func_args->wait_to_release_ms);

    // release mutex
    DEBUG_LOG("releasing mutex for thread %p", thread_func_args);
    if (pthread_mutex_unlock(thread_func_args->mutex) !=0) {
        ERROR_LOG("cold not release mutex for thread %p", thread_func_args);
        thread_func_args->thread_complete_success = false;
        pthread_exit(thread_func_args);
    }

    // if all went well, exit with success
    DEBUG_LOG("thread_func for %p successful.", thread_func_args);
    thread_func_args->thread_complete_success = true;
    pthread_exit(thread_func_args);
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TASK: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    DEBUG_LOG("allocating memory for thread_data");
    struct thread_data *threadData = (struct thread_data*) malloc(sizeof(struct thread_data));
    if (threadData == NULL) {
        ERROR_LOG("could not allocate memory for thread_data");
        return false;
    }

    DEBUG_LOG("creating thread");
    threadData->wait_to_obtain_ms = wait_to_obtain_ms;
    threadData->wait_to_release_ms = wait_to_release_ms;
    threadData->mutex = mutex;

    if (pthread_create(thread, NULL,threadfunc, threadData) != 0) {
        ERROR_LOG("could not create thread");
        free (threadData);
        return false;
    }

    return true;
}

