#ifndef LOG_H
#define LOG_H

#include<mutex>
#include<string>
#include<thread>
#include<sys/time.h>
#include<string.h>
#include<stdarg.h>
#include<assert.h>
#include<sys/stat.h>
#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log {
public:
    // init instance(block queue capacity, saved path, saved suffix)
    void init(int level, const char* path = "./log",
                const char *suffix = ".log",
                int maxQueueCapacity = 1024);

    static Log* instance();
    static void flushLogThread(); // aio write log interface

    void write(int level, const char *format, ...); // output in format
    void flush();

    int getLevel();
    void setLevel(int level);
    bool isopen() {return isOpen_; }
private:
    Log();
    void appendLogLevelTitle_(int level);
    virtual ~Log();
    void asyncWrite_(); // aio write specific function
private:
    static const int LOG_PATH_LEN = 256; // max log path length
    static const int LOG_NAME_LEN = 256; // max log name length
    static const int MAX_LINES = 50000; // max log lines' count

    const char *path_; 
    const char *suffix_;

    int MAX_LINES_;

    int lineCount_; 
    int toDay_; // sort files by date

    bool isOpen_;

    Buffer buff_;
    int level_; // log level
    bool isAsync_; // enable asynchronous logging

    FILE *fp_; // log file ptr
    std::unique_ptr<BlockQueue<std::string>> deque_;
    std::unique_ptr<std::thread> writeThread_;
    std::mutex mtx_;
    
};

#define LOG_BASE(level, format, ...) \
    do { \
        Log *log = Log::instance(); \
        if(log->isopen() && log->getLevel() <= level) { \
            log->write(level, format, ##__VA_ARGS__); \
            log->flush(); \
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)}while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)}while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)}while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)}while(0);

#endif