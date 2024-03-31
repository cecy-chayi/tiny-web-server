#include "httpconn.h"

const char* HttpConn::srcDir;
bool HttpConn::isET;
std::atomic<int> HttpConn::userCount;

HttpConn::HttpConn() {
    fd_ = -1;
    addr_ = { 0 };
    isclose_ = true;
}

HttpConn::~HttpConn() {
    close();
}

void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    userCount++;
    addr_ = addr;
    fd_ = fd;
    writeBuff_.retrieveAll();
    readBuff_.retrieveAll();
    isclose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount: %d", fd_, getIP(), getPort(), (int) userCount);
}

void HttpConn::close() {
    response_.unmapFile();
    if(isclose_ == false) {
        isclose_ = true;
        userCount--;
        ::close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, userCount: %d", fd_, getIP(), getPort(), (int) userCount);
    }
}

int HttpConn::getFd() const {
    return fd_;
}

struct sockaddr_in HttpConn::getAddr() const {
    return addr_;
}

const char* HttpConn::getIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::getPort() const {
    return addr_.sin_port;
}

ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        // scatter read
        len = readBuff_.readFd(fd_, saveErrno);
        if(len <= 0) {
            break;
        }
    }while(isET); // edge triggered, read all out at once
    return len;
}

// gather write
ssize_t HttpConn::write(int* saveErrno) {
    ssize_t len = -1;
    do {
        // write the iov data to fd, gather write
        len = writev(fd_, iov_, iovCnt_);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        // end of write
        if(iov_[0].iov_len + iov_[1].iov_len == 0) {
            break;
        } else if(static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) {
                writeBuff_.retrieveAll();
                iov_[0].iov_len = 0;
            }
        } else {
            iov_[0].iov_base = (uint8_t*) iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuff_.retrieve(len);
        }
    } while (isET || toWriteBytes() > 10240);
    return len; 
}

bool HttpConn::process() {
    request_.init();
    if(readBuff_.readableBytes() <= 0) {
        return false;
    } else if(request_.parse(readBuff_)) {
        // parse success
        LOG_DEBUG("%s", request_.path().c_str());
        response_.init(srcDir, request_.path(), request_.isKeepAlive(), 200);
    } else {
        response_.init(srcDir, request_.path(), request_.isKeepAlive(), 400);
    }
    response_.makeResponse(writeBuff_);
    // response header
    iov_[0].iov_base = const_cast<char*>(writeBuff_.peek());
    iov_[0].iov_len = writeBuff_.readableBytes();
    iovCnt_ = 1;

    // response file
    if(response_.fileLen() > 0 && response_.file()) {
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.fileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d to %d", response_.fileLen(), iovCnt_, toWriteBytes());
    return true;
}

