#include "net/HttpRequest.h"
#include "net/HttpResponse.h"

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

/**
 * Ftp-Cmd
 *
 *
 */

#define FTP_ERROR -100
#define FTP_SUCCESS -99
#define FTP_CLOSE -98
#define FTP_STOP -94

class Ftpd {
public:
    // no return instance val
    static void getInstance(HttpRequest* httpHandler) {
        if (ftpd_ == nullptr) {
            ftpd_ = new Ftpd(httpHandler);
        }

        ftpd_->start();
    }

private:
    Ftpd(HttpRequest* httpHandler)
        : size_(0), path_(""), httpHandler_(httpHandler) {
        ftpCmd_ = {{"ftp_stop", 1},
                   {"ls", 2},
                   {"get", 3},
                   {"put", 4},
                   {"error", 5}};
    }
    int start();
    int getFile();
    int putFile();
    int _preFtp();

private:
    int                   size_;
    string                path_;
    std::map<string, int> ftpCmd_;
    HttpRequest*          httpHandler_;
    static Ftpd*          ftpd_;
};
