#include "log.h"

Log::Log() {
    fp_ = nullptr;
    deque_ = nullptr;
    writeThread_ = nullptr;
    lineCount_ = 0;
    toDay_ = 0;
    isAsync_ = false;
}

Log::~Log() {
    while(!deque_->empty()) {
        deque_->flush();
    }
    deque_->close();
    writeThread_->join();
    if(fp_) {
        std::lock_guard<std::mutex> lock(mtx_);
        flush();
        fclose(fp_);
    }
}

void Log::flush() {
    // only asynchronous needs block_deque
    if(isAsync_) {
        deque_->flush();
    }
    fflush(fp_); // clear file input buffer
}

Log* Log::Instance() {
    static Log log;
    return &log;
}

void Log::AsyncWrite_() {
    std::string str;
    while(deque_->pop(str)) {
        std::lock_guard<std::mutex> lock(mtx_);
        fputs(str.c_str(), fp_);
    }
}

void Log::init(int level, const char* path, const char* suffix, int maxQueCapacity) {
    isOpen_ = true;
    level_ = level;
    path_ = path;
    suffix_ = suffix;
    if(maxQueCapacity) { // non-zero means asynchronous
        isAsync_ = true;
        if(!deque_) {
            deque_ = std::make_unique<BlockQueue<std::string>>(BlockQueue<std::string>(maxQueCapacity));
            writeThread_ = std::make_unique<std::thread>(std::thread(FlushLogThread));
        }
    } else {
        isAsync_ = false;
    }
    lineCount_ = 0;
    time_t timer = time(nullptr); // current time
    struct tm t;
    localtime_r(&timer, &t);
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    toDay_ = t.tm_mday;

    {
        std::lock_guard<std::mutex> lock(mtx_);
        buff_.RetrieveAll(); // clear the buffer
        
        // reopen the file
        if(fp_) {
            flush();
            fclose(fp_);
        }
        fp_ = fopen(fileName, "a"); // open file with rw mode and create it if not-found
        if(fp_ == nullptr) { // if failed
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a"); // create dir
        }
        assert(fp_ != nullptr);
    }
}

void Log::write(int level, const char *format, ...) {
    struct timeval now{0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm t;
    localtime_r(&tSec, &t);
    va_list vaList;

    if(toDay_ != t.tm_mday || (lineCount_ && lineCount_ % MAX_LINES_ == 0)) {
        char newFile[LOG_NAME_LEN];
        char tail[36]{0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if(toDay_ != t.tm_mday) {
            snprintf(newFile, LOG_NAME_LEN, "%s/%s%s", path_, tail, suffix_);
        } else {
            snprintf(newFile, LOG_NAME_LEN, "%s/%s-%d%s", path_, tail, (lineCount_ / MAX_LINES_), suffix_);
        }

        {
            std::lock_guard<std::mutex> lock(mtx_);
            flush();
            fclose(fp_);
            fp_ = fopen(newFile, "a");
            assert(fp_ != nullptr);
        }
    }
    
    // generate the corresponding log in buffer
    {
        std::lock_guard<std::mutex> lock(mtx_);
        ++lineCount_;
        int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d:%02d.%06ld ",
                t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, 
                t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        
        buff_.HasWritten(n);
        AppendLogLevelTitle_(level);

        va_start(vaList, format);
        int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
        va_end(vaList);

        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);
        // asynchronous function(send log to block_queue)
        if(isAsync_ && deque_ && !deque_->full()) {
            deque_->push_back(buff_.RetrieveAllToStr());
        } else {
            // synchronous
            fputs(buff_.Peek(), fp_);
        }
        buff_.RetrieveAll();
    }
}

static char* logLevelName[] = {
    "[debug]: ",
    "[info] : ",
    "[WARN] : ",
    "[ERROR]: "
};

void Log::AppendLogLevelTitle_(int level) {
    buff_.Append(logLevelName[level], 9);
}

int Log::Getlevel() {
    return level_;
}

void Log::setLevel(int level) {
    std::lock_guard<std::mutex> lock(mtx_);
    level_ = level;
}