#pragma once

#include "Poller.h"

#include <vector>
#include <sys/epoll.h>

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;
    
    //重写基类的抽象方法
    Timestamp poll(int timeoutMs,ChannelList* activeChannel) override; 
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    //填写活跃的连接
    void fillActiveChannels(int numEvents,ChannelList* activeChannels) const;
    //更新channel通道
    void update(int opearation,Channel* channel);

    static const int kInitEventListsSize = 16;
    using EventList = std::vector<struct epoll_event>;

    int epollfd_;
    EventList events_;
};