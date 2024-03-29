#include "epoller.h"

Epoller::Epoller(int maxEvent) : epollFd_(epoll_create(512)), events(maxEvent) {
    assert(epollFd_ >= 0 && events.size() > 0);
}

Epoller::~Epoller() {
    close(epollFd_);
}

bool Epoller::addFd(int fd, uint32_t events) {
    if(fd < 0) {
        return false;
    }
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::modFd(int fd, uint32_t events) {
    if(fd < 0) {
        return false;
    }
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::delFd(int fd) {
    if(fd < 0) {
        return false;
    }
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, 0);
}

int Epoller::wait(int timeoutMs) {
    return epoll_wait(epollFd_, &events[0], static_cast<int>(events.size()), timeoutMs);
}

int Epoller::getEventFd(size_t i) const {
    assert(0 <= i && i < events.size());
    return events[i].data.fd;
}

uint32_t Epoller::getEvents(size_t i) const {
    assert(0 <= i && i < events.size());
    return events[i].events;
}
