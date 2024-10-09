#pragma once

#include "noncopyable.hpp"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// muduo中多路事件分发器，核心IO复用模块
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller() = default;                

    //给所以的IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs,ChannelList* activeChannel) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    //判断channel是否在当前的poller当中
    bool hasChannel(Channel* channel) const;

    //EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop* loop); 
protected:
    //map的key为sockfd
    using ChannelMap = std::unordered_map<int,Channel*>;
    ChannelMap channels_;

private:
    EventLoop* ownerLoop_; //poller所属的eventloop
};
