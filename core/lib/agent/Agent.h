#ifndef _HTTPSERVER_
#define _HTTPSERVER_

#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>
#include "net/HttpRequest.h"
#include "net/HttpResponse.h"
#include "SShd.h"

/**
 * @brief agent as a httpserver，also as the initiator of the tcp connection
 *        if CC coredump, the httpserver will restart and try conn again
 */

class HttpServer {
public:
    using fileDownloadCallBack = std::function<int(HttpRequest*, string)>;
    using fileUploadCallBack   = std::function<int(HttpRequest*, string)>;

    HttpServer(string ccIp, int shellPort, int filePort)
        : ccIp_(ccIp), ports_({shellPort, filePort}) {
        // 1. register callback
        getfile_ = std::bind(&HttpServer::getFile, this,
                             std::placeholders::_1, std::placeholders::_2);

        putfile_ = std::bind(&HttpServer::putFile, this,
                             std::placeholders::_1, std::placeholders::_2);

        // 2. connect with CC by two tcp-connection
        _connec();

        // 3. init file threadpool
        fileManager_ = FileManager::getInstance();

        // 4. run the rshell in a new thread
        threads_.emplace_back([this]() {
            prctl(PR_SET_NAME, "shell");

            SShd::getInstance(fds_[0]);// new when call-start,delete when call-end

            // httpServerResart();

            std::cout << "HttpServer::sshd_start(): "
                         "跳出shell-loop回到main主分支\n";
            return;
        });
    }

    ~HttpServer() {}

public:
    int get_shellFd() {
        return fds_[0];
    }
    int get_fileFd() {
        return fds_[1];
    }

    int httpServerResart();

    int _events_CC_readable(HttpRequest* httphandler);

    int getFile(HttpRequest* httpHandler, string path);
    int putFile(HttpRequest* httpHandler, string path);

private:
    int _Response_osInfo();
    int _connec();

private:
    string           ccIp_;
    std::vector<int> fds_;
    std::vector<int> ports_;  // 0：shell  1:file

    std::vector<std::thread>     threads_;
    std::shared_ptr<FileManager> fileManager_;
    SShd*                        sshd_;

public:
    fileDownloadCallBack getfile_;
    fileUploadCallBack   putfile_;

    enum ReturnNum {
        SUCCESS = -100,
        ERROR,
        CLOSE,
    };
};
#endif