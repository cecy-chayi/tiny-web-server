#include "log/log.h"
#include "pool/threadpool.h"
#include <features.h>

#if __GLIBC == 2 && __GLIBC_MINOR__ < 30
#include<sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

void testLog() {
    int cnt = 0, level = 0;
    Log::instance()->init(level, "./testlog1", ".log", 0);
    for(int level = 3; level >= 0; level--) {
        Log::instance()->setLevel(level);
        for(int j = 0; j < 10000; j++) {
            for(int i = 0; i < 4; i++) {
                LOG_BASE(i, "%s 111111111111111 %d ======================== ", "Test", cnt++);
            }
        }
    }
    cnt = 0;
    Log::instance()->init(level, "./testlog2", ".log", 5000);
    for(int level = 0; level < 4; level++) {
        Log::instance()->setLevel(level);
        for(int j = 0; j < 10000; j++) {
            for(int i = 0; i < 4; i++) {
                LOG_BASE(i, "%s 222222222222222 %d ======================== ", "Test", cnt++);
            }
        }
    }   
}

void threadLogTask(int i, int cnt) {
    for(int j = 0; j < 10000; j++) {
        LOG_BASE(i, "PID:[%04d]====== %05d =========== ", gettid(), cnt++);
    }
}

void testThreadPool() {
    Log::instance()->init(0, "./testLog3", ".log", 5000);
    ThreadPool threadPool(6);
    threadPool.init();
    for(int i = 0; i < 20; i++) {
        threadPool.submit(threadLogTask, i % 4, i * 100000);
    }
    threadPool.shutdown();
}

int main(int argc, char const *argv[])
{
    testLog();
    testThreadPool();
    exit(0);
}
