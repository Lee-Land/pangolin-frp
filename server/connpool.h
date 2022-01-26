//
// Created by xiamingjie on 2022/1/22.
//

#ifndef PANGOLIN_FRP_SERVER_CONNPOOL_H
#define PANGOLIN_FRP_SERVER_CONNPOOL_H

#include <map>
#include <arpa/inet.h>

class Connector;
class Epoller;

namespace server {

    enum class OP_TYPE {
        READ,
        WRITE
    };

    class ConnPool {
    public:
        explicit ConnPool(Epoller* epoller);
        /* 添加一个连接到连接池 */
        void addConn(int srvFd, const sockaddr_in& addr);
        /* 从连接池内选择一个连接 */
        Connector* pickConn(int cltFd);
        /* 释放连接放回连接池 */
        void freeConn(Connector* conn);
        /* 数据交换关键函数 */
        void process(int fd, OP_TYPE type);
    private:
        Epoller* epoller_;
        std::map<int, Connector*> conns_;
        std::map<int, Connector*> used_;
        std::map<int, Connector*> frees_;
    };
}

#endif