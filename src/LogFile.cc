#include "LogFile.h"

LogFile::LogFile(const std::string &basename, off_t rollsize, int flushInternalval, int checkEveryN)
                : basename_(basename)
                , rollsize_(rollsize)
                , flushInterval_(flushInternalval)
                , checkEveryN_(checkEveryN)
                , startOfPeriod_(0)
                , lastRoll_(0)
                , lastFlush_(0)
{
    rollFile();
}


LogFile::~LogFile() = default;


void LogFile::append(const char *data, int len)
{
    std::lock_guard<std::mutex> lock(mutex_);
    appendInlock(data,len);
}


void LogFile::flush()
{
    file_->flush();
}


bool LogFile::rollFile()
{
    time_t now = 0;
    std::string filename = getLogFileName(basename_,&now);
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;
    if(now > lastRoll_)
    {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        file_.reset(new FileUtil(filename));
        return true;
    }
    return false;
}


std::string LogFile::getLogFileName(const std::string &basename, time_t *now)
{
    std::string filename;
    filename.reserve(basename.size()+64);
    filename = basename;

    char buffer[64];
    struct tm tm;
    *now = time(NULL);
    localtime_r(now,&tm);
    strftime(buffer, sizeof(buffer), ".%Y%m%d-%H%M%S", &tm);

    filename += buffer;
    filename += ".log";
    return filename;
}


void LogFile::appendInlock(const char *data, int len)
{
    file_->append(data,len);
    time_t now;
    now = time(NULL);
    ++count_;
    if(file_->writtenBytes() > rollsize_)
    {
        rollFile();
    }
    else if(count_ >= checkEveryN_)
    {
        count_ = 0;
        time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;
        if(start != startOfPeriod_)
        {
            rollFile();
        }
    }
    if(now - lastFlush_ > flushInterval_)
    {
        lastFlush_ = now;
        file_->flush();
    }
}


