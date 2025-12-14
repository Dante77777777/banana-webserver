#include "CurrentThread.h"

namespace bananaWebServer
{
    thread_local int t_cacheTid = 0;
    void cacheTid()
    {
        if (t_cacheTid == 0)
        {
            t_cacheTid = static_cast<pid_t>(::syscall(SYS_gettid)); // Ensure syscall and SYS_gettid are defined
        }
    }
}   