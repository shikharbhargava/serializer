#ifndef _SERIALIZATION_TPP_
#define _SERIALIZATION_TPP_

#include <opencv2/core.hpp>

// Template specializations

namespace Serialization
{
    template <>
    inline void Serializer::write<cv::Point>(const cv::Point &p)
    {
        write(p.x);
        write(p.y);
    }
    template <>
    inline void Serializer::write<cv::Point2f>(const cv::Point2f &p)
    {
        write(p.x);
        write(p.y);
    }

    template <>
    inline void Serializer::write<cv::Rect>(const cv::Rect &r)
    {
        write(r.x);
        write(r.y);
        write(r.width);
        write(r.height);
    }

    template <>
    inline void Serializer::write<cv::Mat>(const cv::Mat &m)
    {
        int rows = m.rows, cols = m.cols, type = m.type();
        write(rows);
        write(cols);
        write(type);
        size_t dataSize = m.total() * m.elemSize();
        write(dataSize);
        if (m.isContinuous())
        {
            const uint8_t *d = m.data;
            buffer.insert(buffer.end(), d, d + dataSize);
        }
        else
        {
            cv::Mat tmp = m.clone();
            const uint8_t *d = tmp.data;
            buffer.insert(buffer.end(), d, d + dataSize);
        }
    }

    template <>
    inline void Deserializer::read<cv::Point>(cv::Point &p)
    {
        read(p.x);
        read(p.y);
    }
    template <>
    inline void Deserializer::read<cv::Point2f>(cv::Point2f &p)
    {
        read(p.x);
        read(p.y);
    }
    template <>
    inline void Deserializer::read<cv::Rect>(cv::Rect &r)
    {
        read(r.x);
        read(r.y);
        read(r.width);
        read(r.height);
    }
    template <>
    inline void Deserializer::read<cv::Mat>(cv::Mat &m)
    {
        int rows, cols, type;
        size_t dataSize;
        read(rows);
        read(cols);
        read(type);
        read(dataSize);
        m.create(rows, cols, type);
        if (pos + dataSize > size)
            throw std::runtime_error("Buffer underflow");
        std::memcpy(m.data, data + pos, dataSize);
        pos += dataSize;
    }
} // namespace Serialization

#endif // _SERIALIZATION_TPP_