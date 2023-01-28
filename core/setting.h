#ifndef _SETTING_H_
#define _SETTING_H_

#define cc_port 7788
#define file_port 6677
#define cc_ip "192.168.75.138"

// note：agent run as a daemon without output-log
// unnote: agent run as debug with output-log
// so, add debug log by same with old code, otherwise your log will dup2 to CC  
#define AGENT_BACK "true"

#define LOG_ROOT "/usr/local/src/easyloggingpp/src/easylogging++.h"

#endif