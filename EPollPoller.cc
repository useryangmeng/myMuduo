#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <unistd.h>
#include <errno.h>
#include <cstring>

// channel 未添加到poller中
const int kNew = -1; // channel 的index初始化为-1
// channel 已添加到poller中
const int kAdded = 1;
// channel 从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListsSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error %d \n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannel)
{
    //实际上poll的高并发，应该使用debug日志
    LOG_INFO("func = %s => fd total count:%lu\n",__FUNCTION__,channels_.size());

    int numEvents = epoll_wait(epollfd_,&(*events_.begin()),static_cast<int>(events_.size()),timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now()); 

    if(numEvents > 0)
    {
        LOG_INFO("%d events happened\n",numEvents);
        fillActiveChannels(numEvents,activeChannel);
        if(numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout\n",__FUNCTION__);
    }
    else
    {
        if(saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err! \n");
        }
    }
    
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
/*
 *                   EventLoop
 *       ChannelList         Poller
 *                     ChannelMap<fd,channel*> epollfd
 */
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func = %s => fd = %d events = %d index = %d\n", __FUNCTION__, channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else // channel已经注册
    {
        int fd = channel->fd();
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从poller中删除channel
void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    int index = channel->index();

    LOG_INFO("func = %s => fd = %d \n", __FUNCTION__, fd);

    channels_.erase(fd);
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
        channel->set_index(kDeleted);
    }

    channel->set_index(kNew);
}

// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for(int i = 0;i < numEvents;i++)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);//这样EventLoop就拿到了它的poller给它返回的所有发生事件的channel的事件列表了
    }

}

// 更新channel通道,epoll_ctl add/mod/del的具体操作
void EPollPoller::update(int operation, Channel *channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    LOG_INFO("EPollPoller::update:Channel = %p",channel);
    event.data.ptr = channel;
    LOG_INFO("EPollPoller::update:event.data.ptr = %p",event.data.ptr);
    //event.data.fd = channel->fd();
    int fd = channel->fd();

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_ADD)
        {
            LOG_FATAL("epoll_ctl_add error %d \n", errno);
        }
        else if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl_del error %d \n", errno);
        }
        else if (operation == EPOLL_CTL_MOD)
        {
            LOG_FATAL("epoll_ctl_mod error %d \n", errno);
        }
    }
}