#include <iostream>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <algorithm>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include "HttpResponse.h"

using std::string;

int HttpResponse::read(int* Errno) {
    return inBuff_.readFd(fd_, Errno);
}
int HttpResponse::read(int fd, int* Errno) {
    return inBuff_.readFd(fd, Errno);
}

int HttpResponse::write(int* Errno) {
    return outBuff_.writeFd(fd_, Errno);
}

/**
 * @brief 状态解析http请求报文和响应报文
 *
 * @param mode 1:server, 2:client
 * @return true 已经收到并解析完一个完整的http头
 * @return false 尚不构成一个头，需要您主动继续读取
 */
bool HttpResponse::parseResponse() {
    bool ok      = true;
    bool hasMore = true;
    while (hasMore) {
        // 解析请求行
        if (state_ == ExpectRequestLine) {
            const char* crlf = inBuff_.findCRLF();
            if (crlf) {
                ok = _parseResponseLine(inBuff_.peek(), crlf);
                if (ok) {
                    inBuff_.retrieveUntil(crlf + 2);
                    state_ = ExpectHeaders;
                } else
                    hasMore = false;
            } else
                hasMore = false;
        }

        // 解析请求头
        if (state_ == ExpectHeaders) {
            char* crlf = const_cast<char*>(inBuff_.findCRLF());
            if (crlf) {
                const char* colon = std::find(inBuff_.peek(), crlf, ':');
                if (colon != crlf)
                    _addHeader(inBuff_.peek(), colon, crlf);
                else {
                    state_  = GotAll;
                    hasMore = false;
                }
                inBuff_.retrieveUntil(crlf + 2);
            } else {
                hasMore = false;
            }
        }

        // body 我没有另外开辟内存来暂存body，所以当你处理body时，需要小心,
        // 下面的代码可以保证你读取一个完整的Http请求才会返回，无论你的body多大。
        // you must [getBody] or [discardBody] before handle next request,
        // otherwise, tcp-pack-stick is very possable.
        if (state_ == ExpectBody) {
#if 0
			int len = stoi(getHeader("Content-Length"));
			
			if(inBuff_.readableBytes() < len)
			{
				int tem = len - inBuff_.readableBytes();

				std::cout << "距离一个完整的包还差 "<< tem <<"\n";
				std::cout << "可写大小(扩容前)" <<inBuff_.writeableBytes() << "\n";
				std::cout << "可读大小（扩容前）"<<inBuff_.readableBytes()<<"\n";
				
				inBuff_.ensureWriteableBytes( tem );
				std::cout << "可写大小(扩容后)" <<inBuff_.writeableBytes() << "\n";
				std::cout << "可读大小（扩容后）"<<inBuff_.readableBytes()<<"\n";

				int ret = ::read(fd_, inBuff_.beginWrite(), tem);
				if (  ret != tem )
				{
					std::cout << "HttpResponse::parseResponse(): body read error"<<"  tem"<<tem<<"  ret"<<ret<<"\n";
				}
				inBuff_.hasWriten(tem);

				
				state_ = GotAll;
				hasMore = false;
			}
			else
			{
#if 0
					_stroeBody();
					inBuff_.retrieve(len);
#endif

				state_ = GotAll;
				hasMore = false;
			}
#endif
        }
    }
    return ok;
}

#if 0
void HttpResponse::_stroeBody()
{
	int len = std::stoi( getHeader("Content-Length"));
	// delete when resetParse 
	char *start = (char*)malloc( len );
	std::copy(start, start + len, inBuff_.peek());
	body_ = start;
}

int HttpResponse::getBodyByPointer(char *body)
{
	body = body_;
	int len = std::stoi( getHeader("Content-Length"));
	inBuff_.retrieveAsString(len);
}
#endif

std::string HttpResponse::getBodyByString() {
    int    len  = std::stoi(getHeader("Content-Length"));
    string body = inBuff_.retrieveAsString(len);
    return body;
}

int HttpResponse::getBodyToFd(int fd, int len, int* Errno) {
    return inBuff_.writeFd(fd, len, Errno);
}
void HttpResponse::throwAwayBody()
{
    int    len  = std::stoi(getHeader("Content-Length"));
    inBuff_.retrieve(len);
}

void HttpResponse::discardBody() {
    int len = std::stoi(getHeader("Content-Length"));
    inBuff_.retrieve(len);
}

bool HttpResponse::_parseResponseLine(const char* begin, const char* end) {
    bool        succeed = false;
    const char* start   = begin;
    const char* space   = std::find(start, end, ' ');

    if (space != end) {
        if (*(space - 1) == '1')
            _setVersion(HTTP11);
        else if (*(space - 1) == '0')
            _setVersion(HTTP10);
        else {
            return false;
        }
    }

    start = space + 1;
    space = std::find(start, end, ' ');

    if (space != end) {
        // string code = (start, space-1); error init
        // 双指针初始化string, string.back() = *(space-1)
        string code(start, space);
        code_ = stoi(code);
        start = space + 1;
        string msg(start, end);
        msg_    = msg;
        succeed = true;
    }

    return succeed;
}

/* [描述] 解析头的内部函数
 * [详情] WinSize是字符数组 */
void HttpResponse::_addHeader(const char* begin, const char* colon,
                              const char* end) {
    string filed(begin, colon);  // 首部字段
    ++colon;
    while (colon < end && *colon == ' ')
        ++colon;
    string value(colon, end);
    while (!value.empty() &&
           value[value.size() - 1] == ' ')  // 首部行的最后一行
        value.resize(value.size() - 1);  // 把最后一行的空格删掉
    headers_[filed] = value;
    // std::cout<<filed<<":"<<headers_[filed]<<std::endl;
}

void HttpResponse::resetParse() {
    state_   = ExpectRequestLine;  // 报文解析状态
    version_ = Unknown;            // HTTP版本
    code_    = 0;
    msg_     = "";
    headers_.clear();  // 报文头部
}

/* 获取首部内容，依据首部字段查找*/
string HttpResponse::getHeader(const string& field) const {
    string res;
    auto   itr = headers_.find(field);

    if (itr != headers_.end())
        res = itr->second;
    return res;
}

void HttpResponse::_setVersion(Version version) {
    version_ = version;
    // std::cout<<"协议版本："<<version_<<std::endl;
}

// /* 对pwrite的封装 */
// int Http::Pwrite(int fd, int len, off_t offset)
// {
// 	int ret = inBuff_.Pwrite(fd, len, offset);
// 	return ret;
// }

// int HttpRequest::ssl_write()
// {
// 	return outBuff_.sslSend(ssl_);
// }

/* [描述] 分割字符串
 * [参数] str:待分割串， pattern：分割符号
 */
// std::vector<string> Http::my_splitStr(const string &str, char pattern)
// {
// 	return inBuff_.split(str, pattern);
// }
