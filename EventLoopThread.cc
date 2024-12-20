#include "EventLoopThread.h"
#include "Logger.h"

EventLoopThread::EventLoopThread(const ThreadInitCallbck &cb, const std::string &name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , cond_()
    , callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    thread_.start(); // 启动底层新线程，执行ThreadInitCallbck

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }

    return loop;
}

// 下面这个方法是在单独的新线程里面运行的
void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个独立的EventLoop,和上面的线程是一一对应的，one loop peer thread

    LOG_INFO("EventLoopThread::threadFunc() new thread created! \n");

    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}