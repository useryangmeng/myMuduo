#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>

//从fd上读取数据，Poller工作在LT模式
//Buffer缓冲区的大小有限，但是从fd上读取数据时，不知道tcp数据的最终大小
ssize_t Buffer::readFd(int fd,int* saveErrno)
{
    char extrabuf[65536] = {0};

    struct iovec vec[2];
    const size_t writeable = writeableBytes();
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len = writeable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writeable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = readv(fd,vec,iovcnt);
    if(n < 0)
    {
        *saveErrno = errno;
    }
    else if(n <= writeable)
    {
        writeIndex_ += n;
    }
    else//extrabuf也写入了数据
    {
        writeIndex_ = buffer_.size();
        append(extrabuf,n - writeable);
    }

    return n;
}