#ifndef TIMER_H
#define TIMER_H

#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>

class Timer : noncopyable
{
public:
    using TimerCallback = std::function<void()>;
    Timer(TimerCallback cb, Timestamp expiration, const double interval)
        : callback_(std::move(cb))
        , expiration_(expiration)
        , interval_(interval)
        , repeat_(interval == 0)
    {}

    void run()
    {
        callback_();
    }

    Timestamp expiration() const  { return expiration_; }
    bool repeat() const { return repeat_; }

    // 重启定时器(如果是非重复事件则到期时间置为0)
    void restart(Timestamp now);


private:
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    TimerCallback callback_;

};




#endif