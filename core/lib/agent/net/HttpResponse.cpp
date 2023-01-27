#include "HttpResponse.h"

#include <stdio.h>

const std::map<int, string> HttpResponse::statusCode2Message = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Sever Error"}};

void HttpResponse::createResponse(Buffer* output) {
    auto itr = statusCode2Message.find(statusCode_);
    if (itr == statusCode2Message.end()) {
        statusCode_ = 404;
        return;
    }
    string line = version_ + " " + std::to_string(statusCode_) + " " +
                  itr->second + "\r\n";

    output->append(line);

    if (closeConnection_) {
        output->append("Connection: close\r\n");
    } else {
        output->append("Content-Length: " + std::to_string(content_length) +
                       "\r\n");
        output->append("Connection: Keep-Alive\r\n");
    }

    for (const auto& header : headers_) {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }
    output->append("\r\n");

    if (body_)
        output->append(body_, content_length);
}