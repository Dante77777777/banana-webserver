#include "AsyncLogging.h"
#include <stdio.h>


AsyncLogging::AsyncLogging(const std::string &basename, off_t rollSize, int flushInterval = 3)
    : flushInterval_(flushInterval)
    , running_(false)
    , basename_(basename)
    , rollSize_(rollSize)
    , thread_(std::bind(&AsyncLogging::threadFunc,this),"Logging")
    , mutex_()
    , cond_()
    , currentBuffer_(new LargeBuffer)
    , nextBuffer_(new LargeBuffer)
    , buffers_()
{
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}


void AsyncLogging::append(const char* data, int len)
{
    std::unique_lock<std::mutex> write_lock(mutex_);
    if(currentBuffer_->avail() > static_cast<size_t>(len))
    {
        currentBuffer_->append(data,len);
    }
    else
    {
        buffers_.push_back(currentBuffer_);
        if(nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else
        {
            currentBuffer_.reset(new LargeBuffer);
        }
        currentBuffer_->append(data,len);
    }
    cond_.notify_one();
}


void AsyncLogging::threadFunc()
{
    LogFile outPut(basename_,rollSize_);
    BufferPtr newBuffer1(new LargeBuffer);
    BufferPtr newBuffer2(new LargeBuffer);

    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    while (running_)
    {
        {
            std::unique_lock<std::mutex> write_lock(mutex_);
            if(buffers_.empty())
            {
                cond_.wait_for(write_lock,std::chrono::seconds(3));
            }
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            if(!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
            buffersToWrite.swap(buffers_);
        }
        for(auto& buffer : buffers_)
        {
            outPut.append(buffer->data(),buffer->length());
        }
        if(buffersToWrite.size() > 2)
        {
            buffersToWrite.resize(2);
        }
        if(!newBuffer1)
        {
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1.reset();
        }
        if(!newBuffer2)
        {
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2.reset();
        }
        buffersToWrite.clear();
        outPut.flush();
    }
    outPut.flush();
}