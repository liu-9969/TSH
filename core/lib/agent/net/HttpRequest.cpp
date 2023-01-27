#include <assert.h>
#include <sys/stat.h>
#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <memory>

#include "HttpRequest.h"

// #include "../../config/setting.h"


// #define server 1
// #define client 2

// 这里，它作为HttpRequest,但是我希望它主动去连接CC

// int HttpRequest::ssl_read()
// {
// 	return inBuff_.sslRead(ssl_);
// }

int HttpRequest::read(int *Errno)
{
	return inBuff_.readFd(fd_, Errno);
}
int HttpRequest::read(int fd, int *Errno)
{
	return inBuff_.readFd(fd, Errno);
}

// int HttpRequest::ssl_write()
// {
// 	return outBuff_.sslSend(ssl_);
// }

int HttpRequest::write(int *Errno)
{
	return outBuff_.writeFd(fd_, Errno);
}





// /* 对pwrite的封装 */
// int HttpRequest::Pwrite(int fd, int len, off_t offset)
// {
// 	int ret = inBuff_.Pwrite(fd, len, offset);
// 	return ret;
// }



// /* 清空buffer */
// void HttpRequest::Reset_outBuf()
// {
// 	outBuff_.retrieveAll();
// }
// void HttpRequest::Reset_inBuf()
// {
// 	inBuff_.retrieveAll();
// }



/* [描述] 分割字符串 
 * [参数] str:待分割串， pattern：分割符号
 */
// std::vector<string> HttpRequest::my_splitStr(const string &str, char pattern)
// {
// 	return inBuff_.split(str, pattern);
// }




/**
 * @brief stateful parse one http Request pack
 * @return true:  recv and prase a cpmplete pack
 * @return false: pack is not complete, you need read more
 */
bool HttpRequest::parseRequest()
{
	bool ok = true;
	bool hasMore = true;
	while(hasMore)
	{
		// first line
		if(state_ == ExpectRequestLine)
		{
			const char* crlf = inBuff_.findCRLF();
			if(crlf)
			{
				ok = _parseRequestLine(inBuff_.peek(),crlf);
				if(ok)
				{
					inBuff_.retrieveUntil(crlf + 2);
					state_ = ExpectHeaders;
				}
				else
					hasMore = false;
			}
			else
				hasMore = false;
		}

		// header
		if(state_ == ExpectHeaders)
		{
			char* crlf = const_cast<char*>(inBuff_.findCRLF());
			if(crlf)
			{
				const char* colon = std::find(inBuff_.peek(),crlf,':');
				if(colon != crlf)
					_addHeader(inBuff_.peek(),colon,crlf);
				else
				{
					state_ =ExpectBody;
				}     
				inBuff_.retrieveUntil(crlf + 2);
			}
			else{
				hasMore = false;
			}
		}

		// body
		if(state_ == ExpectBody)
		{
			int len = stoi(getHeader("Content-Length"));
			if(inBuff_.readableBytes() < len)
			{
				hasMore = true;
			}
			else
			{
				// in here, maybe the next pack is pre-readed
				state_ = GotAll;
				hasMore = false;
			}
		}
	}
	return ok;
}

#if 0
void HttpRequest::_stroeBody()
{
	int len = std::stoi( getHeader("Content-Length"));
	// delete when resetParse 
	char *start = (char*)malloc( len );
	// std::copy(start, start + len, inBuff_.peek());
	memcpy(start, inBuff_.peek(), len);
	body_ = start;
	// inBuff_.retrieve(len);
}

int HttpRequest::getBodyByPointer(char *body)
{
	body = body_;
	int len = std::stoi( getHeader("Content-Length"));
	inBuff_.retrieveAsString(len);
}
#endif

std::string HttpRequest::getBodyByString()
{
	int len = std::stoi( getHeader("Content-Length"));
	string body = inBuff_.retrieveAsString(len);
	return body;
}

std::vector<char> HttpRequest::getBodyByArry()
{
	int len = std::stoi(getHeader("Content-Length"));
	return inBuff_.retrieveAsVector(len);
}

void HttpRequest::discardBody()
{
	int len = std::stoi( getHeader("Content-Length"));
	inBuff_.retrieve(len);
}

bool HttpRequest::_parseRequestLine(const char* begin,const char* end)
{
    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start,end,' '); //解析方法
    if(space != end && _setMethod(start,space))
    {
        start = space + 1;
        space = std::find(start,end,' ');//解析路径
        if(space != end)
        {
            const char* question = std::find(start,space,'?');
            if(question != space)
            {
                _setPath(start,question);
                _setQuery(question+1,space);
            }
            else
                _setPath(start, space);
        
            start = space + 1;//解析版本
            //string m(start,end),字符串最后一个字符为*（end-1）
            succeed = end - start == 8 && std::equal(start,end-1,"HTTP/1.");
            if(succeed)
            {
                if(*(end - 1 ) == '1')
                    _setVersion(Http11);
                else if(*(end - 1) == '0')
                    _setVersion(Http10);
                else
                    succeed = false;
            }
        }
    }
    return succeed;
}






/* [描述] 解析头的内部函数 
 * [详情] WinSize是字符数组 */

void HttpRequest::_addHeader(const char* begin,const char* colon,const char* end)
{
	string filed(begin,colon);//首部字段
	
	++ colon;
	while(colon < end && *colon == ' ')
		++ colon;
	string value(colon,end);
	while(!value.empty() && value[value.size()-1] == ' ')//首部行的最后一行
		value.resize(value.size()-1);//把最后一行的空格删掉
	headers_[filed] = value;
	// std::cout<<filed<<":"<<headers_[filed]<<std::endl;
}


void HttpRequest::resetParse()
{
	state_ = ExpectRequestLine; // 报文解析状态
    method_ = Invalid; // HttpRequest方法
    version_ = Unknown; // HttpRequest版本
    path_ = ""; // URL路径
    query_ = ""; // URL参数
    headers_.clear(); // 报文头部
}

bool HttpRequest::_setMethod(const char*begin,const char* end)
{
    string m(begin,end);
    //std::cout<<"请求方法："<<m<<std::endl;
    if(m == "GET")
        method_ = Get;
    else if(m == "POST")
        method_ = Post;
    else if(m == "HEAD")
        method_ = Head;
    else if(m == "PUT")
        method_ = Put;
    else if(m == "DELETE")
        method_ = Delete;
    else
        method_ = Invalid;

    return method_ != Invalid; 
}

/* 获取首部内容，依据首部字段查找*/
string HttpRequest::getHeader(const string& field) const
{
	string res;
	auto itr = headers_.find(field);
	if(itr != headers_.end())
		res = itr -> second;
	return res;
}

void HttpRequest::_setQuery(const char* begin,const char* end)
{
    query_.assign(begin,end);
}

void HttpRequest::_setVersion(Version version)
{
    version_ = version;
    //std::cout<<"协议版本："<<version_<<std::endl;
}

void HttpRequest::_setPath(const char* begin,const char* end)
{
    string subPath;
    subPath.assign(begin,end);
    //std::cout<<"路径："<<subPath<<std::endl;
    if(subPath == "/")//请求没有指定路径
        subPath = "index.html";//给他一个默认路径
    path_ = subPath;
    //std::cout<<"请求路径："<<path_<<std::endl;
}



