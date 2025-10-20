// shm_queue.h
#ifndef _SHM_QUEUE_H_
#define _SHM_QUEUE_H_

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#define SHM_NAME "/my_shm_queue"
#define SHM_SIZE (1024 * 1024 * 10) // 10 MB shared memory

struct ShmQueue
{
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    size_t head;       // byte offset
    size_t tail;       // byte offset
    size_t capacity;   // total buffer size
    uint8_t buffer[1]; // flexible array member (actual size SHM_SIZE - sizeof(ShmQueue))
};

#endif // _SHM_QUEUE_H_
