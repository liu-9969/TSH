#include "Buffer.h"
#include <cstring>  // perror
#include <iostream>
#include <unistd.h>   // write
#include <sys/uio.h>  // readv

// ssize_t Buffer::readFd(int fd)
// {
//     int n = 0;
//     n = read(fd, beginWrite(), 4096);

//     if(n > 0)
//     {
//         writerIndex_ += n;
//     }

//     return n;
// }

ssize_t Buffer::readFd(int fd, int* saveErrno) {
    char         extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writeableBytes();
    vec[0].iov_base       = _begin() + writerIndex_;
    vec[0].iov_len        = writable;
    vec[1].iov_base       = extrabuf;
    vec[1].iov_len        = sizeof(extrabuf);
    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    const int     iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n      = ::readv(fd, vec, iovcnt);
    if (n < 0) {
        *saveErrno = errno;
    } else if (static_cast<size_t>(n) <= writable)
        writerIndex_ += n;
    else {
        writerIndex_ = buffer_.size();  // writerIndex移动到最后
        append(extrabuf, n - writable);
    }
    return n;
}

size_t Buffer::readN(int fd, int len, int* Errno) {
    int n, sum = 0;
    while (sum < len) {
        n = ::read(fd, beginWrite(), len - sum);
        if (n < 0) {
            *Errno = errno;
            return -1;
        }
        hasWriten(n);
        sum += n;
    }
    return sum;
}

ssize_t Buffer::writeFd(int fd, int len, int* savedErrno) {
    int n, sum = 0;
    while (sum < len) {
        n = ::write(fd, peek(), len - sum);

        if (n < 0) {
            *savedErrno = errno;
            return -1;
        }

        retrieve(n);
        sum += n;
    }
    return sum;
}

// this one容易出现粘包
ssize_t Buffer::writeFd(int fd, int* savedErrno) {
    int n;
    int sum = readableBytes();

    while (readableBytes() != 0) {
        char*  bufPtr = peek();
        size_t nLeft  = readableBytes();
        n             = ::write(fd, bufPtr, nLeft);
        if (n < 0) {
            *savedErrno = errno;
            return -1;
        }
        retrieve(n);
    }

    return sum;
}

// ssize_t Buffer::sslRead(SSL *ssl)
// {
//     int n = 1;

//     n = SSL_read(ssl, beginWrite(), 4096);

//     if( n > 0)
//     {
//         writerIndex_ += n;
//     }

//     return n;
// }

// ssize_t Buffer::sslSend(SSL *ssl)
// {
//     int sum = 0;
//     int n = 1;

//     while(n > 0)
//     {
//         n = SSL_write(ssl, peek(), readableBytes());
//         if(n > 0)
//         {
//             readerIndex_ += n;
//             sum += n;
//         }
//         else if(n == 0)
//         {
//             break;
//         }
//         else
//             return -1;
//     }

//     return sum;
// }

// ssize_t Buffer::readFd(int fd)
// {
//     int sum = 0;
//     int n = 1;
//     while( 1)
//     {
//         n = ::read(fd, beginWrite(), 4096);

//         if( n > 0)
//         {
//             writerIndex_ += n;
//             sum += n;
//         }
//         else if( n == 0)
//         {
//             break;
//         }
//         else
//             return -1;
//     }

//     return sum;
// }

// ssize_t Buffer::writeFd(int fd)
// {
//     int n = 0;
//     write(fd, peek(), readableBytes());

//     if(n > 0)
//     {
//         readerIndex_ += n;
//     }

//     return n;
// }

// int Buffer::readN (SSL *ssl, int len)
// {
// 	int sum = 0;
// 	int n = -1;

// 	while(sum < len)
// 	{
//         n = SSL_read(ssl, beginWrite(), len);
//         if(n < 0)
//             return -2;

//         if( n == 0)
//             return -1;

//         sum += n;
// 	}
// 	writerIndex_ += sum;
// 	return sum;
// }

// 如果buffer缓冲区未装下，把extrabuf中的内容添加进buffer
// readv无法使vector进行扩容，当n太大时，会把vector填满并且剩余字节读到extrabuf，然后我们已经准备好成员函数makespace进行扩容，
// 所以我们调用append()，把extrabuf里的数据拷贝到vector。
