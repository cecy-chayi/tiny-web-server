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
        len = writev(fd_, iov_, iovCnt_);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        // end of transfer
        if(iov_[0].iov_len + iov_[1].iov_len == 0) {
            break;
        }
    }while(isET); // edge triggered
}


