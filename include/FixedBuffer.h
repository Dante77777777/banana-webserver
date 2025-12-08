#pragma once
#include <string.h>
#include <string>

class AsyncLogging;
constexpr int kSmallBufferSize = 4000;
constexpr int kLargeBufferSize = 4000 * 1000;

template <int buffer_size>
class FixedBuffer : noncopyable
{
public:
    FixedBuffer() : cur_(data_),size_(0)
    {

    }

    void append(const cahr* data, size_t len)
    {
        if(avail() > len)
        {
            memcpy(cur_,data,len);
            add(len);
        }
    }

    const char* data() const { return data_; }
    int length() const { return size_; }
    char* current() { return cur_; }
    size_t avail() const { return static_cast<size_t>(buffer_size - size_); }
    void add(size_t len)
    {
        cur_ += len;
        size_ += len;
    }
    void reset()
    {
        cur_ = data_;
        size_ = 0;
    }
    void bzero() { ::bzero(data_, sizeof(data_)); }
private:
    char data_[buffer_size];
    char* cur_;
    int size_;
};