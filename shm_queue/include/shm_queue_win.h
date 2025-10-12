#ifndef _SHM_QUEUE_H_
#define _SHM_QUEUE_H_

#include <windows.h>
#include <stddef.h>

#define QUEUE_SIZE 8
#define SHM_NAME "Global\\my_shm_queue"

// Shared memory ring buffer for integers (Windows version)
struct ShmQueue {
    // Synchronization handles for interprocess use
    HANDLE mutex;       // equivalent to pthread_mutex_t
    HANDLE not_empty;   // equivalent to pthread_cond_t (signaled when queue not empty)
    HANDLE not_full;    // equivalent to pthread_cond_t (signaled when queue not full)

    size_t head;
    size_t tail;
    int data[QUEUE_SIZE];
};

#endif // _SHM_QUEUE_H_
