#ifndef BUFFER_H
#define BUFFER_H

#include<cstring>
#include<iostream>
#include<unistd.h>
#include<sys/uio.h>
#include<vector>
#include<atomic>
#include<assert.h>
#include<sys/socket.h>

class Buffer {
public:
    Buffer(int initBuffSize = kInitialSize);
    ~Buffer() = default;

    size_t writableBytes() const;
    size_t readableBytes() const;
    size_t prependableBytes() const;

    const char* peek() const;
    void ensureWritable(size_t);
    void hasWritten(size_t);

    void retrieve(size_t);
    void retrieveUntil(const char*);

    void  retrieveAll();
    std::string retrieveAllToStr();

    const char* beginWriteConst() const;
    char* beginWrite();

    void append(const std::string&);
    void append(const char*, size_t);
    void append(const void*, size_t);
    void append(const Buffer&);

    ssize_t readFd(int, int*);
    ssize_t writeFd(int, int*);
private:
    char* beginPtr_();
    const char* beginPtr_() const;
    void makeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> readPos_; // read index
    std::atomic<std::size_t> writePos_; // write index
    static const size_t kInitialSize = 1024;
};

#endif