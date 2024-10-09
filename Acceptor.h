#pragma once

#include "noncopyable.hpp"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;

    Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);
    ~Acceptor();

    void setConnectionCallback(const NewConnectionCallback& cb) {newConnectonCallback_ = cb;}
    bool listening() { return listenning_; }
    void listen();
private:
    void handleRead();

    EventLoop* loop_;//Acceptor所在的就是用户定义的eventloop中，即mainloop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectonCallback_;
    bool listenning_;
};

