#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <errno.h>
#include <memory>
#include <sys/socket.h>
#include <string>

static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is nullptr!\n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop,
            const std::string& name,
            int sockfd,
            const InetAddress& localAddr,
            const InetAddress& peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(name)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop,sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64*1024*1024)//64M
{
    //设置channel的回调函数，事件发送，调用回调
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::hanleWrite,this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose,this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError,this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n",name_.c_str(),sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n",name_.c_str(),channel_->fd(),static_cast<int>(state_));
}

void TcpConnection::send(const std::string& buf)
{
    if(state_ == kConnected)
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(),buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,this,buf.c_str(),buf.size()));
        }
    }
}

void TcpConnection::sendInLoop(const void* data,size_t len)
{
    ssize_t nwrite = 0;
    size_t remianing = len;
    bool faultError = false;

    //之前调用过connection的shutdown，不能再进行发送
    if(state_ == kDisconnected)
    {
        LOG_ERROR("disconnected ,give up writing! \n");
    }

    //channel第一次写数据，而且缓冲区没有待发送的数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes()==0)
    {
        nwrite = ::write(channel_->fd(),data,len);
        if(nwrite >= 0)
        {
            remianing = len - nwrite;
            if(remianing == 0 && writeCompleteCallback_)
            {
                //既然数据一次发送完毕，就不用再给channel设置epollout事件了
                loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
            }
        }
        else //nwrite < 0
        {
            nwrite = 0;
            if(errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop \n");
                if(errno == EPIPE || errno ==ECONNRESET)//SIGPIPE RESET
                {
                    faultError = true;
                }
            }
        }
    }

    //说明当前这一次的write，并没有把数据全部发送出去，剩余的数据需要保存在缓冲区中，然后给channel
    //注册epollout事件，poller发现tcp的发送缓冲区有空间，会通知相应的sock-channel，调用handleWrite回调函数
    if(!faultError && remianing > 0)
    {
        //缓冲区原来待发送数据的长的胡
        size_t oldLen = outputBuffer_.readableBytes();
        if(oldLen + remianing > highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_,shared_from_this(),oldLen + remianing)); 
        }
        outputBuffer_.append((char*)data + nwrite,remianing);
        if(channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

//关闭连接
void TcpConnection::shutdown()
{
    if(state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting())//说明发送缓冲区的数据全部发送完毕,与hanleWrite的函数对应
    {
        socket_->shutdownWrite();//关闭写端，触发EPOLLHUP，与channel::handleEventWithGurd条件对应
    }
}

//建立连接
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();//向epoll注册epollin事件

    //新连接建立，执行回调
    connectionCallback_(shared_from_this());
}

//连接销毁
void TcpConnection::connectDestroyed()
{
    if(state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();//把channel所有感兴趣的事件从poller中del掉
        connectionCallback_(shared_from_this());
    }
    channel_->remove();//从poller中删除channel
}

void TcpConnection::handleRead(Timestamp receveTime)
{
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(),&saveErrno);
    if(n > 0)
    {
        //已建立连接的用户，有可读事件发生，调用用户定义的回调函数
        messageCallback_(shared_from_this(),&inputBuffer_,receveTime); 
    }
    else if(n == 0)
    {
        handleClose();
    }
    else
    {
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead! \n");
        handleError();
    }
}

void TcpConnection::hanleWrite()
{
    if(channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(),&saveErrno);
        if(n > 0)
        {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0)
            {
                channel_->disableReading();
                if(writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                }
                if(state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite! \n");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down,no more writing \n",channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n",channel_->fd(),static_cast<int>(state_));
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);
    closeCallback_(connPtr);
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if(::getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n",name_.c_str(),err);
}