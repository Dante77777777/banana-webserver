#pragma once
#include <string>
#include <string.h>
#include "noncopyable.h"
#include "FixedBuffer.h"

class GeneralTemplate : noncopyable
{
public:
    GeneralTemplate() : data_(nullptr), len_(0)
    {

    }
    GeneralTemplate(const char* data, int len) : data_(data), len_(len)
    {

    }

    const char* data_;
    int len_;
};


class LogStream : noncopyable
{
public:
    using Buffer = FixedBuffer<kSmallBufferSize>;
    void append(const char* data, int len)
    {
        buffer_.append(data,len);
    }

    const Buffer &buffer() const
    {
        return buffer_;
    }

    void reset_buffer()
    {
        buffer_.reset();
    }
     // 重载输出流运算符<<，用于将布尔值写入缓冲区
    LogStream &operator<<(bool express);

    // 重载输出流运算符<<，用于将短整型写入缓冲区
    LogStream &operator<<(short number);
    // 重载输出流运算符<<，用于将无符号短整型写入缓冲区
    LogStream &operator<<(unsigned short);
    // 重载输出流运算符<<，用于将整型写入缓冲区
    LogStream &operator<<(int);
    // 重载输出流运算符<<，用于将无符号整型写入缓冲区
    LogStream &operator<<(unsigned int);
    // 重载输出流运算符<<，用于将长整型写入缓冲区
    LogStream &operator<<(long);
    // 重载输出流运算符<<，用于将无符号长整型写入缓冲区
    LogStream &operator<<(unsigned long);
    // 重载输出流运算符<<，用于将长长整型写入缓冲区
    LogStream &operator<<(long long);
    // 重载输出流运算符<<，用于将无符号长长整型写入缓冲区
    LogStream &operator<<(unsigned long long);

    // 重载输出流运算符<<，用于将浮点数写入缓冲区
    LogStream &operator<<(float number);
    // 重载输出流运算符<<，用于将双精度浮点数写入缓冲区
    LogStream &operator<<(double);

    // 重载输出流运算符<<，用于将字符写入缓冲区
    LogStream &operator<<(char str);
    // 重载输出流运算符<<，用于将C风格字符串写入缓冲区
    LogStream &operator<<(const char *);
    // 重载输出流运算符<<，用于将无符号字符指针写入缓冲区
    LogStream &operator<<(const unsigned char *);
    // 重载输出流运算符<<，用于将std::string对象写入缓冲区
    LogStream &operator<<(const std::string &);
    // (const char*, int)的重载
    LogStream& operator<<(const GeneralTemplate& g);
private:
    // 定义最大数字大小常量
    static constexpr int kMaxNumberSize = 32;

    // 对于整型需要特殊的处理，模板函数用于格式化整型
    template <typename T>
    void formatInteger(T num);

    // 内部缓冲区对象
    Buffer buffer_;
};