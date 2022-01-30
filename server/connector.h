//
// Created by xiamingjie on 2022/1/26.
//

#ifndef PANGOLIN_FRP_CONNECTOR_H
#define PANGOLIN_FRP_CONNECTOR_H

#include <cstddef>
#include <arpa/inet.h>

namespace server {
    enum class RET_CODE {
        OK,
        NOTHING,
        IO_ERR,
        CLOSED,
        BUFFER_FULL,
        BUFFER_EMPTY,
        TRY_AGAIN
    };

/**
 * 连接器,描述一对连接
 * */
    class Connector {
    public:
        Connector();
        ~Connector();

        void initClt(int fd, const sockaddr_in &addr);

        void initSrv(int fd, const sockaddr_in &addr);

        void reset();

        RET_CODE readClt();

        RET_CODE writeClt();

        RET_CODE readSrv();

        RET_CODE writeSrv();

    public:
        int cltFd;             //客户端 fd
        sockaddr_in cltAddr;   //客户端地址

        char* cltBuffer;      //客户端缓冲区
        size_t cltReadIdx;
        size_t cltWriteIdx;

        int srvFd;             //服务端 fd
        sockaddr_in srvAddr;   //服务端地址

        char* srvBuffer;       //服务端缓冲区
        size_t srvReadIdx;
        size_t srvWriteIdx;

        bool srvClosed;        //服务端已关闭
    };

}
#endif //PANGOLIN_FRP_CONNECTOR_H