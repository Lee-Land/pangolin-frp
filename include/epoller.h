//
// Created by xiamingjie on 2022/1/19.
//

#ifndef PANGOLIN_FRP_EPOLLER_H
#define PANGOLIN_FRP_EPOLLER_H

#include <sys/epoll.h>
#include <unordered_map>

using EventType = uint32_t;

class Epoller {

public:
    explicit Epoller(int maxEventNumber) noexcept(false);

    ~Epoller();

    void addReadFd(int fd, bool etAble) const;

    void removeFd(int fd) const;

    void modifyFd(int fd, EventType event) const;

    void closeFd(int fd) const;

    int wait(int timeout);

    EventType event(size_t i) const;

    int eventFd(size_t i) const;

private:
    int epollFd_;
    struct epoll_event* events_;
    int maxEventNumber_;
};

#endif //PANGOLIN_FRP_EPOLLER_H
