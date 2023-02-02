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
        : healthy_(true), ccIp_(ccIp), ports_({shellPort, filePort}) {
        // 1. connect with CC by two tcp-connection
        _connec();

        // 2. init threadpoll for file tance
        FileManager::getInstance(2, &threads_);

        // 3. run the agent server
        threads_.emplace_back([this](){
            prctl(PR_SET_NAME, "shell");

            SShd::getInstance(fds_);
            {
                // httpServer will restart
                std::unique_lock<std::mutex> lock(lock_);
                healthy_ = false;
            }
        });
    }

    ~HttpServer() {
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


public:
    bool                    healthy_ = true;
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


public:
   

    enum ReturnNum {
        SUCCESS = -100,
        ERROR,
        CLOSE,
    };
};
#endif