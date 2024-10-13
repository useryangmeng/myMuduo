#pragma once

#include <vector>
#include <stddef.h>
#include <string>
#include <algorithm>

class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;
    
    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writeIndex_(kCheapPrepend)
    {
    }
    ~Buffer() = default;

    size_t readableBytes() { return writeIndex_ - readerIndex_; }
    size_t writeableBytes() { return buffer_.size() - writeIndex_; }
    size_t prependableBytes() { return readerIndex_; }

    //返回缓冲区中可读数据的起始地址
    const char* peek() const { return begin() + readerIndex_; }

    //读操作之后，index的位置更新
    void retrieve(size_t len) 
    {
        if(len < readableBytes())
        {
            readerIndex_ += len;
        }
        else//读取全部长度
        {
            retrieveAll();
        }
    }
    void retrieveAll() { readerIndex_ = writeIndex_ = kCheapPrepend; }

    //将buffer数据转换为string类型的数据返回
    std::string retrieveAllAsString() { return retrieveAsString(readableBytes()); }
    std::string retrieveAsString(size_t len) 
    {
        std::string result(peek(),len);
        retrieve(len);
        return result;
    }

    void ensureWriteableBytes(size_t len)
    {
        if(len > writeableBytes())
        {
            makeSpace(len);
        }
    }

    //把data，data+len内存上的数据，添加到writeable缓冲区当中
    void append(const char* data,size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data,data + len,beginWrite());
        writeIndex_ += len;
    }

    //write的内存位置
    char* beginWrite()
    {
        return begin() + writeIndex_; 
    }

    const char* beginWrite() const
    {
        return begin() + writeIndex_;
    }

    //从fd上读取数据
    ssize_t readFd(int fd,int* saveErrno);
    //通过fd发送数据
    ssize_t writeFd(int fd,int* saveErrno);

private:
    char* begin() { return &*buffer_.begin(); }
    const char* begin() const { return &*buffer_.begin(); }

    void makeSpace(size_t len)
    {
        if(writeableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writeIndex_ + len);
        }
        else
        {
            size_t readlen = readableBytes(); 
            std::copy(begin() + readerIndex_,
                    begin() + writeIndex_,
                    begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writeIndex_ = kCheapPrepend + len;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writeIndex_;
};
