#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoopThreadPool.h"

#include <strings.h>

TcpServer::TcpServer(EventLoop *loop,
          const InetAddress &listenAddr,
          const std::string &nameArg,
          Option option)
    : loop_(loop)
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
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }   
}

void TcpServer::setThreadNum(int threadNum)
{

}

void TcpServer::start()
{

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