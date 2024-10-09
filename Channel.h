#pragma once

#include "noncopyable.hpp"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;
// class Timestamp;   不能使用，由于handleEvent会调用，而EventLoop* 调用为指针

/**
 * Channel理解为通道，封装了sockfd和其感兴趣的event，如EPOLLON、EPOLLOUT事件
 *还绑定了poller返回的具体事件
 */
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知以后，处理事件的函数
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }

    // 设置fd相应的事件状态
    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isReading() const { return events_ & kReadEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop peer thread
    EventLoop *ownerLoop() { return loop_; }

    void remove();

private:
    void update();
    void handleEventWithGurd(Timestamp receveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd，Poller监听的对象
    int events_;      // 注册fd感兴趣的事件
    int revents_;     // poller返回的发送的具体事件
    int index_;       // channel对应的状态，new、added、deleted

    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为Channel通道里面能够获取fd最终发生的具体的事件events，所以它负责具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
