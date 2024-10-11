#pragma once

#include "noncopyable.hpp"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                 const std::string& name,
                 int sockfd,
                 const InetAddress& localAddr,
                 const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    bool connected() const {return state_ == kConnected;}

    //发送数据
    void send(const void* message,int len);
    void send(const std::string& buf);
    //关闭连接
    void shutdown(); 


    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb) { highWaterMarkCallback_ = cb; }
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

    //连接建立
    void connectEstablished();
    //连接销毁
    void connectDestroyed();


private:
    enum StateE { kDisconnected,kConnecting,kConnected,kDisconnecting };
    void handleRead(Timestamp receveTime);
    void hanleWrite();
    void handleClose();
    void handleError();

    void setState(StateE state) { state_ = state; }

    void sendInLoop(const void* message,size_t len); 
    void shutdownInLoop();


    EventLoop* loop_;//is not baseLoop
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;//业务回调
    MessageCallback messageCallback_;

    WriteCompleteCallback writeCompleteCallback_;

    HighWaterMarkCallback highWaterMarkCallback_;//水位标志、回调
    CloseCallback closeCallback_;

    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};

