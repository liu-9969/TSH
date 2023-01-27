#include "Ftpd.h"

Ftpd* Ftpd::ftpd_ = nullptr;

/**
 * @brief ftp start
 * @return 连接断开、ftp_stop命令会返回，否则一直loop
 */
int Ftpd::start()
{
    fd_set rd;
    int    n, ret, Errno;
    std::cout << "Ftpd::start(): 进入ftp-loop\n";
    while (1) {
        while (!httpHandler_->parseFinish()) {
            ret = httpHandler_->read(&Errno);

            // 断开连接
            if (ret == 0 || (ret < 0 && (Errno != EAGAIN))) {
                std::cout << "Ftpd::start(): Errno" << Errno << "\n";
                return -1;
            }
            if (ret < 0 && (Errno == EAGAIN)) {
                std::cout << "Ftpd::start(): Errno" << Errno << "\n";
                /* 正常不应该走到这里，因为select确实告诉我可读了，且不存在其他线程已经抢先读走的情况
                 */
            }

            httpHandler_->parseRequest();
        }
        string cmd = httpHandler_->getHeader("Ftp-Cmd");

        switch (ftpCmd_[cmd]) {
        case 1:  // ftp_stop
            return 0;

        case 2:  // ls
            break;

        case 3:  // get
            if (getFile() != FTP_SUCCESS)
                return 0;
            break;

        case 4:  // put
            break;

        case 5:
            break;
        }

        httpHandler_->resetParse();

        FD_ZERO(&rd);
        n = httpHandler_->getFd();
        FD_SET(n, &rd);

        if (select(n + 1, &rd, NULL, NULL, NULL) < 0) {
            return -1;
        }
    }
}

// 本地路径的检查
int Ftpd::_preFtp()
{
    path_ = httpHandler_->getQuery();

    // 校验本地路径合法性
    struct stat buffer;

    if (access(path_.c_str(), 0) != F_OK ||
        stat(path_.c_str(), &buffer) != 0 || (!S_ISREG(buffer.st_mode))) {
        std::cout << "\n[invalid local path]: Please check and try again\n";
        return FTP_ERROR;
    }
    size_ = buffer.st_size;
    return 0;
}

// bug记录：接收方一直没有收取，这边发送2.4M =
// 69632*后就阻塞掉了（阻塞socket）
// 文件稍微大点，readv会把我们64K(栈)+4K的BUffer读满，即69632

int Ftpd::getFile()
{
    int ret, Errno;

    if (_preFtp() != 0) {
        HttpResponse response(404);
        response.createResponse(&(httpHandler_->outBuff_));
        ret = httpHandler_->write(&Errno);
        return FTP_SUCCESS;
    }

    int fd = open(path_.c_str(), O_RDONLY);
    if (fd < 0) {
        HttpResponse response(404);
        response.createResponse(&(httpHandler_->outBuff_));
        ret = httpHandler_->write(&Errno);
        return FTP_SUCCESS;
    }

    int sum = 0;
    while (1) {
        Buffer _outBuff;
        ret = _outBuff.readFd(fd, &Errno);

        if (ret > 0) {
            HttpResponse response(200);
            response.addHeader("Ftp-Size", std::to_string(size_));
            response.setBody(_outBuff.peek(), _outBuff.readableBytes());
            response.createResponse(&(httpHandler_->outBuff_));
            int tem = _outBuff.readableBytes();
            _outBuff.retrieveAll();
            ret = httpHandler_->write(&Errno);
            sum += tem;
            std::cout << "[FTP_Server] 请求文件大小:" << size_
                      << "   目前总计发了:" << sum
                      << " 本次实际发送了(算http头):" << ret << std::endl;

            if (ret <= 0) {
                std::cout << "Ftpd::getFile():httpHandler_->write Error"
                          << Errno << "\n";
                return FTP_CLOSE;
            }
        }

        if (ret <= 0) {
            close(fd);
            break;
        }
    }

    return FTP_SUCCESS;
}