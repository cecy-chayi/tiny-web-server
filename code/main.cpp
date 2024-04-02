#include <unistd.h>
#include "server/webserver.h"
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>

int main() {
    WebServer server(
        1316, 3, 60000, false,
        3306, "chayi", "123456", "webserver",
        12, 8, true, 1, 1024
    );
    server.start();
    exit(0);
}