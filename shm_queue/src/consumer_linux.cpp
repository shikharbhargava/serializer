#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "shm_queue.h"

std::queue<int> localQueue;
std::mutex localMutex;
std::condition_variable localCV;
bool stopReader = false;

// Reader thread
void readerThread(ShmQueue *queue)
{
    while (!stopReader)
    {
        pthread_mutex_lock(&queue->mutex);

        while (queue->head == queue->tail)
        {
            pthread_cond_wait(&queue->not_empty, &queue->mutex);
        }

        int val = queue->data[queue->head];
        queue->head = (queue->head + 1) % QUEUE_SIZE;

        pthread_cond_signal(&queue->not_full);
        pthread_mutex_unlock(&queue->mutex);

        {
            std::lock_guard<std::mutex> lock(localMutex);
            localQueue.push(val);
        }
        localCV.notify_one();

        if (val == -1)
            break;
    }
}

int main()
{
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1)
    {
        perror("shm_open");
        return 1;
    }

    void *ptr = mmap(nullptr, sizeof(ShmQueue),
                     PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    auto *queue = static_cast<ShmQueue *>(ptr);

    std::thread reader(readerThread, queue);
    int count = 0;
    while (true)
    {
        std::unique_lock<std::mutex> lock(localMutex);
        localCV.wait(lock, []
                     { return !localQueue.empty(); });

        int val = localQueue.front();
        localQueue.pop();
        lock.unlock();

        if (val == -1)
        {
            std::cout << "Consumer received termination signal. Exiting...\n";
            break;
        }
        count++;
        //std::cout << "Main thread displays: " << val << std::endl;
    }

    stopReader = true;
    reader.join();

    // Cleanup
    munmap(queue, sizeof(ShmQueue));
    close(fd);
    std::cout << "Consumed: " << count << std::endl;
    return 0;
}
