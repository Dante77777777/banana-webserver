#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

#include <Buffer.h>

ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extraBuffer[65536] = {0};
    struct iovec vec[2];
    const size_t writable = writableBytes();

    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extraBuffer;
    vec[1].iov_len = sizeof(extraBuffer);

    const int iovcnt = (writable < sizeof(extraBuffer)) ? 2 : 1;
    const ssize_t n = ::readv(fd,vec,iovcnt);
    if(n < 0)
    {
        *saveErrno = n;
    }
    else if(n < writable)
    {
        writerIndex_ += n;
    }
    else
    {
        writerIndex_ = buffer_.size();
        append(extraBuffer,n);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno)
{
    ssize_t n = ::write(fd,peek(),readableBytes());
    if(n < 0)
    {
        *saveErrno = n;
    }
    return n;
}