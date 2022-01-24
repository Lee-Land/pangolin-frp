//
// Created by xiamingjie on 2022/1/19.
//

#include "epoller.h"
#include "logger.h"
#include "fdwrapper.h"

#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <exception>

Epoller::Epoller() : events_() {
    epollFd_ = epoll_create(5);
    if (epollFd_ == -1) {
        throw std::runtime_error(strerror(errno));
    }
}

Epoller::~Epoller() {
    auto iter = eventDataSet_.begin();
    for (; iter != eventDataSet_.end();) {
        delete iter->second;
        eventDataSet_.erase(iter++);
    }
    close(epollFd_);
}

int Epoller::wait(int timeout) {
    return ::epoll_wait(epollFd_, events_, MAX_EVENT_NUMBER, timeout);
}

uint32_t Epoller::event(size_t i) const {
    return events_[i].events;
}

int Epoller::eventFd(size_t i) const {
    return events_[i].data.fd;
}

EventData* Epoller::eventData(size_t i) {
    return static_cast<EventData*>(events_[i].data.ptr);
}

void Epoller::addFd(int fd, bool etAble) {
    addFd(fd, etAble, nullptr);
}

void Epoller::addFd(int fd, bool etAble, EventData *eventData) {
    struct epoll_event event = {};
    if (eventData == nullptr) {
        event.data.fd = fd;
    } else {
        event.data.ptr = eventData;
    }
    event.events = EPOLLIN;
    if (etAble) {
        event.events |= EPOLLET;
    }
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) == -1) {
        LOG_ERROR("fd: %d operation of add failure.", fd);
        throw std::runtime_error(strerror(errno));
    }

    fdwrapper::setNonBlocking(fd);

    if (eventData != nullptr) {
        eventDataSet_[fd] = eventData;
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

void Epoller::closeFd(int fd) {
    if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        LOG_ERROR("fd: %d operation of del failure.", fd);
        throw std::runtime_error(strerror(errno));
    }

    close(fd);
    rmEventData_(fd);
}

void Epoller::rmEventData_(int fd) {
    if (eventDataSet_.count(fd) > 0) {
        auto ed = eventDataSet_[fd];
        delete ed->token;
        delete ed;
        eventDataSet_.erase(fd);
    }
}