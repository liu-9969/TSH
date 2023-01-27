
// #include "../core/setting.h"
#include "/usr/local/src/easyloggingpp/src/easylogging++.h"
#define LOG_CONF "/usr/local/src/easyloggingpp/log.conf"

INITIALIZE_EASYLOGGINGPP

int main()
{
  el::Configurations conf{
    LOG_CONF};
  el::Loggers::
    reconfigureAllLoggers(conf);
  LOG(DEBUG) << "A debug message";
  LOG(INFO) << "An info message";
}