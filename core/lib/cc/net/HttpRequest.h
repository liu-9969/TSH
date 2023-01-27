/**
 * @date 2022-11-11
 * @author liuxiangle
 * @brief create simple httpResponse or httpRequest
 */

#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include <string>
#include <map>
#include <memory>
#include "Buffer.h"
using std::string;

#define CONNECT_TIMEOUT 1000  // 非活跃连接500ms断开

class Buffer;
class HttpResquest {
public:
    HttpResquest()
        : method_("POST"),
          url_("/"),
          query_(""),
          version_("HTTP/1.1"),
          closeConnection_(false),
          body_(NULL),
          content_length(0) {}

    // explicit HttpResquest(string line)
    //   : line_(line){}

    void setCloseConnection(bool on) {
        closeConnection_ = on;
    }

    bool closeConnection() const {
        return closeConnection_;
    }

    void setQuery(string query) {
        query_ = query;
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
    void setMethod(string method) {
        method_ = method;
    }

    void setUrl(string url) {
        url_ = url;
    }

    void setVersion(string version) {
        version_ = version;
    }

    void createRequest(Buffer* output);

private:
    // string line_;

    string                   method_;
    string                   url_;
    string                   query_;
    string                   version_;
    bool                     closeConnection_;
    std::map<string, string> headers_;

    char* body_;
    int   content_length;
};

#endif
