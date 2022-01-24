//
// Created by xiamingjie on 2022/1/21.
//

#ifndef PANGOLIN_FRP_FDWRAPPER_H
#define PANGOLIN_FRP_FDWRAPPER_H

#include <arpa/inet.h>

class Host;

namespace fdwrapper {
    /**
     * 设置 fd 为非阻塞, 并保留原属性
     * */
    int setNonBlocking(int fd);

    /**
     * 从 Host 中获取地址
     * */
    struct sockaddr_in getAddress(const Host &host);

    /**
     * 获取本端地址
     * */
    struct sockaddr_in getLocalAddr(int fd, bool *ret);

    /**
     * 获取对端地址
     * */
    struct sockaddr_in getRemoteAddr(int fd, bool *ret);

    /**
     * 创建一个 Tcp socket 并监听
     * */
    int listenTcp(const Host &localhost);

    int listenTcp(struct sockaddr_in &address);

    /**
     * 创建一个 Tcp 连接
     * */
    int connTcp(const Host &server);

    int connTcp(struct sockaddr_in &address);

    void closeFd(int fd);

    /**
     * 添加信号处理函数
     * */
    bool addSigHandler(int sig, void(*handler)(int), bool restart = true);
};


#endif //PANGOLIN_FRP_FDWRAPPER_H
