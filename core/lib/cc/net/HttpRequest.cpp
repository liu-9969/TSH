#include "HttpRequest.h"
#include <stdio.h>

void HttpResquest::createRequest(Buffer* output) {
    string line;
    if (query_ != "")
        line =
            method_ + " " + url_ + "?" + query_ + " " + version_ + "\r\n";
    else
        line = method_ + " " + url_ + " " + version_ + "\r\n";

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
    if (body_) {
        output->append(body_, content_length);
    }
}