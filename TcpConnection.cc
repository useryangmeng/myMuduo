#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"



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
    , highWaterMark_(64*1024*1024)
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