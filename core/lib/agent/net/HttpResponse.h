/**
 * @date 2022-11-11
 * @author liuxiangle
 * @brief create simple httpResponse or httpRequest
 */

#ifndef _HTTPREPONSE_H_
#define _HTTPREPONSE_H_

#include <string>
#include <map>
#include <memory>
#include "Buffer.h"
using std::string;

#define CONNECT_TIMEOUT 1000  // 非活跃连接500ms断开

class Buffer;
class HttpResponse {
public:
    //  enum Method{
    //     Get = GET,
    //     Pos = POST,
    //     PUT = PUT,
    //     DELETE = "DELETE"
    // };
    // enum Version{
    //     HTTP10,
    //     HTTP11 = "HTTP/1.1"
    // };

    // enum HttpStatusCode{
    //     kUnknown,
    //     k200Ok = 200,
    //     k301MovedPermanently = 301,
    //     k400BadRequest = 400,
    //     k404NotFound = 404,
    // };

    explicit HttpResponse(int code)
        : statusCode_(code),
          version_("HTTP/1.1"),
          closeConnection_(false),
          body_(NULL),
          content_length(0) {}

    // explicit HttpCreator(string line)
    //   : line_(line){}

    void setStatusCode(int code) {
        statusCode_ = code;
    }

    void setCloseConnection(bool on) {
        closeConnection_ = on;
    }

    bool closeConnection() const {
        return closeConnection_;
    }

    void setContentType(const string& contentType) {
        addHeader("Content-Type", contentType);
    }

    // FIXME: replace string with StringPiece
    void addHeader(const string key, const string value) {
        headers_[key] = value;
    }

    void setBody(char* body, int len) {
        body_          = body;
        content_length = len;
    }
    void setBody(const char* body, int len) {
        body_          = const_cast<char*>(body);
        content_length = len;
    }
    // void setBody(string body, int len)

    void setVersion(string version) {
        version_ = version;
    }

    void createResponse(Buffer* output);

private:
    // string line_;
    std::map<string, string>                headers_;
    static const std::map<int, std::string> statusCode2Message;

    int    statusCode_;
    string version_;

    bool closeConnection_;

    char* body_;
    int   content_length;
};

class AgentResponse : public HttpResponse {
public:
    int response_osInfo();
};

#endif
