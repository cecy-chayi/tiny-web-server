#include "webserver.h"

WebServer::WebServer(
    int port, int trigMode, int timeoutMS, bool optLinger,
    int sqlPort, const char* sqlUser, const char* sqlPwd,
    const char* dbName, int connPoolNum, int threadNum,
    bool openLog, int logLevel, int logQueSize)
    : port_(port), openLinger_(optLinger), timeoutMS_(timeoutMS),
    isClose_(false), timer_(std::make_unique<HeapTimer>()),
        threadpool_(std::make_unique<ThreadPool>(threadNum)),
        epoller_(std::make_unique<Epoller>()) {

    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strcat(srcDir_, "/resources/");
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;

    // init sql connection pool
    SqlConnPool::instance()->init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);
    // init event and listen socket
    initEventMode_(trigMode);
    if(!initSocket_()) {
        isClose_ = true;
    }

    if(openLog) {
        Log::instance()->init(logLevel, "./log", ".log", logQueSize);
        if(isClose_) { 
            LOG_ERROR("========== Server init error! ==========");
        } else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port: %d, OpenLinger: %s", port_, optLinger ? "true" : "false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                                (listenEvent_ & EPOLLET) ? "ET" : "LT",
                                (connEvent_ & EPOLLET) ? "ET" : "LT");
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
        }
    }
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::instance()->closePool();
    threadpool_->shutdown();
}

void WebServer::initEventMode_(int trigMode) {
    listenEvent_ = EPOLLHUP; // socket close ?
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1-2:
        listenEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::start() {
    int timeMS = -1; // block
    if(!isClose_) {
        LOG_INFO("========== Server start ==========");
    }
    while(!isClose_) {
        if(timeoutMS_ > 0) {
            // get the next timeout waiting event
            // (at least before this time, no users will expire
            // and every time the timeout connection closed,
            // a new request needs to come in)
            timeMS = timer_->getNextTick();
        }
        int eventCnt = epoller_->wait(timeMS);
        for(int i = 0; i < eventCnt; i++) {
            int fd = epoller_->getEventFd(i);
            uint32_t events = epoller_->getEvents(i);
            if(fd == listenFd_) {
                dealListen_();
            } else if(events & (EPOLLRDHUP || EPOLLHUP || EPOLLERR)) {
                assert(users_.count(fd));
                closeConn_(&users_[fd]);
            } else if(events & EPOLLIN) {
                assert(users_.count(fd));
                dealRead_(&users_[fd]);
            } else if(events & EPOLLOUT) {
                assert(users_.count(fd));
                dealWrite_(&users_[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::sendError_(int fd, const char* info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::closeConn_(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->getFd());
    epoller_->delFd(client->getFd());
    client->close();
}

void WebServer::addClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if(timeoutMS_ > 0) {
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::closeConn_, this, &users_[fd]));
    }
    epoller_->addFd(fd, connEvent_ | EPOLLIN);
    setFdNonBlock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].getFd());
}

// process the listen event, put this into the heap_timer & epoller
void WebServer::dealListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr *) &addr, &len);
        if(fd <= 0) {
            return ;
        }
        else if(HttpConn::userCount >= MAX_FD) {
            sendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return ;
        }
        addClient_(fd, addr);
    } while(listenEvent_ & EPOLLET);
}

// process the read event, append the read task on threadpool
void WebServer::dealRead_(HttpConn* client) {
    assert(client);
    extendTime_(client);
    threadpool_->submit(std::bind(WebServer::onRead_, this, client));
}

// process the write event, append the write task on threadpool
void WebServer::dealWrite_(HttpConn* client) {
    assert(client);
    extendTime_(client);
    threadpool_->submit(std::bind(WebServer::onWrite_, this, client));
}

void WebServer::extendTime_(HttpConn* client) {
    assert(client);
    if(timeoutMS_ > 0) {
        timer_->adjust(client->getFd(), timeoutMS_);
    }
}

void WebServer::onRead_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    // read from httpconn read-buffer
    ret = client->read(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        closeConn_(client);
        return;
    }
    // business logic processing
    onProcess(client);
}

void WebServer::onProcess(HttpConn* client) {
    // 
    if(client->process()) {
        // modify the event after write
        epoller_->modFd(client->getFd(), connEvent_ | EPOLLOUT);
    } else {
        // // modify the event if read-buffer empty
        epoller_->modFd(client->getFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::onWrite_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->toWriteBytes() == 0) {
        if(client->isKeepAlive()) {
            // convert mode to read
            epoller_->modFd(client->getFd(), connEvent_ | EPOLLIN);
            return ;
        }
    } else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            // write-buffer is full
            epoller_->modFd(client->getFd(), connEvent_ | EPOLLOUT);
            return ;
        }
    }
    closeConn_(client);
}

bool WebServer::initSocket_() {
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port: %d error!", port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    {
        struct linger optLinger = {0};
        if(openLinger_) {
            // elegant close, send out or timeout
            optLinger.l_onoff = 1;
            optLinger.l_linger = 1;
        }

        listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
        if(listenFd_ < 0) {
            LOG_ERROR("Create socket[%d] error!", port_);
            return false;
        }

        ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
        if(ret < 0) {
            close(listenFd_);
            LOG_ERROR("Init socket[%d] error!", port_);
            return false;
        }
    }

    // SO_REUSEADDR: if the IP + PORT bound to the currently started
    // process conflicts with the IP + PORT occupied by the connection
    // in the TIME_WAIT state, but the newly started process uses the
    // SO_REUSEADDR option, then the process can be bound successfully
    int optval = 1;
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*) &optval, sizeof(int));
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error!");
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port: %d error!", port_);
        close(listenFd_);
        return false;
    }

    // accept queue size: min(6, somaxconn)
    ret = listen(listenFd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port: %d error!", port_);
        close(listenFd_);
        return false;
    }
    ret = epoller_->addFd(listenFd_, listenEvent_ | EPOLLIN);

    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }

    setFdNonBlock(listenFd_);
    LOG_INFO("Server port: %d", port_);
    return true;
}

int WebServer::setFdNonBlock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}
