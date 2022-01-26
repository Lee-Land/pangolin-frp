//
// Created by xiamingjie on 2022/1/22.
//

#ifndef PANGOLIN_FRP_CLIENT_CONNPOOL_H
#define PANGOLIN_FRP_CLIENT_CONNPOOL_H

#include <map>

class Connector;
class Epoller;
class Host;

namespace client {

    class ConnPool {
    public:
        explicit ConnPool(Epoller* epoller, const Host& host);
        /* 取得一条连接 */
        Connector* pickConn(int cltFd);
        /* 释放一个连接器 */
        void freeConn(Connector* conn);
        /* 回收连接到连接池 */
        void recycleConn();

    private:
        static const size_t CONN_NUMBER = 10;   //池内连接数量

        Epoller* epoller_;

        std::map<int, Connector*> conns_;
        std::map<int, Connector*> used_;
        std::map<int, Connector*> frees_;
    };

}

#endif //PANGOLIN_FRP_CLIENT_CONNPOOL_H
