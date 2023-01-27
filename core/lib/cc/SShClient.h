#ifndef _SSHCLIENT_
#define _SSHCLIENT_

#include <iostream>
#include <string>
#include <vector>

#include "FileManager.h"
#include "net/HttpResponse.h"
#include "net/HttpRequest.h"

/**
 * @brief SShd-client for linux
 */
extern std::map<string, string> URL_;
using fileDownloadCallBack =
    std::function<int(std::shared_ptr<HttpResponse>, std::vector<string>)>;
using fileUploadCallBack =
    std::function<int(std::shared_ptr<HttpResponse>, std::vector<string>)>;

// 为了在shell里引用CC里的全部cmd-func
class CC;

class SShClient {
public:
    int start();
    // clang-format off
    SShClient(CC* cc, int id, std::shared_ptr<HttpResponse> httpHandler)
        : again_(false),
          cc_(cc),
          id_(id),
          httpHandler_(httpHandler),
          onFileDownload_(std::bind(&SShClient::getFile, this,
                                    std::placeholders::_1,
                                    std::placeholders::_2)) {
        filterCmds_ = {
                        {"help",      1},
                        {"ls_target", 2},
                        {"target",    3},
                        {"cur_target",4},
                        {"get",       5},
                        {"put",       6},
                        {"stop",      7}
                      };
    }
    // clang-format on
    ~SShClient() {}

private:
    void init();
    void reset();
    // shell-loop
    int runshell();

    // transfer shell-data back and forth
    int _events_terminal_readable();
    int _events_terminal_writable();

    // transfer file
    int _preFtp(std::vector<string>& splitCmd);

    int getFile(std::shared_ptr<HttpResponse> httpHandler_,
                std::vector<string>           splitCmd_);

    int putFile(std::shared_ptr<HttpResponse> httpHandler_,
                std::vector<string>           splitCmd_);

    void _progress_bar(const char* file_name, float sum, float file_size);

    std::vector<string> _split(const string& str, char pattern);

    string SSH_get_line(int* id);

    bool again_;
    CC*  cc_;
    int  id_;

    std::shared_ptr<HttpResponse> httpHandler_;
    fileDownloadCallBack          onFileDownload_;
    fileUploadCallBack            onFileUpload_;
    std::vector<string>           splitCmd_;
    std::map<string, int>         filterCmds_;

    enum ReturnNum { SUCCESS = -100, ERROR, CLOSE, STOP };
};
#endif