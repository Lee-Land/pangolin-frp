//
// Created by xiamingjie on 2022/1/19.
//

#include "epoller.h"
#include "logger.h"
#include "fdwrapper.h"

#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>

Epoller::Epoller(int maxEventNumber) : maxEventNumber_(maxEventNumber) {
    epollFd_ = epoll_create(5);
    if (epollFd_ == -1) {
        throw std::runtime_error(strerror(errno));
    }

    events_ = new epoll_event[maxEventNumber];
}

Epoller::~Epoller() {
    delete [] events_;
    close(epollFd_);
}

int Epoller::wait(int timeout) {
    return ::epoll_wait(epollFd_, events_, maxEventNumber_, timeout);
}

EventType Epoller::event(size_t i) const {
    return events_[i].events;
}

int Epoller::eventFd(size_t i) const {
    return events_[i].data.fd;
}

void Epoller::addReadFd(int fd, bool etAble) const {
    struct epoll_event event = {};
    event.data.fd = fd;
    event.events = EPOLLIN;
    if (etAble) {
        event.events |= EPOLLET;
    }
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) == -1) {
        LOG_ERROR("fd: %d operation of add failure.", fd);
        throw std::runtime_error(strerror(errno));
    }

    fdwrapper::setNonBlocking(fd);
}

void Epoller::removeFd(int fd) const {
    if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        LOG_ERROR("fd: %d operation of del failure.", fd);
        throw std::runtime_error(strerror(errno));
    }
}

void Epoller::modifyFd(int fd, EventType eventType) const {
    struct epoll_event event = {};
    event.data.fd = fd;
    event.events = eventType | EPOLLET;
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) == -1) {
        LOG_ERROR("fd: %d operation of mod failure.", fd);
        throw std::runtime_error(strerror(errno));
    }
}

void Epoller::closeFd(int fd) const {
    removeFd(fd);
    close(fd);
}