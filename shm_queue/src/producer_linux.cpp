#include <iostream>
#include <csignal>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>
#include "shm_queue.h"
#include "shm_queue_data.h"

volatile sig_atomic_t stopRequested = 0;
ShmQueue *queue = nullptr;
int shm_fd = -1;

// ==================== Generic push =====================
template <typename T>
bool push_to_queue(const T &data)
{
    Serializer s;
    s.write(data);
    auto serializedData = s.data();
    uint32_t serializedDataSize = s.dataLength();

    pthread_mutex_lock(&queue->mutex);

    while (true)
    {
        size_t freeSpace = (queue->head <= queue->tail) ? queue->capacity - queue->tail + queue->head : queue->head - queue->tail;
        if (freeSpace > serializedDataSize + sizeof(uint32_t))
            break;
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    // Write size
    if (queue->tail + sizeof(uint32_t) > queue->capacity)
    {
        size_t part = queue->capacity - queue->tail;
        memcpy(queue->buffer + queue->tail, &serializedDataSize, part);
        memcpy(queue->buffer, ((uint8_t *)&serializedDataSize) + part, sizeof(uint32_t) - part);
        queue->tail = sizeof(uint32_t) - part;
    }
    else
    {
        memcpy(queue->buffer + queue->tail, &serializedDataSize, sizeof(uint32_t));
        queue->tail += sizeof(uint32_t);
        if (queue->tail == queue->capacity)
            queue->tail = 0;
    }

    // Write data
    if (queue->tail + serializedDataSize > queue->capacity)
    {
        size_t part = queue->capacity - queue->tail;
        memcpy(queue->buffer + queue->tail, serializedData, part);
        memcpy(queue->buffer, serializedData + part, serializedDataSize - part);
        queue->tail = serializedDataSize - part;
    }
    else
    {
        memcpy(queue->buffer + queue->tail, serializedData, serializedDataSize);
        queue->tail += serializedDataSize;
        if (queue->tail == queue->capacity)
            queue->tail = 0;
    }

    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

// ==================== Signal =====================
void handle_sigint(int)
{
    stopRequested = 1;
}

cv::Scalar loopColor(int val)
{
    // Scale value to 0-255
    int v = val % 256;

    // Simple gradient: Red, Green, Blue
    int r = v;             // Red channel
    int g = 255 - v;       // Green channel
    int b = (v * 2) % 256; // Blue channel loops faster

    return cv::Scalar(b, g, r); // BGR for OpenCV
}
int main(int argc, char *argv[])
{
    bool useData = false;
    if (argc > 1 && std::string(argv[1]) == "data")
        useData = true;

    // Shared memory
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        return 1;
    }
    ftruncate(shm_fd, sizeof(ShmQueue) + SHM_SIZE - 1);

    void *ptr = mmap(nullptr, sizeof(ShmQueue) + SHM_SIZE - 1,
                     PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    queue = static_cast<ShmQueue *>(ptr);

    // Init sync
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
    queue->capacity = SHM_SIZE;

    std::signal(SIGINT, handle_sigint);

    std::cout << "Producer started. Press Ctrl+C to stop.\n";

    int value = 1;
    while (!stopRequested)
    {
        if (!useData)
        {
            push_to_queue(value);
        }
        else
        {
            Data d;
            d.text = "Frame " + std::to_string(value);
            // create an RGB color (OpenCV uses BGR order in cv::Scalar)
            auto color = loopColor(value * 2);
            d.frame = cv::Mat(480, 640, CV_8UC3, color);
            d.end = false;
            push_to_queue(d);
        }
        value++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // send termination signal
    if (!useData)
        push_to_queue(-1);
    else
    {
        Data d;
        d.end = true;
        push_to_queue(d);
    }

    munmap(queue, sizeof(ShmQueue) + SHM_SIZE - 1);
    close(shm_fd);
    shm_unlink(SHM_NAME);
    std::cout << "Producer exited\n";
}
