#ifndef _SHM_QUEUE_DATA_
#define _SHM_QUEUE_DATA_

#include "Serialization.hpp"
#include <opencv2/opencv.hpp>

using Serialization::Serializer;
using Serialization::Deserializer;

// ==================== Data struct =====================
struct Data
{
    cv::Mat frame;
    std::string text;
    bool end = false;

    void serialize(Serializer *s) const
    {
        s->write(frame);
        s->write(text);
        s->write(end);
    }

    void deserialize(Deserializer *d)
    {
        d->read(frame);
        d->read(text);
        d->read(end);
    }
};


#endif // _SHM_QUEUE_DATA_
