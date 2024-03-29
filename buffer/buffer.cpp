#include"buffer.h"

// input & output are relative to client code

Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(kCheapPrepend), writePos_(kCheapPrepend) {}

// output buffer length
size_t Buffer::writableBytes() const {
    return buffer_.size() - writePos_;
}

// input buffer length
size_t Buffer::readableBytes() const {
    return writePos_ - readPos_;
}

// prepend length
size_t Buffer::prependableBytes() const {
    return readPos_;
}

// the first char to read
const char* Buffer::peek() const {
    return &buffer_[readPos_];
}

// ensure the vector size for writing
void Buffer::ensureWritable(size_t len) {
    if(len > writableBytes()) {
        makeSpace_(len);
    }
    assert(len <= writableBytes());
}

// move the writePos_ after writing, use in append
void Buffer::hasWritten(size_t len) {
    writePos_ += len;
}

// move the readPos_ after reading
void Buffer::retrieve(size_t len) {
    readPos_ += len;
}

// move the readPos_ to the end
void Buffer::retrieveUntil(const char* end) {
    assert(peek() <= end);
    retrieve(end - peek());
}

// retrieve all data, then clear the buffer
void Buffer::retrieveAll() {
    bzero(beginPtr_(), buffer_.size());
    readPos_ = writePos_ = kCheapPrepend;
}

// retrieve input buffer and send string's type
std::string Buffer::retrieveAllToStr() {
    std::string str(peek(), readableBytes());
    retrieve(readableBytes());
    return str;
}

// const ptr for writePos_
const char* Buffer::beginWriteConst() const {
    return &buffer_[writePos_];
}

// ptr for writePos_
char* Buffer::beginWrite() {
    return &buffer_[writePos_];
}

// push str in output buffer
void Buffer::append(const char* str, size_t len) {
    assert(str);
    ensureWritable(len);
    std::copy(str, str + len, beginWrite());
    hasWritten(len);
}

void Buffer::append(const std::string &str) {
    append(str.c_str(), str.size());
}

void Buffer::append(const void* data, size_t len) {
    append(static_cast<const char*>(data), len);
}

// push another buf's input buffer in output buffer
void Buffer::append(const Buffer &buf) {
    append(buf.peek(), buf.readableBytes());
}

// push file in output buffer
ssize_t Buffer::readFd(int fd, int* Errno) {
    static constexpr int STACK_BUFFER_SIZE = 65535;
    char buff[STACK_BUFFER_SIZE]; // stack
    struct iovec iov[2];
    size_t writable = writableBytes();

    iov[0].iov_base = beginWrite();
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = STACK_BUFFER_SIZE;

    // scatter read, reduce the syscall count
    ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *Errno = errno;
    } else if(static_cast<size_t>(len) <= writable) {
        writePos_ += len;
    } else {
        writePos_ = buffer_.size();
        append(buff, static_cast<size_t>(len - writable));
    }
    return len;
}

// push input buffer in file
ssize_t Buffer::writeFd(int fd, int *Errno) {
    ssize_t len = write(fd, peek(), readableBytes());
    if(len < 0) {
        *Errno = errno;
    } else {
        retrieve(len);
    }
    return len;
}

char* Buffer::beginPtr_() {
    return &buffer_[0];
}

const char* Buffer::beginPtr_() const {
    return &buffer_[0];
}

void Buffer::makeSpace_(size_t len) {
    if(prependableBytes() + writableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    } else {
        size_t readable = readableBytes();
        std::copy(beginPtr_() + readPos_, beginPtr_() + writePos_, beginPtr_());
        readPos_ = kCheapPrepend;
        writePos_ = readable;
        assert(readable == readableBytes());
    }
}