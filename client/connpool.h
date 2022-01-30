//
// Created by xiamingjie on 2022/1/22.
//

#ifndef PANGOLIN_FRP_CLIENT_CONNPOOL_H
#define PANGOLIN_FRP_CLIENT_CONNPOOL_H

#include <map>
#include <vector>

class Epoller;
class Host;

namespace client {

    enum class OP_TYPE {
        READ,
        WRITE
    };

    class Connector;

    class ConnPool {
    public:
        explicit ConnPool(Epoller* epoller, const std::pair<Host, Host> &mapping);
        /* 取得一条连接 */
        Connector* pickConn(int cltFd);
        bool exist(int fd);
        /* 释放一个连接器 */
        void freeConn(Connector* conn);
        /* 使用一条连接 */
        void usingConn(Connector* conn);
        /* 回收连接到连接池 */
        void recycleConn();
        /* 与客户端进行数据交换 */
        void process(int fd, OP_TYPE type);

    private:
        static const size_t CONN_NUMBER = 10;   //池内连接数量

        Epoller* epoller_;

        std::map<int, Connector*> conns_;
        std::map<int, Connector*> used_;
        std::map<int, Connector*> frees_;
    };

}

#endif //PANGOLIN_FRP_CLIENT_CONNPOOL_H
