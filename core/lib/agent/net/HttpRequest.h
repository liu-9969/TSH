#ifndef _BASE_HttpRequest_H
#define _BASE_HttpRequest_H
#include "Buffer.h"
#include <string>
#include <vector>
#include <map>
#include <unistd.h>

using std::map;
using std::string;

// 解析请求报文
class HttpRequest {
public:
    enum HttpRequestParseState {
        ExpectRequestLine,
        ExpectHeaders,
        ExpectBody,
        GotAll
    };
    enum Method { Invalid, Get, Post, Head, Put, Delete };
    enum Version { Unknown, Http10, Http11 };

    HttpRequest(int fd)
        : fd_(fd),
          state_(ExpectRequestLine),
          method_(Invalid),
          version_(Unknown),
          path_(""),
          query_("") {}

    ~HttpRequest() {
        // close (fd_);
    }

public:
    bool parseRequest();  // 状态解析HttpRequest报文
    bool parseFinish() {
        return state_ == GotAll;
    }
    void resetParse();  // 重置解析状态

    int read(int* Error);
    int read(int fd, int* Errno);

    int write(int* Error);
    int ssl_read();
    int ssl_write();
    // int Pwrite(int fd, int len, off_t offset);

    std::string getPath() const {
        return path_;
    }
    std::string getQuery() const {
        return query_;
    }
    std::string       getHeader(const std::string& field) const;
    std::string       getBodyByString();
    std::vector<char> getBodyByArry();
    void              discardBody();
    int               getFd() {
        return fd_;
    }
    // void print_progress_bar(const char *file_name,float sum,float
    // file_size);

private:
    bool _parseRequestLine(const char* begin,
                           const char* end);  // 解析请求行
    bool _setMethod(const char* begin, const char* end);
    void _setPath(const char* begin, const char* end);
    void _setQuery(const char* begin, const char* end);
    void _setVersion(Version version);
    void _addHeader(const char* begin, const char* colon, const char* end);
    // void _stroeBody();
    int _connect();

public:
    Buffer inBuff_;
    Buffer outBuff_;

private:
    int fd_;

    // SSL *ssl_;

    Method  method_;
    Version version_;
    string  path_;
    string  query_;

    HttpRequestParseState         state_;
    map<std::string, std::string> headers_;
};

#endif
