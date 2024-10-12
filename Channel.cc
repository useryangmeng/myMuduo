#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include "sys/epoll.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel()
{
}

// 防止当channel被手动remove掉，channel还在执行回调操作,当一个TcpConnection创建的时候调用
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

// 改变channel所表示的fd的事件后，更新poller里面fd的相应事件epoll_ctl
void Channel::update()
{
    // 通过channel所属的eventloop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 在channel所属的eventloop中，把当前的channel删除掉
void Channel::remove()
{
    loop_->removeChannel(this);
}

// fd得到poller通知以后，处理事件的函数
void Channel::handleEvent(Timestamp receiveTime)
{
    std::shared_ptr<void> guard;
    if(tied_)
    {
        guard = tie_.lock();
        if(guard)
        {
            handleEventWithGurd(receiveTime); 
        }
    }
    else
    {
        handleEventWithGurd(receiveTime);
    }
}

//根据poller通知的channel发生的具体事件，由channel负责调用具体的回调操作
void Channel::handleEventWithGurd(Timestamp receveTime)
{
    LOG_INFO("channel handleEvent revents:%d",revents_);

    //异常,挂起事件（一般为对端关闭文件描述符）
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    }

    if(revents_ & EPOLLERR)
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }

    //EPOLLPRI紧急事件
    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(receveTime);
        }
    }

    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }
}