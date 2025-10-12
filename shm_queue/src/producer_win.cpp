#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <windows.h>
#include "shm_queue.h"

volatile sig_atomic_t stopRequested = 0;
ShmQueue* queue = nullptr;
HANDLE hMapFile = NULL;

// Push a value into the shared queue safely
void push_to_queue(int value)
{
    WaitForSingleObject(queue->mutex, INFINITE);

    // Wait until queue is not full
    while (((queue->tail + 1) % QUEUE_SIZE) == queue->head)
    {
        ResetEvent(queue->not_full);
        ReleaseMutex(queue->mutex);
        WaitForSingleObject(queue->not_full, INFINITE);
        WaitForSingleObject(queue->mutex, INFINITE);
    }

    queue->data[queue->tail] = value;
    queue->tail = (queue->tail + 1) % QUEUE_SIZE;

    // Signal that queue is not empty
    SetEvent(queue->not_empty);
    ReleaseMutex(queue->mutex);
}

// Ctrl+C handler
BOOL WINAPI handle_ctrl_c(DWORD)
{
    stopRequested = 1;
    std::cout << "\nCtrl+C received. Stopping producer..." << std::endl;
    if (queue)
        push_to_queue(-1);
    return TRUE;
}

int main()
{
    // Create a security descriptor allowing everyone access to Global objects
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE); // Allow full access
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    // Create shared memory region with explicit security attributes
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        &sa,
        PAGE_READWRITE,
        0,
        sizeof(ShmQueue),
        TEXT(SHM_NAME)
    );

    if (hMapFile == NULL)
    {
        std::cerr << "CreateFileMapping failed: " << GetLastError() << std::endl;
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

    queue = static_cast<ShmQueue*>(ptr);

    // Check if first process (GetLastError() == ERROR_ALREADY_EXISTS → no)
    bool firstInit = (GetLastError() != ERROR_ALREADY_EXISTS);

    if (firstInit)
    {
        std::cout << "Initializing shared queue and sync objects..." << std::endl;

        queue->mutex = CreateMutex(&sa, FALSE, TEXT("Global\\QueueMutex"));
        queue->not_empty = CreateEvent(&sa, TRUE, FALSE, TEXT("Global\\QueueNotEmpty"));
        queue->not_full = CreateEvent(&sa, TRUE, TRUE, TEXT("Global\\QueueNotFull"));

        if (!queue->mutex || !queue->not_empty || !queue->not_full)
        {
            std::cerr << "Failed to create sync objects. Error: " << GetLastError() << std::endl;
            return 1;
        }

        queue->head = 0;
        queue->tail = 0;
    }
    else
    {
        std::cout << "Opening existing sync objects..." << std::endl;
        queue->mutex = OpenMutex(SYNCHRONIZE, FALSE, TEXT("Global\\QueueMutex"));
        queue->not_empty = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, TEXT("Global\\QueueNotEmpty"));
        queue->not_full = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, TEXT("Global\\QueueNotFull"));

        if (!queue->mutex || !queue->not_empty || !queue->not_full)
        {
            std::cerr << "Failed to open sync objects. Error: " << GetLastError() << std::endl;
            return 1;
        }
    }

    SetConsoleCtrlHandler(handle_ctrl_c, TRUE);
    std::cout << "Producer started. Producing numbers every 10ms (Ctrl+C to stop)." << std::endl;

    int value = 1;
    while (!stopRequested)
    {
        push_to_queue(value);
        // std::cout << "Produced: " << value << std::endl;
        value++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Cleanup
    UnmapViewOfFile(ptr);
    CloseHandle(hMapFile);

    std::cout << "Produced: " << value - 1 << std::endl;
    std::cout << "Producer exited." << std::endl;
    return 0;
}
