/**
 * @date 2022-11-11
 * @author liuxiangle、muduo
 * @brief a simple http-status-parser(suport server、client)
 */
#ifndef _HTTP_CLIENT_H
#define _HTTP_CLIENT_H

#include "Buffer.h"
#include <string>
#include <vector>
#include <map>
#include <unistd.h>

using std::map;
using std::string;

// 解析响应数据
class HttpResponse {
public:
    enum HttpRequestParseState {
        ExpectRequestLine,
        ExpectHeaders,
        ExpectBody,
        GotAll
    };
    enum Version { Unknown, HTTP10, HTTP11 };

    HttpResponse(int fd)
        : fd_(fd),
          code_(0),
          msg_(""),
          version_(Unknown),
          state_(ExpectRequestLine) {}
    // listener_(ip, port){}

    ~HttpResponse() {
        close (fd_);
    }

    // int listener();
    bool parseResponse();  // 状态解析http报文
    bool parseFinish() {
        return state_ == GotAll;
    }
    void resetParse();  // 重置解析状态

    // int ssl_read();
    // int ssl_write();
    int read(int* Error);
    int read(int fd, int* Errno);
    int write(int* Error);
    // int Pwrite(int fd, int len, off_t offset);

    std::string getHeader(const std::string& field) const;
    std::string getBodyByString();
    int         getBodyToFd(int fd, int len, int* Errno);
    int         getFd() {
        return fd_;
    }
    int getCode() {
        return code_;
    }
    string getMsg() {
        return msg_;
    }
    // int getBodyByPointer(char *body);
    // void _stroeBody();
    void discardBody();
    // void print_progress_bar(const char *file_name,float sum,float
    // file_size);

private:
    bool _parseRequestLine(const char* begin,
                           const char* end);  // 解析请求行
    bool _parseResponseLine(const char* begin,
                            const char* end);  // 解析响应行
    void _setVersion(Version version);
    void _addHeader(const char* begin, const char* colon, const char* end);

private:
    int fd_;

    int                           code_;
    string                        msg_;
    Version                       version_;
    HttpRequestParseState         state_;
    map<std::string, std::string> headers_;

public:
    Buffer inBuff_;
    Buffer outBuff_;
    // Listener listener_;
};

#endif