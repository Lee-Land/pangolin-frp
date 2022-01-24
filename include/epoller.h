//
// Created by xiamingjie on 2022/1/19.
//

#ifndef PANGOLIN_FRP_EPOLLER_H
#define PANGOLIN_FRP_EPOLLER_H

#include <sys/epoll.h>
#include <unordered_map>

/*
 * Epoller 数据
 * */
struct EventData {
    int fd = -1;
    unsigned int length = 0;
    char *token = nullptr;
};

using EventDataSet = std::unordered_map<int, EventData *>;
using EventType = uint32_t;

const int MAX_EVENT_NUMBER = 10000;

class Epoller {

public:
    Epoller() noexcept(false);

    ~Epoller();

    void addFd(int fd, bool etAble);

    void addFd(int fd, bool etAble, EventData *eventData);

    void modifyFd(int fd, EventType event) const;

    void closeFd(int fd);

    int wait(int timeout);

    uint32_t event(size_t i) const;

    EventData* eventData(size_t i);

    int eventFd(size_t i) const;

private:
    void rmEventData_(int fd);

private:
    int epollFd_;
    EventDataSet eventDataSet_;
    struct epoll_event events_[MAX_EVENT_NUMBER];
};

#endif //PANGOLIN_FRP_EPOLLER_H
