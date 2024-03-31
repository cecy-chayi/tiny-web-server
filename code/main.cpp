#include <unistd.h>
#include "webserver.h"
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>

int main() {
    WebServer server(
        1316, 3, 60000, false,
        3306, "chayi", "123456", "webserver",
        12, 6, true, 0, 1024
    );
    server.start();
}