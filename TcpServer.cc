#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoopThreadPool.h"
#include "TcpConnection.h"
#include "Logger.h"

#include <strings.h>

EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is nullptr!\n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

int num = 0;

TcpServer::TcpServer(EventLoop *loop,
          const InetAddress &listenAddr,
          const std::string &nameArg,
          Option option)
    : loop_(CheckLoopNotNull(loop))
    , ipPort_(listenAddr.toIpPort())
    , name_(nameArg)
    , acceptor_(new Acceptor(loop,listenAddr,option == kReusePort))
    , threadPool_(new EventLoopThreadPool(loop,nameArg))
    , connectionCallback_(nullptr)
    , messageCallback_(nullptr)
    , nextConnId_(1)
{
    acceptor_->setConnectionCallback(std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for(auto& it : connections_) 
    {
        TcpConnectionPtr conn(it.second);
        it.second.reset();
        //conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }   
}

void TcpServer::setThreadNum(int threadNum)
{
    threadPool_->setThreaNum(threadNum);
}

void TcpServer::start()
{
    if(started_++ == 0)
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{

}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{

}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{

}