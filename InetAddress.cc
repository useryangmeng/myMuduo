#include "InetAddress.h"
#include <cstring>

InetAddress::InetAddress(uint16_t port,std::string ip)
{
    memset(&addr_, 0, sizeof(addr_));
    //inet_net_pton(AF_INET, ip.c_str(), &addr_.sin_addr.s_addr, sizeof(ip.c_str()));
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
}

std::string InetAddress::toIp() const
{
    //ip
    char ip[128] = {0};
    inet_ntop(AF_INET,&addr_.sin_addr,ip,sizeof(ip));
    return ip;
}

std::string InetAddress::toIpPort() const
{
    //ip : port
    char buf[128] = {0};
    inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof(buf));
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf+end,":%u",port);
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

// #include <iostream>
// int main()
// {
//     InetAddress addr(8080);
    
//     std::cout << ntohs(addr.getSockAddr()->sin_port) << std::endl;
//     return 0;
// }