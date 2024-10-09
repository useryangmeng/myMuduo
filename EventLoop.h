#pragma once

#include "noncopyable.hpp"
#include "Channel.h"
#include "Timestamp.h"
#include "CurrentThread.h"

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;

// 事件循环类，主要包含两个模块 Channel Poller（epoll的抽象，在muduo中实现了epoll和poll两种，这里只实现epoll）
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    //开启事件循环
    void loop();
    //退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    //在当前loop执行
    void runInLoop(Functor cb);
    //把cb放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    //用来唤醒loop所在的线程
    void weakup();

    //EventLoop的方法 -> Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    //判断eventLoop对象是否在自己的线程里面
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); };

private:
    void handleRead();//weak up
    void doPendingFunctors();//执行回调

    using ChannelList = std::vector<Channel *>;

    std::atomic_bool looping_; // 原子操作，底层通过CAS实现
    std::atomic_bool quit_;    // 标志退出loop循环

    const pid_t threadId_;     // 记录当前loop所在线程的id
    Timestamp pollReturnTime_; // poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    int weakupFd_; // eventfd,用于唤醒，当mainLoop获取一个新用户的channel后，通过轮询算法选择一个subLoop，通过该成员唤醒subLopp，处理channel
    std::unique_ptr<Channel> weakupChannel_;

    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; // 表示当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;    // 存贮loop需要执行的所有的回调cao'zuo
    std::mutex mutex_;                        // 互斥锁，用来保护上面的vector容器的线程安全操作
};
