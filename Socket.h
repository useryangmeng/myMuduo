#pragma once

class InetAddress;

#include "noncopyable.hpp"

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        :sockfd_(sockfd)   
    {
    }
    ~Socket();

    int fd() {return sockfd_;}

    void bindAddress(const InetAddress& addr); 
    void listen();
    int accept(InetAddress* peerAddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};
