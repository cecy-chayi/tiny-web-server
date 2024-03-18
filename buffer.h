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

    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t PrependableBytes() const;

    const char* Peek() const;
    void EnsureWritable(size_t);
    void HasWritten(size_t);

    void Retrieve(size_t);
    void RetrieveUntil(const char*);

    void  RetrieveAll();
    std::string RetrieveAllToStr();

    const char* BeginWriteConst() const;
    char* BeginWrite();

    void Append(const std::string&);
    void Append(const char*, size_t);
    void Append(const void*, size_t);
    void Append(const Buffer&);

    // void PrependInt64(int64_t);
    // void PrependInt32(int32_t);
    // void PrependInt16(int16_t);
    // void PrependInt8(int8_t);
    void Prepend(const void*, size_t);

    ssize_t ReadFd(int, int*);
    ssize_t WriteFd(int, int*);
private:
    char* BeginPtr_();
    const char* BeginPtr_() const;
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> readPos_; // read index
    std::atomic<std::size_t> writePos_; // write index

    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;
};

#endif