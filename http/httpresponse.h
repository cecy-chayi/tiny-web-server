#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h> // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void init(const std::string &srcDir, std::string &path, bool isKeepAlive = false, int code = -1);
    void makeResponse(Buffer &buff);
    void unmapFile();
    char* file();
    size_t fileLen() const;
    void errorContent(Buffer &buff, std::string message);
    int code() const {
        return code_;
    }

private:
    void addStateLine_(Buffer &buff);
    void addHeader_(Buffer &buff);
    void addContent_(Buffer &buff);

    void errorHtml_();
    std::string getFileType_();

private:
    int code_;
    bool isKeepAlive_;
    std::string path_;
    std::string srcDir_;
    char* mmFile_;
    struct stat mmFileStat_;
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};

#endif