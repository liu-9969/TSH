#ifndef _HTTPSERVER_
#define _HTTPSERVER_

#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>
#include <sys/types.h>
#include <unistd.h>
#include "net/HttpRequest.h"
#include "net/HttpResponse.h"
#include "SShd.h"
#include "../../setting.h"

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

        // 3. threadpool for file translate
        fileManager_ = FileManager::getInstance();

        // 4. thread for tcp-ports lensten
        threads_.emplace_back([this]() -> int {
            fd_set rd;
            int    ret;
            int    shellFd = get_shellFd();
            int    fileFd  = get_fileFd();

            while (1) {
                FD_SET(fileFd, &rd);

                if (select(fileFd + 1, &rd, NULL, NULL, NULL) < 0)
                    return -1;

                if (FD_ISSET(fileFd, &rd)) {
                    HttpRequest* httphandler = new HttpRequest(fileFd);

                    switch (_events_CC_readable(httphandler)) {
                        case SUCCESS:
                            break;
                        case CLOSE:
                            delete httphandler;
                            {
                                std::unique_lock<std::mutex> lock(lock_);
                                healthy_ = false;
                            }
                            return -1;
                        case ERROR:
                            delete httphandler;
                            {
                                std::unique_lock<std::mutex> lock(lock_);
                                healthy_ = false;
                            }
                            return -1;
                    }

                    string path = httphandler->getQuery();

                    FileManager::getInstance()->File_pushJob(
                        std::bind(getfile_, httphandler, path));
                }
            }
        });

        // 5. thread for interactive shell
        threads_.emplace_back([this]() -> int {
            prctl(PR_SET_NAME, "shell");

            SShd::getInstance(fds_[0]);

            {
                std::unique_lock<std::mutex> lock(lock_);
                healthy_ = false;
            }
            return -1;
        });
    }

    ~HttpServer() {
        close(fds_[1]);
        for (auto& thread : threads_) {
            thread.join();
        }
    }

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

public:
    bool                    healthy_;
    std::mutex              lock_;
    std::condition_variable iskill_conn;

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