#pragma once

#include "noncopyable.hpp"
#include "EventLoop.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallbck = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallbck& cb = ThreadInitCallbck(),const std::string& name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();
private:
    void threadFunc();

    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallbck callback_;
};

