#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <windows.h>
#include "shm_queue.h"  // Use the Windows-specific header

std::queue<int> localQueue;
std::mutex localMutex;
std::condition_variable localCV;
bool stopReader = false;

// Reader thread
void readerThread(ShmQueue* queue)
{
    while (!stopReader)
    {
        // Wait until not_empty event is signaled
        WaitForSingleObject(queue->not_empty, INFINITE);

        // Lock the shared mutex
        WaitForSingleObject(queue->mutex, INFINITE);

        while (queue->head == queue->tail)
        {
            // Nothing to read, reset not_empty and wait again
            ResetEvent(queue->not_empty);
            ReleaseMutex(queue->mutex);
            WaitForSingleObject(queue->not_empty, INFINITE);
            WaitForSingleObject(queue->mutex, INFINITE);
        }

        int val = queue->data[queue->head];
        queue->head = (queue->head + 1) % QUEUE_SIZE;

        // Signal space availability
        SetEvent(queue->not_full);
        ReleaseMutex(queue->mutex);

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
    std::cout << "Consumer starting, waiting for shared memory..." << std::endl;

    // Try to open shared memory (wait a little if producer not yet ready)
    HANDLE hMapFile = NULL;
    for (int i = 0; i < 20; ++i)
    {
        hMapFile = OpenFileMapping(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            TEXT(SHM_NAME)
        );

        if (hMapFile)
            break;

        std::cerr << "OpenFileMapping attempt " << (i + 1)
            << " failed: " << GetLastError() << " — retrying...\n";
        Sleep(200);
    }

    if (hMapFile == NULL)
    {
        std::cerr << "OpenFileMapping failed: " << GetLastError() << std::endl;
        return 1;
    }

    void* ptr = MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0, 0,
        sizeof(ShmQueue)
    );

    if (ptr == NULL)
    {
        std::cerr << "MapViewOfFile failed: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }

    auto* queue = static_cast<ShmQueue*>(ptr);

    // Open existing synchronization objects
    queue->mutex = OpenMutex(SYNCHRONIZE, FALSE, TEXT("Global\\QueueMutex"));
    queue->not_empty = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, TEXT("Global\\QueueNotEmpty"));
    queue->not_full = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, TEXT("Global\\QueueNotFull"));

    if (!queue->mutex || !queue->not_empty || !queue->not_full)
    {
        std::cerr << "Failed to open synchronization objects. Error: " << GetLastError() << std::endl;
        UnmapViewOfFile(ptr);
        CloseHandle(hMapFile);
        return 1;
    }

    std::thread reader(readerThread, queue);
    int count = 0;

    while (true)
    {
        std::unique_lock<std::mutex> lock(localMutex);
        localCV.wait(lock, [] { return !localQueue.empty(); });

        int val = localQueue.front();
        localQueue.pop();
        lock.unlock();

        if (val == -1)
        {
            std::cout << "Consumer received termination signal. Exiting...\n";
            break;
        }

        count++;
        // std::cout << "Consumed: " << val << std::endl;
    }

    stopReader = true;
    reader.join();

    // Cleanup
    UnmapViewOfFile(ptr);
    CloseHandle(hMapFile);

    std::cout << "Consumed total: " << count << std::endl;
    std::cout << "Consumer exited successfully." << std::endl;
    return 0;
}
