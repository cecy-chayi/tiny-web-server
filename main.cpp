#include <unistd.h>
#include "webserver.h"
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>

int init_daemon() {
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    int pid = fork();
    if(pid > 0) {
        // exit the parent process
        exit(0);
    } else if(pid < 0) {
        return -1;
    }
    // build a new process group
    setsid();
    pid = fork();
    if(pid > 0) {
        exit(0);
    } else if(pid < 0) {
        return -1;
    }
    for(int i = 0; i < NOFILE; close(i++));
    chdir("/");
    umask(0);
    signal(SIGCHLD, SIG_IGN);
    return 0;
}

int main() {
    init_daemon();
    WebServer server(
        1316, 3, 60000, false,
        3306, "chayi", "123456", "webserver",
        12, 6, true, 1, 1024
    );
    server.start();
}