#include "Socket.h"
#include "InetAddress.h"
#include "Logger.h"

#include "unistd.h"
#include "sys/socket.h"
#include "string.h"
#include "sys/types.h"
#include <netinet/tcp.h>


Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress& addr)
{
    if(0 != ::bind(sockfd_,(struct sockaddr*)addr.getSockAddr(),sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind socket: %d fail \n",sockfd_);
    }
}

void Socket::listen()
{
    if(0 != ::listen(sockfd_,8))
    {
        LOG_FATAL("listen socket:%d fail! \n",sockfd_);
    }
}

int Socket::accept(InetAddress* peerAddr)
{
    int ret = 0;
    struct sockaddr_in caddr;
    memset(&caddr,0,sizeof(caddr));
    socklen_t len = 0;

    ret = ::accept(sockfd_,(struct sockaddr*)&caddr,&len);
    // if(ret == -1)
    // {
    //     LOG_FATAL("Accept socket:%d fail ! \n",sockfd_);
    // }
    if(ret >= 0)
    {
        peerAddr->setSockAddr(caddr);
    }
    return ret;
}

void Socket::shutdownWrite()
{
    if(::shutdown(sockfd_,SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite sock:%d fail \n",sockfd_);
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int opt = on ? 1 : 0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&opt,sizeof(opt));
}

void Socket::setReuseAddr(bool on)
{
    int opt = on ? 1 : 0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));    
}

void Socket::setReusePort(bool on)
{
    int opt = on ? 1 : 0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&opt,sizeof(opt));
}

void Socket::setKeepAlive(bool on)
{
    int opt = on ? 1 : 0;
    ::setsockopt(sockfd_,IPPROTO_TCP,SO_KEEPALIVE,&opt,sizeof(opt));
}
