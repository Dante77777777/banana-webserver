#include "Logger.h"
#include "CurrentThread.h"


namespace bananaWebServer
{
    thread_local char t_errnobuf[512];
    thread_local char t_timer[64];
    thread_local time_t t_lastSecond;    
} // namespace bananaWebServer


const char* getErrnoMsg(int savedErrno)
{
    return strerror_r(savedErrno, bananaWebServer::t_errnobuf, sizeof(bananaWebServer::t_errnobuf));
}

const char *getLevelName[Logger::LogLevel::LEVEL_COUNT]{
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

static void defaultOutput(const char *data, int len)
{
    fwrite(data, len, sizeof(char), stdout);
}

static void defaultFlush()
{
    fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

Logger::Impl::Impl(Logger::LogLevel level, int savedErrno, const char *filename, int line)
    : time_(Timestamp::now()),
      stream_(),
      level_(level),
      line_(line),
      basename_(filename)
{
    formatTime();
    stream_ << GeneralTemplate(getLevelName[level],6);
    if(savedErrno != 0)
    {
        stream_ << getErrnoMsg(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}


void Logger::Impl::formatTime()
{
    Timestamp now = Timestamp::now();
    //计算秒数
    time_t seconds = static_cast<time_t>(now.microSecondsSinceEpoch() / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(now.microSecondsSinceEpoch() % Timestamp::kMicroSecondsPerSecond);
    //计算剩余微秒数
    struct tm *tm_timer = localtime(&seconds);
    // 写入此线程存储的时间buf中
    snprintf(bananaWebServer::t_timer, sizeof(bananaWebServer::t_timer), "%4d/%02d/%02d %02d:%02d:%02d",
             tm_timer->tm_year + 1900,
             tm_timer->tm_mon + 1,
             tm_timer->tm_mday,
             tm_timer->tm_hour,
             tm_timer->tm_min,
             tm_timer->tm_sec);
    // 更新最后一次时间调用
    bananaWebServer::t_lastSecond = seconds;

    // muduo使用Fmt格式化整数，这里我们直接写入buf
    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "%06d ", microseconds);

    
    stream_ << GeneralTemplate(bananaWebServer::t_timer, 17) << GeneralTemplate(buf, 7);

}


void Logger::Impl::finish()
{
    stream_ << " - " << GeneralTemplate(basename_.data_, basename_.len_)
            << ':' << line_ << '\n';
}


Logger::Logger(const char *filename, int line, LogLevel level) : impl_(level, 0, filename, line)
{
}


Logger::~Logger()
{
    impl_.finish();
    const LogStream::Buffer &buffer = stream().buffer();
    // 输出(默认项终端输出)
    g_output(buffer.data(), buffer.length());
    // FATAL情况终止程序
    if (impl_.level_ == FATAL)
    {
        g_flush();
        abort();
    }
}


void Logger::setOutput(OutputFunc out)
{
    g_output = out;
}


void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}