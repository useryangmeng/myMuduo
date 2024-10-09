#pragma once

#include "noncopyable.hpp"
#include "InetAddress.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "Timestamp.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <memory>
#include <atomic>

class Buffer;
class TcpConnection;


class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const std::string& nameArg,
            Option option = kNoReusePort);
    ~TcpServer();

    const std::string& ipPort() { return ipPort_;}//查询接口
    const std::string& name() { return name_; }
    EventLoop* getLoop() { return loop_; }

    void setThreadNum(int threadNum);//设置subloop的个数
    void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }
    std::shared_ptr<EventLoopThreadPool> threadPool() { return threadPool_;}

    void start();//开启监听 acceptor

    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }//设置业务回调
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }


private:
    void newConnection(int sockfd,const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string,TcpConnectionPtr>;   

    EventLoop* loop_; //acceptor loop
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;//运行在mainLoop，用于监听新的事件
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;//业务回调
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
     
    ThreadInitCallback threadInitCallback_;//线程初始化回调
    std::atomic_int32_t started_;

    int nextConnId_;
    ConnectionMap connections_;//保存所有的连接
};
