#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC ,0);
    if(sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d lisen socket create error! \n",__FILE__,__FUNCTION__,__LINE__);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking()) //socket
    , acceptChannel_(loop_,acceptSocket_.fd())
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);//bind

    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen();//listen

    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    InetAddress peerAddr(8888);
    int connfd = acceptSocket_.accept(&peerAddr);//accept
    if(connfd > 0)
    {
        if(newConnectonCallback_)
        {
            newConnectonCallback_(connfd,peerAddr);//轮询找到subloop，唤醒，分发当前客户端的channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else//error
    {
        LOG_ERROR("%s:%s:%d lisen socket accept error! \n",__FILE__,__FUNCTION__,__LINE__);
        if(errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d lisen socket reached limit! \n",__FILE__,__FUNCTION__,__LINE__);
        }
    }
}