//
// Created by xiamingjie on 2022/1/19.
//

#include <iostream>
#include <unordered_map>
#include <cstring>
#include "epoller.h"
#include "buffer.h"
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <proto.h>
#include "fdwrapper.h"

using namespace std;

enum class TestEnum {
    FIRST = 0,
    SECOND,
    THIRD
};

int setNonBlocking(int fd) {
    int old_op = fcntl(fd, F_GETFL);
    int new_op = old_op | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_op);
    return old_op;
}

void test() {
    unordered_map<int, EventData *> eventSet;

    for (int i = 0; i < 5; ++i) {
        auto eventData = new EventData;
        eventData->fd = i;
        eventData->length = 5;
        eventData->token = new char[eventData->length];
        memset(eventData, '\0', eventData->length);
        strcpy(eventData->token, "abcd");

        eventSet[i] = eventData;
    }

    auto eventData = eventSet[3];
    char *evToken = eventData->token;
    strcpy(evToken, "xmja");

    for (auto &event: eventSet) {
        char *token = event.second->token;
        cout << event.first << " -> " << token << endl;
        delete token;

        delete event.second;
    }
}

void test2() {
    unordered_map<int, int> testMap;

    for (int i = 0; i < 10; ++i) {
        testMap[i] = i;
    }

    auto iter = testMap.begin();
    for (; iter != testMap.end();) {
        cout << iter->second << endl;
        testMap.erase(iter++);
    }

    cout << "size: ";
    cout << testMap.size() << endl;
}

void printBuffer(const Buffer &buffer, RET_CODE ret) {
    switch (ret) {
        case RET_CODE::NOTHING: {
            printf("没有数据或数据不完整\n");
            break;
        }
        case RET_CODE::OK: {
            printf("%s\n", buffer.peek());
            break;
        }
        case RET_CODE::IO_ERR: {
            printf("IO错误, %s\n", strerror(errno));
            break;
        }
        case RET_CODE::CLOSED: {
            printf("FD关闭\n");
            break;
        }
        default: {
            break;
        }
    }
}

void test3() {
//    Buffer buffer(10);
//
//    setNonBlocking(STDIN_FILENO);
//
//    Epoller epoller;
//    epoller.addFd(STDIN_FILENO, true);
//
//    for (;;) {
//        int number = epoller.wait(-1);
//
//        if (number == -1) {
//            printf("epoll call failure. %s\n", strerror(errno));
//            break;
//        }
//
//        for (int i = 0; i < number; ++i) {
//            int fd = epoller.eventFd(i);
//            uint32_t eventType = epoller.event(i);
//
//            if (fd == STDIN_FILENO && eventType == EPOLLIN) {
//                RET_CODE ret = buffer.readFd(STDIN_FILENO);
//                printBuffer(buffer, ret);
//            }
//        }
//    }

//    RET_CODE ret = buffer.readFd(STDIN_FILENO, 5);
//    printBuffer(buffer, ret);
//    ret = buffer.readFd(STDIN_FILENO, 5);
//    printBuffer(buffer, ret);
}

void test_server(int port) {
    Epoller epoller;

    Buffer buffer;

    int listenFd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        printf("socket call fail.%s\n", strerror(errno));
        return;
    }

//    Host host;
//    host.hostName = "119.91.195.167"; Listen 119.91.195.167
//    host.port = port;
    struct sockaddr_in host = {};
    bzero(&host, sizeof(host));
    host.sin_family = AF_INET;
    inet_pton(AF_INET, "f119.91.195.168", &host.sin_addr);
    host.sin_port = htons(port);
//    int listenFd = fdwrapper::listenTcp(host);

    int on = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        close(listenFd);
        printf("set reuseaddr fail.%s\n", strerror(errno));
        return;
    }

    if (bind(listenFd, (struct sockaddr*)&host, sizeof (host)) == -1) {
        close(listenFd);
        printf("bind call fail.%s\n", strerror(errno));
        return;
    }

    if (listen(listenFd, 5) == -1) {
        close(listenFd);
        printf("listen call fail.%s\n", strerror(errno));
        return;
    }

    epoller.addFd(listenFd, true);

    for (;;) {
        int number = epoller.wait(-1);

        if (number == -1) {
            break;
        }

        for (int i = 0; i < number; ++i) {
            int evFd = epoller.eventFd(i);
            uint32_t evType = epoller.event(i);

            if (evFd == listenFd && (evType & EPOLLIN)) {

                struct sockaddr_in peerAddr = {};
                memset(&peerAddr, 0, sizeof(peerAddr));
                socklen_t peerLen = sizeof(peerAddr);

                int connFd = accept(evFd, (struct sockaddr *) &peerAddr, &peerLen);
                if (connFd <= 0) continue;

                epoller.addFd(connFd, true);

            } else if (evType & EPOLLIN) {
                int save;
                ssize_t len = buffer.readFd(evFd, &save);
                if (len == 0) {
                    printf("len=0, %s\n", strerror(save));
                    continue;
                }
                if (len == -1) {
                    printf("len=-1, %s\n", strerror(save));
                    continue;
                }
                printf("receive %ld bytes data: %s\n", len, buffer.toString().c_str());

                epoller.modifyFd(evFd, EPOLLOUT);
            } else if (evType & EPOLLOUT) {

                const char* response = "ok.";
                send(evFd, response, strlen(response), 0);

                epoller.modifyFd(evFd, EPOLLIN);
            }
        }
    }
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        printf("please input mode and port\n");
        return 1;
    }

    int port = std::stoi(argv[1]);

    test_server(port);


    return 0;
}