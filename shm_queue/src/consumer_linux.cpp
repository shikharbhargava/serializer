#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "shm_queue.h"
#include "Serialization.hpp"
#include <opencv2/opencv.hpp>

using Serialization::Deserializer;

ShmQueue *queue = nullptr;
int shm_fd = -1;

std::queue<std::vector<uint8_t>> localQueue;
std::mutex localMutex;
std::condition_variable localCV;
bool stopReader = false;

// ==================== Generic pop raw bytes =====================
bool pop_bytes_from_queue(std::vector<uint8_t> &buf)
{
    pthread_mutex_lock(&queue->mutex);

    while (queue->head == queue->tail)
        pthread_cond_wait(&queue->not_empty, &queue->mutex);

    uint32_t dataSize;
    if (queue->head + sizeof(uint32_t) > queue->capacity)
    {
        size_t part = queue->capacity - queue->head;
        memcpy(&dataSize, queue->buffer + queue->head, part);
        memcpy(((uint8_t *)&dataSize) + part, queue->buffer, sizeof(uint32_t) - part);
        queue->head = sizeof(uint32_t) - part;
    }
    else
    {
        memcpy(&dataSize, queue->buffer + queue->head, sizeof(uint32_t));
        queue->head += sizeof(uint32_t);
        if (queue->head == queue->capacity)
            queue->head = 0;
    }

    buf.resize(dataSize);
    if (queue->head + dataSize > queue->capacity)
    {
        size_t part = queue->capacity - queue->head;
        memcpy(buf.data(), queue->buffer + queue->head, part);
        memcpy(buf.data() + part, queue->buffer, dataSize - part);
        queue->head = dataSize - part;
    }
    else
    {
        memcpy(buf.data(), queue->buffer + queue->head, dataSize);
        queue->head += dataSize;
        if (queue->head == queue->capacity)
            queue->head = 0;
    }

    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);

    return true;
}

// ==================== Data struct =====================
struct Data
{
    cv::Mat frame;
    std::string text;
    bool end = false;

    void deserialize(Deserializer *d)
    {
        d->read(frame);
        d->read(text);
        d->read(end);
    }
};

// ==================== Reader thread =====================
void readerThread()
{
    while (!stopReader)
    {
        std::vector<uint8_t> buf;
        if (!pop_bytes_from_queue(buf))
            continue;

        {
            std::lock_guard<std::mutex> lock(localMutex);
            localQueue.push(buf);
        }
        localCV.notify_one();
    }
}

// ==================== Main =====================
int main(int argc, char *argv[])
{
    bool useData = false;
    if (argc > 1 && std::string(argv[1]) == "image")
        useData = true;

    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        return 1;
    }

    void *ptr = mmap(nullptr, sizeof(ShmQueue) + SHM_SIZE - 1,
                     PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    queue = static_cast<ShmQueue *>(ptr);

    std::thread reader(readerThread);

    // Make the display window resizable
    const char *winName = "Consumer Frame";
    cv::namedWindow(winName, cv::WINDOW_NORMAL);
    cv::resizeWindow(winName, 800, 600);

    int count = 0;
    while (true)
    {
        std::unique_lock<std::mutex> lock(localMutex);
        localCV.wait(lock, []
                     { return !localQueue.empty(); });

        auto buf = localQueue.front();
        localQueue.pop();
        lock.unlock();

        if (!useData)
        {
            int val;
            Deserializer d(buf.data(), buf.size());
            d.read(val);

            if (val == -1)
                break;
            std::cout << "Consumed int: " << val << std::endl;
        }
        else
        {
            Data d;
            Deserializer ds(buf.data(), buf.size());
            d.deserialize(&ds);

            std::cout << "Consumed Data: " << d.text
                      << " Frame size: " << d.frame.cols << "x" << d.frame.rows << std::endl;

            // Prepare image for display (convert single-channel to BGR)
            cv::Mat disp;
            if (d.frame.channels() == 1)
                cv::cvtColor(d.frame, disp, cv::COLOR_GRAY2BGR);
            else
                disp = d.frame.clone();

            // Compose overlay text
            std::string text = d.text.empty() ? "No text" : d.text;
            // Also show frame size
            std::string sizeText = std::to_string(disp.cols) + "x" + std::to_string(disp.rows);
            std::string overlay = text + " (" + sizeText + ")";

            int fontFace = cv::FONT_HERSHEY_SIMPLEX;
            double fontScale = std::max(0.5, std::min(2.0, disp.cols / 800.0));
            int thickness = std::max(1, (int)(fontScale * 2));
            int baseline = 0;
            cv::Size textSize = cv::getTextSize(overlay, fontFace, fontScale, thickness, &baseline);

            // Position text at top-left with a small margin and draw a filled rectangle behind it for contrast
            cv::Point textOrg(10, 10 + textSize.height);
            cv::Point rectTL(textOrg.x - 6, textOrg.y - textSize.height - 6);
            cv::Point rectBR(textOrg.x + textSize.width + 6, textOrg.y + 6);
            cv::rectangle(disp, rectTL, rectBR, cv::Scalar(0, 0, 0), cv::FILLED);
            cv::putText(disp, overlay, textOrg, fontFace, fontScale, cv::Scalar(255, 255, 255), thickness, cv::LINE_AA);

            // Show the frame
            cv::imshow(winName, disp);
            int key = cv::waitKey(1);
            if (key == 27) // ESC to exit
                break;
            if (d.end)
                break;
        }
        count++;
    }

    stopReader = true;
    reader.join();

    munmap(queue, sizeof(ShmQueue) + SHM_SIZE - 1);
    close(shm_fd);
    std::cout << "Consumed " << count << " items\n";
}
