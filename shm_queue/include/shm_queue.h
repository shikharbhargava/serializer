#ifndef _SHM_QUEUE_H_
#define _SHM_QUEUE_H_

#include <pthread.h>
#include <stddef.h>

#define QUEUE_SIZE 8
#define SHM_NAME "/my_shm_queue"

// Shared memory ring buffer for integers
struct ShmQueue {
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    size_t head;
    size_t tail;
    int data[QUEUE_SIZE];
};


#endif // _SHM_QUEUE_H_
