//
// Created by xiamingjie on 2022/1/19.
//

#include <iostream>
#include <unordered_map>
#include <cstring>
#include "epoller.h"
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include "fdwrapper.h"
#include "connector.h"
#include "proto.h"

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

void test_server(int port) {
    Epoller epoller(10);


    Host host;
    host.hostName = "127.0.0.1";
    host.port = port;
    int listenFd = fdwrapper::connTcp(host);


    epoller.addReadFd(listenFd, true);

    char buffer[1024];

    for (;;) {
        int number = epoller.wait(-1);

        if (number == -1) {
            break;
        }

        for (int i = 0; i < number; ++i) {
            int evFd = epoller.eventFd(i);
            uint32_t evType = epoller.event(i);

            if (evType & EPOLLIN) {
                memset(buffer, 0, sizeof(buffer));
                ssize_t len = recv(evFd, buffer, sizeof(buffer) - 1, 0);

                if (len == -1) {
                    printf("error : %s\n", strerror(errno));
                    epoller.closeFd(evFd);
                    continue;
                }

                if (len == 0) {
                    epoller.closeFd(evFd);
                    continue;
                }

                printf("receive: %s\n", buffer);

                epoller.modifyFd(evFd, EPOLLOUT);

            } else if (evType & EPOLLOUT) {
                ssize_t len = send(evFd, buffer, strlen(buffer), 0);
                printf("send: %s\n", buffer);
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