#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>

#include <string>
#include <functional>

class echoServer
{
public: 
    echoServer(const InetAddress& listenAddr,EventLoop* loop,const std::string name) : server_(loop,listenAddr,name) ,loop_(loop)
    {
        server_.setConnectionCallback(std::bind(&echoServer::onConnection,this,std::placeholders::_1));
        server_.setMessageCallback(std::bind(&echoServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

        server_.setThreadNum(3);
    }

    void start()
    {
        server_.start();
    }
private:

    void onConnection(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_INFO("Connection UP : %s",conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN : %s",conn->peerAddress().toIpPort().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp time)
    {
        std::string buffer = buf->retrieveAllAsString();
        conn->send(buffer);
        conn->shutdown(); //EPOLLHUP -> close
    }

    TcpServer server_;
    EventLoop* loop_;
};


int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    echoServer server(addr,&loop,"echo");

    server.start();
    loop.loop();

    return 0;
}