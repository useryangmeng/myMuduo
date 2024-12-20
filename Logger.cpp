#include "Logger.h"
#include "Timestamp.h"

#include <iostream>

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}

// 日志输出 [级别信息] time ： xxx
void Logger::log(std::string msg)
{
    switch(logLevel_)
    {
    case INFO: 
        std::cout << "[INFO] ";
        break;
    case ERROR: 
        std::cout << "[ERROR] ";
        break;
    case FATAL: 
        std::cout << "[FATAL] ";
        break;
    case DEBUG: 
        std::cout << "[DEBUG] ";
        break;
    default:
        break;
    }

    //打印时间和message
    std::cout << Timestamp::now().toString() <<" : " << msg << std::endl;
}
