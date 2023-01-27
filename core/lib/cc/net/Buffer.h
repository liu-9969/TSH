/*
 * @date 2022-11-11
 * @author liuxiangle muduo
 * @brief Buffer in App layer with stateful and auto expansion
 *
 * 应用程序不需要关心

@code

     +------------------------------------------------------------------------+
     |    perpendable     |      readable bytes      |    writeable bytes |
     |                    |         (content)        | |
     +------------------------------------------------------------------------+
     |   already handle   |       wait handle        |        none use | 0
<=     readerIndex        <=       writerIndex      <=        size()

@endcode

*/
#ifndef _BUFFER_H_
#define _BUFFER_H_
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <vector>
#include <assert.h>
#include <string>
#include <algorithm>

class Buffer {
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize  = 4096;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend) {
        assert(readableBytes() == 0);
        assert(writeableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    ~Buffer() {}

public:
    // 返回可读、可写、缓冲区前空闲字节数
    // 返回可写位置
    // 返回可读位置
    // 写入数据后，移动writerIndex
    ssize_t readFd(int fd, int* saveErrno);
    ssize_t writeFd(int fd, int* saveErrno);
    ssize_t writeFd(int fd, int len, int* saveErrno);
    size_t  readN(int fd, int len, int* Errno);

    // ssize_t sslRead(SSL *ssl);
    // ssize_t sslSend(SSL *ssl);

    // ssize_t readFd(int fd);
    // ssize_t writeFd(int fd);

    // int readN (SSL *ssl, int len);

    size_t printfBufSize() {
        return buffer_.size();
    }

    size_t readableBytes() const {
        return writerIndex_ - readerIndex_;
    }

    size_t writeableBytes() const {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const {
        return readerIndex_;
    }

    char* beginWrite() {
        return _begin() + writerIndex_;
    }

    const char* beginWrite() const {
        return _begin() + writerIndex_;
    }

    const char* peek() const {
        return _begin() + readerIndex_;
    }

    char* peek() {
        return _begin() + readerIndex_;
    }

    void append(const void* data, size_t len) {
        append(static_cast<const char*>(data), len);
    }

    void append(const Buffer& other) {
        append(other.peek(), other.readableBytes());
    }

    void append(const std::string& str)  // 插入数据
    {
        append(str.data(), str.length());
    }

    void hasWriten(size_t len) {
        writerIndex_ += len;
    }

    // 读取数据后，readIndex移动len个字节
    void retrieve(size_t len) {
        assert(len <= readableBytes());
        if (len < readableBytes()) {
            readerIndex_ += len;
        } else {
            retrieveAll();
        }
    }
    // 读至end
    void retrieveUntil(const char* end) {
        assert(end >= peek());
        assert(end <= beginWrite());
        retrieve(end - peek());
    }
    // 初始化
    void retrieveAll() {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // 以string方式读取全部内容
    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    // 以string方式读取len个字节
    std::string retrieveAsString(size_t len) {
        assert(len <= readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    // 以数组方式读取全部内容
    std::vector<char> retrieveAsVector() {
        std::vector<char> result;
        while (readableBytes()) {
            result.push_back(buffer_[readerIndex_]);
            readerIndex_++;
        }
        return result;
    }
    ///
    // 确保writeable Bytes有足够空间
    void ensureWriteableBytes(size_t len) {
        if (len > writeableBytes()) {
            _makeSpace(len);
        }
        assert(writeableBytes() >= len);
    }
    // 插入数据
    void append(const char* data, size_t len) {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        hasWriten(len);
    }

    // 查找"\r\n"
    const char* findCRLF() const {
        const char  CRLF[] = "\r\n";
        const char* crlf =
            std::search(peek(), beginWrite(), CRLF, CRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }
    const char* findCRLF(const char* start) const {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const char  CRLF[] = "\r\n";
        const char* crlf = std::search(start, beginWrite(), CRLF, CRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

private:
    // 返回缓冲区开始的指针
    char* _begin() {
        return &*buffer_.begin();
    }
    const char* _begin() const {
        return &*buffer_.begin();
    }
    // 确保writeable Bytes有足够的空间
    void _makeSpace(size_t len) {
        if (writeableBytes() + prependableBytes() < len + kCheapPrepend) {
            buffer_.resize(writerIndex_ + len);
        } else {
            assert(kCheapPrepend < readerIndex_);
            size_t readable = readableBytes();
            // FIXME:move bytes to prepend
            std::copy(_begin() + readerIndex_, _begin() + writerIndex_,
                      _begin() + +kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());  // 是否全部拷贝到头部
        }
    }

private:
    std::vector<char> buffer_;
    size_t            readerIndex_;
    size_t            writerIndex_;
};

#endif
