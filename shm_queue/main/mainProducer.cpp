#include <iostream>
#include <csignal>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "shm_queue.h"

volatile sig_atomic_t stopRequested = 0;
ShmQueue *queue = nullptr; // global so signal handler can use it
int shm_fd = -1;

// Push a value into shm queue safely
void push_to_queue(int value)
{
    pthread_mutex_lock(&queue->mutex);

    // Wait until not full
    while (((queue->tail + 1) % QUEUE_SIZE) == queue->head)
    {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    queue->data[queue->tail] = value;
    queue->tail = (queue->tail + 1) % QUEUE_SIZE;

    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
}

// Signal handler for Ctrl+C
void handle_sigint(int)
{
    stopRequested = 1;
    std::cout << "\nSIGINT received. Stopping producer...\n";
    if (queue)
    {
        push_to_queue(-1); // send termination signal
    }
}

int main()
{
    // Create shared memory
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_fd, sizeof(ShmQueue)) == -1)
    {
        perror("ftruncate");
        return 1;
    }

    void *ptr = mmap(nullptr, sizeof(ShmQueue),
                     PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    queue = static_cast<ShmQueue *>(ptr);

    // Init sync objects (first run only)
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&queue->mutex, &mutex_attr);
    pthread_cond_init(&queue->not_empty, &cond_attr);
    pthread_cond_init(&queue->not_full, &cond_attr);

    queue->head = queue->tail = 0;

    // Install signal handler
    std::signal(SIGINT, handle_sigint);

    std::cout << "Producer started. Generating numbers every 0.5 sec (Ctrl+C to stop).\n";

    int value = 1;
    while (!stopRequested)
    {
        push_to_queue(value);
        //std::cout << "Produced: " << value << std::endl;
        value++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Cleanup
    munmap(queue, sizeof(ShmQueue));
    close(shm_fd);

    if (shm_unlink(SHM_NAME) == -1)
    {
        perror("shm_unlink");
    }
    else
    {
        std::cout << "Shared memory unlinked.\n";
    }
    std::cout << "Produced: " << value - 1 << std::endl;
    std::cout << "Producer exited.\n";
    return 0;
}
