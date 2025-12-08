#include <cstring>
#include "FileUtil.h"

FileUtil::FileUtil(std::string& file_name) : file_(::fopen(file_name.c_str(),"ae")), writtenBytes_(0)
{
    ::setbuffer(file_,buffer_,sizeof(buffer_));
}

FileUtil::~FileUtil()
{
    if(file_)
    {
        ::fclose(file_);
    }
}

void FileUtil::flush()
{
    ::fflush(file_);
}

void FileUtil::append(const char* data, size_t len)
{
    size_t written = 0;
    while (written != len)
    {
        size_t remain = len - written;
        int n = write(data+written,remain);
        if(n != remain)
        {
            int err = ::ferror(file_);
            if(err)
            {
                fprintf(stderr, "AppendFile::append() failed %s\n", strerror(err));
                clearerr(file_);
                break;
            }
        }
        written += n;
    }
    writtenBytes_ += written;
}

size_t FileUtil::write(const char* data, size_t len)
{
    return ::fwrite_unlocked(data,len,1,file_);
}