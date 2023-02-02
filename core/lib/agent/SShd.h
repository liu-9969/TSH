#ifndef _SSHD_
#define _SSHD_
#include <iostream>
#include <vector>
#include <memory>
#include <unistd.h>
#include "net/HttpRequest.h"
#include "net/Buffer.h"
#include <queue>
#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <sys/prctl.h>

using fileDownloadCallBack = std::function<int(HttpRequest*, string)>;
using fileUploadCallBack   = std::function<int(HttpRequest*, string)>;
using File_JobFunction = std::function<void()>;


/**
 * @brief a rshell based tty for linux
 * @design: single and nothing return, all things all resourse do inside the
 *          class, just expose this interfase
 */
class SShd {
public:
    static void getInstance(std::vector<int> fds) {
        if (sshd_ == nullptr) {
            sshd_ = new SShd(fds);
        }

        switch (sshd_->runshell()) {
            // agent will restart
            case CLOSE: 
                delete sshd_;
            case ERROR: 
                delete sshd_;
        }
    }

    ~SShd() {
        delete httpHandler_;
        close(fd_);
        close(ffd_);
    }

private:
    SShd(std::vector<int>& fds)
        : fd_(fds[0]), ffd_(fds[1]), httpHandler_(new HttpRequest(fds[0])) {
        init();
    }

    // init the tty
    void init();

    // for access the loop agagin
    void reset();

    // core shell-loop
    int runshell();

    // event defined by self
    int _events_pty_readable();
    int _events_pty_writable();
    int _events_file();

    // call back for file
    fileDownloadCallBack getfile_;
    fileUploadCallBack   putfile_;
    int                  getFile(HttpRequest* httpHandler, string path);
    int                  putFile(HttpRequest* httpHandler, string path);

    // some call p-private
    int _setNonBlock(int fd);
    int _events_CC_readable(HttpRequest* httphandler);

    int    fd_;
    int    ffd_;
    int    tty_;  // slave side
    int    pty_;  // master side
    string cmd_;
    bool   filter_;
    bool   login_filter_;

    HttpRequest* httpHandler_;
    static SShd* sshd_;
    Buffer       _outBuff;

    enum ReturnNum {
        SUCCESS = -100,
        ERROR,
        CLOSE,
    };
};



class FileManager {
public:
    static FileManager* getInstance(int num = 2, std::vector<std::thread> * const threads = nullptr ) {
        if (fileManager_ == nullptr) {
            fileManager_ = new FileManager(num, threads);
        }
        return fileManager_;
    }

    void File_pushJob(const File_JobFunction& job);
    FileManager(int num, std::vector<std::thread> *const threads);
    ~FileManager();

    // std::vector<std::thread> threads_;

private:
    std::queue<File_JobFunction> Job_queues_;
    std::mutex                   File_lock_;
    std::condition_variable      File_conn_;
    bool                         stop_;

    static FileManager *fileManager_;
};

#endif