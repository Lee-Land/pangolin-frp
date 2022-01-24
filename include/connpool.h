//
// Created by xiamingjie on 2022/1/22.
//

#ifndef PANGOLIN_FRP_CONNPOOL_H
#define PANGOLIN_FRP_CONNPOOL_H

#include <arpa/inet.h>
#include <vector>
#include <unordered_map>
#include <map>

#include "buffer.h"
#include "proto.h"

class Epoller;  //前置声明

namespace pangolin {
    /**
     * 连接器
     * */
    class Connector {
    public:
    public:
        explicit Connector(RunType type_);
        ~Connector() = default;

        void initClt(int fd, struct sockaddr_in& addr);
        void initSrv(int fd, struct sockaddr_in& addr);

        /**
         * 客户端与服务端交换数据关键函数
         * */
        RET_CODE readClt();
        RET_CODE writeClt();
        RET_CODE readSrv();
        RET_CODE writeSrv();

        /* 重置连接 */
        void reset();

    public:
        int cltFd;                      //客户端 fd
        struct sockaddr_in cltAddr;     //客户端地址
        Buffer cltBuffer;               //客户端缓冲区

        int srvFd;                      //服务端 fd
        struct sockaddr_in srvAddr;     //服务端地址
        Buffer srvBuffer;               //服务端缓冲区

        RunType type;                   //运行类型 server/client
    };

    enum class OP_TYPE {
        READ,
        WRITE
    };

    using IntToConn = std::map<int, Connector*>;

    /**
     * 连接池
     * */
     class ConnPool {
     public:
         explicit ConnPool(Epoller* epoller, RunType type);
         ~ConnPool() = default;

         /* 在客户端下运行，和服务器建立连接并加入连接池 */
         int conn2srv(const Host& host);
         void establish(const Host& host);  //由客户端发起，建立和服务端的连接池
         /* 在服务端运行，接收客户端连接并加入连接池 */
         void srv4conn(int fd, struct sockaddr_in& addr);

         /* 从池里选择一个服务端连接 */
         Connector* pickConn4Server(int fd);
         /* 从池里选择一个客户端连接 */
         void pickConn4Client(int fd, const Host& internalHost);

         /* 释放连接 */
         void freeConn(Connector* conn);

         /* 回收连接,将释放后的连接重新放入连接池 */
         void recycleConns();

         /* 服务端数据转发 */
         void processServer(int fd, OP_TYPE type);
         /* 客户端数据转发 */
         void processClient(int fd, OP_TYPE type);

     private:
         const size_t CONN_NUMBER = 8;   //连接池默认连接数

         Epoller* epoller_;                     //Epoll 对象

         IntToConn conns_;                      //待分配连接池
         IntToConn used_;                       //已分配连接池
         IntToConn freed_;                      //已释放连接池

         RunType type_;                         //连接池运行模式 server/client
     };
}


#endif //PANGOLIN_FRP_CONNPOOL_H
