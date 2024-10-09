#pragma once

#include "noncopyable.hpp"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallbck = std::function<void(EventLoop*)>; 

    EventLoopThreadPool(EventLoop* baseLoop,const std::string& nameArg);
    ~EventLoopThreadPool();

    void setThreaNum(int num) {numThreads_ = num;}

    void start(const ThreadInitCallbck& cb = ThreadInitCallbck());

    //如果工作在多线程中，baseloop_默认以轮询的方式分配channel给subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoop();

    bool started() const {return started_;}
    const std::string& name() const {return name_;}
private:
    EventLoop* baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};
