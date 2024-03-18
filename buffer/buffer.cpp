#include"buffer.h"

// input & output are relative to client code

Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(kCheapPrepend), writePos_(kCheapPrepend) {}

// output buffer length
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

// input buffer length
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

// prepend length
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

// the first char to read
const char* Buffer::Peek() const {
    return &buffer_[readPos_];
}

// ensure the vector size for writing
void Buffer::EnsureWritable(size_t len) {
    if(len > WritableBytes()) {
        MakeSpace_(len);
    }
    assert(len <= WritableBytes());
}

// move the writePos_ after writing
void Buffer::HasWritten(size_t len) {
    writePos_ += len;
}

// move the readPos_ after reading
void Buffer::Retrieve(size_t len) {
    readPos_ += len;
}

// move the readPos_ to the end
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

// retrieve all data, then clear the buffer
void Buffer::RetrieveAll() {
    bzero(BeginPtr_(), buffer_.size());
    readPos_ = writePos_ = kCheapPrepend;
}

// retrieve input buffer and send string's type
std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    Retrieve(ReadableBytes());
    return str;
}

// const ptr for writePos_
const char* Buffer::BeginWriteConst() const {
    return &buffer_[writePos_];
}

// ptr for writePos_
char* Buffer::BeginWrite() {
    return &buffer_[writePos_];
}

// push str in output buffer
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWritable(len);
    std::copy(str, str + len, BeginWrite());
}

void Buffer::Append(const std::string &str) {
    Append(str.c_str(), str.size());
}

void Buffer::Append(const void* data, size_t len) {
    Append(static_cast<const char*>(data), len);
}

// push another buf's input buffer in output buffer
void Buffer::Append(const Buffer &buf) {
    Append(buf.Peek(), buf.ReadableBytes());
}

// push file in output buffer
ssize_t Buffer::ReadFd(int fd, int* Errno) {
    static constexpr int STACK_BUFFER_SIZE = 65535;
    char buff[STACK_BUFFER_SIZE]; // stack
    struct iovec iov[2];
    size_t writable = WritableBytes();

    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = STACK_BUFFER_SIZE;

    ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *Errno = errno;
    } else if(static_cast<size_t>(len) <= writable) {
        HasWritten(len);
    } else {
        writePos_ = buffer_.size();
        Append(buff, static_cast<size_t>(len - writable));
    }
    return len;
}

// push input buffer in file
ssize_t Buffer::WriteFd(int fd, int *Errno) {
    ssize_t len = write(fd, Peek(), ReadableBytes());
    if(len < 0) {
        *Errno = errno;
    } else {
        Retrieve(len);
    }
    return len;
}

void Buffer::Prepend(const void* data, size_t len) {
    assert(len <= PrependableBytes());
    readPos_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, BeginPtr_() + readPos_);
}

char* Buffer::BeginPtr_() {
    return &buffer_[0];
}

const char* Buffer::BeginPtr_() const {
    return &buffer_[0];
}

void Buffer::MakeSpace_(size_t len) {
    if(PrependableBytes() + WritableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    }
    size_t readable = ReadableBytes();
    std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
    readPos_ = kCheapPrepend;
    writePos_ = readable;
    assert(readable == ReadableBytes());
}