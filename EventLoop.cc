#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <unistd.h>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 防止一个线程创建多个EventLoop thread_local
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认Poller默认的IO接口复用的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd，用来notify唤醒subReactor处理新来的channel
int createEventFd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error : %d\n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , weakupFd_(createEventFd())
    , weakupChannel_(new Channel(this,weakupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n",this,threadId_);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p in thread %d \n",t_loopInThisThread,threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    //设置wakeupfd的事件类型，以及发生事件后的回调操作
    weakupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    weakupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    weakupChannel_->disableAll();
    weakupChannel_->remove();
    ::close(weakupFd_);
    t_loopInThisThread = nullptr;
    //其他new出来的智能指针自动处理
}

//开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n",this);

    while(!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs,&activeChannels_);
        for(Channel* channel : activeChannels_)
        {
            //Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        //执行当前EventLoop事件循环需要处理的回调操作
        /**  
         * IO线程 ： mainloop accept fd -> subloop
         * mainloop 事先注册一个回调cb（需要subloop来执行） wakeup subloop后执行下面的方法，执行之前mainloop注册的cb操作
         */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping \n",this);
    looping_ = false;
}

//退出事件循环 1.loop在自己的线程中调用quit 2.在非loop的线程中，调用loop的quit
void EventLoop::quit()
{
    quit_ = true;
    if(!isInLoopThread())//如果在其他线程中，调用quit  如：在一个subloop（/worker）中，调用了mainloop（IO）的quit
    {
        weakup();
    }
}

//在当前loop执行
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())//在当前的loop线程中，执行cb
    {
        cb();
    }
    else//在非当前loop线程中执行cb，那么就需要唤醒loop所在线程，执行cb
    {
        queueInLoop(cb);
    }
}

//把cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    //唤醒相应的 需要执行上面回调操作的loop线程了
    if(!isInLoopThread() || callingPendingFunctors_)//第二个条件对应自己线程执行回调，当为true时，因为顺序执行完后会阻塞在poll中，使自己线程发起的cb无法执行
    {
        weakup(); //唤醒loop所在线程
    }
}


//用来唤醒loop所在的线程 向weakupfd_写一个数据,weakup Channel就会发生读事件，当前loop线程就会被唤醒
void EventLoop::weakup()
{
    uint64_t one = 1;
    ssize_t n = write(weakupFd_,&one,sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::weakup() writes %lu bytes instead of 8",n);
    }
}

//EventLoop的方法 -> Poller的方法
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

//weak up
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(weakupFd_,&one,sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8",n);
    }
}

//执行回调
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor& functor : functors)
    {
        functor();//执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}