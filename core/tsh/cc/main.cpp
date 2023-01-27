#include <iostream>

#include "../../setting.h"
#include "../../lib/cc/CC.h"
#include "../../../Log/easylogging++.h"
INITIALIZE_EASYLOGGINGPP
#define ELPPP_THREAD_SAFE

int main() {

    // 日志配置
    el::Configurations conf{"Log/log.conf"};
    el::Loggers::reconfigureAllLoggers(conf);

    // 初始化CC
    CC cc(cc_ip, cc_port, file_port);
    LOG(INFO) << "CC Start";

    int id = -1;

    // 控制台
    while (1) {
        string cmd = cc.CC_get_line(&id);

        switch (cc.getCMDS_()[cmd]) {
            case 0:
                // printf help
                cc.CC_help_printf();
                break;

            case 1:
                // target list
                cc.CC_target_list();

                break;

            case 2:
                // target select
                cc.CC_target_select(id);
                id = -1;
                break;

            case 3:
                // Enter new line
                break;

            case 4:
                cc.CC_target_current();
                break;

            case 5:
                std::cout << "error input!\n\n";
                break;

            case 6:
                break;

            case 7:
                break;
        }
    }

    LOG(INFO) << "CC exit";
}