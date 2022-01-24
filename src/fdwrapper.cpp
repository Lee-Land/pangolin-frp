//
// Created by xiamingjie on 2022/1/21.
//

#include "fdwrapper.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <csignal>
#include "proto.h"
#include "logger.h"

namespace fdwrapper {
    int setNonBlocking(int fd) {
        int old_op = fcntl(fd, F_GETFL);
        int new_op = old_op | O_NONBLOCK;
        fcntl(fd, F_SETFL, new_op);
        return old_op;
    }

    struct sockaddr_in getAddress(const Host &host) {
        struct sockaddr_in address = {};
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        inet_pton(AF_INET, host.hostName.c_str(), &address.sin_addr);
        address.sin_port = htons(host.port);

        return address;
    }

    struct sockaddr_in getLocalAddr(int fd, bool* ret) {
        struct sockaddr_in address = {};
        memset(&address, 0, sizeof(address));
        socklen_t len = sizeof(address);

        if (getsockname(fd, (sockaddr*)&address, &len) == -1) {
            *ret = false;
        } else {
            *ret = true;
        }

        return address;
    }

    struct sockaddr_in getRemoteAddr(int fd, bool* ret) {
        struct sockaddr_in address = {};
        memset(&address, 0, sizeof(address));
        socklen_t len = sizeof(address);

        if (getpeername(fd, (sockaddr*)&address, &len) == -1) {
            *ret = false;
        } else {
            *ret = true;
        }

        return address;
    }

    int listenTcp(const Host &localhost) {
        struct sockaddr_in address = getAddress(localhost);
        return listenTcp(address);
    }

    int listenTcp(struct sockaddr_in &address) {
        int sockFd = socket(PF_INET, SOCK_STREAM, 0);
        if (sockFd < 0) return -1;

        int val = 1;
        if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1) {
            LOG_ERROR("%s\n", strerror(errno));
            return -1;
        }

        if ((bind(sockFd, (struct sockaddr *) &address, sizeof(address)) == -1) || (listen(sockFd, 5) == -1)) {
            close(sockFd);
            return -1;
        }

        return sockFd;
    }

    int connTcp(const Host &server) {
        struct sockaddr_in address = getAddress(server);
        return connTcp(address);
    }

    int connTcp(struct sockaddr_in &address) {
        int sockFd = socket(PF_INET, SOCK_STREAM, 0);
        if (sockFd < 0) return -1;

        if (connect(sockFd, (struct sockaddr *) &address, sizeof(address)) == -1) {
            close(sockFd);
            return -1;
        }

        return sockFd;
    }

    void closeFd(int fd) {
        close(fd);
    }

    bool addSigHandler(int sig, void(handler)(int), bool restart) {
        struct sigaction sa = {};
        memset(&sa, '\0', sizeof(sa));
        sa.sa_handler = handler;
        if (restart) {   /* 重新调用被信号中断的系统调用 */
            sa.sa_flags |= SA_RESTART;
        }
        sigfillset(&sa.sa_mask);
        return sigaction(sig, &sa, nullptr) != -1;
    }
}