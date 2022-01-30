//
// Created by xiamingjie on 2022/1/26.
//

#include "connector.h"
#include "proto.h"
#include "logger.h"

#include <sys/socket.h>
#include <stdexcept>
#include <cstring>
#include <cerrno>

namespace client {

    Connector::Connector() : cltAddr(), srvAddr(), cltFd(-1), srvFd(-1), srvClosed(true) {
        cltBuffer = new char[BUFSIZ];
        if (!cltBuffer) {
            throw std::exception();
        }

        srvBuffer = new char[BUFSIZ];
        if (!srvBuffer) {
            throw std::exception();
        }

        cltReadIdx = cltWriteIdx = srvReadIdx = srvWriteIdx = 0;
    }

    void Connector::initClt(int cltFd_, const sockaddr_in &addr) {
        cltFd = cltFd_;
        cltAddr = addr;
    }

    void Connector::initSrv(int srvFd_, const sockaddr_in &addr) {
        srvFd = srvFd_;
        srvAddr = addr;

        srvClosed = false;
    }

    void Connector::reset() {
        srvFd = -1;
        cltReadIdx = cltWriteIdx = srvReadIdx = srvWriteIdx = 0;
        memset(cltBuffer, 0, BUFSIZ);
        memset(srvBuffer, 0, BUFSIZ);
        srvClosed = true;
    }

    RET_CODE Connector::readClt() {
        ssize_t bytesRead = 0;
        while (true) {
            if (cltReadIdx >= BUFSIZ)    //缓冲区已满
            {
                return RET_CODE::BUFFER_FULL;
            }

            bytesRead = recv(cltFd, cltBuffer + cltReadIdx, BUFSIZ - cltReadIdx, 0);
            if (bytesRead == -1) {
                if (errno == EAGAIN || errno ==
                                       EWOULDBLOCK)                    // 非阻塞情况下： EAGAIN表示没有数据可读，请尝试再次调用,而在阻塞情况下，如果被中断，则返回EINTR;  EWOULDBLOCK等同于EAGAIN
                {
                    break;
                }
                return RET_CODE::IO_ERR;
            } else if (bytesRead == 0)  //连接被关闭
            {
                return RET_CODE::CLOSED;
            }

            cltReadIdx += bytesRead;
        }
        return ((cltReadIdx - cltWriteIdx) > 0) ? RET_CODE::OK : RET_CODE::NOTHING;
    }

    RET_CODE Connector::readSrv() {
        ssize_t bytesRead = 0;
        while (true) {
            if (srvReadIdx >= BUFSIZ)    //缓冲区已满
            {
                return RET_CODE::BUFFER_FULL;
            }

            bytesRead = recv(srvFd, srvBuffer + srvReadIdx, BUFSIZ - srvReadIdx, 0);
            if (bytesRead == -1) {
                if (errno == EAGAIN || errno ==
                                       EWOULDBLOCK)                    // 非阻塞情况下： EAGAIN表示没有数据可读，请尝试再次调用,而在阻塞情况下，如果被中断，则返回EINTR;  EWOULDBLOCK等同于EAGAIN
                {
                    break;
                }
                return RET_CODE::IO_ERR;
            } else if (bytesRead == 0)  //连接被关闭
            {
                return RET_CODE::CLOSED;
            }

            srvReadIdx += bytesRead;
        }
        return ((srvReadIdx - srvWriteIdx) > 0) ? RET_CODE::OK : RET_CODE::NOTHING;
    }

    RET_CODE Connector::writeClt() {
        ssize_t bytes_write = 0;
        while (true) {
            if (srvReadIdx <= srvWriteIdx) {
                srvReadIdx = 0;
                srvWriteIdx = 0;
                return RET_CODE::BUFFER_EMPTY;
            }

            bytes_write = send(cltFd, srvBuffer + srvWriteIdx, srvReadIdx - srvWriteIdx, 0);
            if (bytes_write == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return RET_CODE::TRY_AGAIN;   //非阻塞 数据未写完
                }
                LOG_ERROR("write server socket failed, %s", strerror(errno));
                return RET_CODE::IO_ERR;
            } else if (bytes_write == 0) {
                return RET_CODE::CLOSED;
            }

            srvWriteIdx += bytes_write;
        }
    }

    RET_CODE Connector::writeSrv() {
        ssize_t bytes_write = 0;
        while (true) {
            if (cltReadIdx <= cltWriteIdx) {
                cltReadIdx = 0;
                cltWriteIdx = 0;
                return RET_CODE::BUFFER_EMPTY;
            }

            bytes_write = send(srvFd, cltBuffer + cltWriteIdx, cltReadIdx - cltWriteIdx, 0);
            if (bytes_write == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return RET_CODE::TRY_AGAIN;   //非阻塞 数据未写完
                }
                LOG_ERROR("write server socket failed, %s", strerror(errno));
                return RET_CODE::IO_ERR;
            } else if (bytes_write == 0) {
                return RET_CODE::CLOSED;
            }

            cltWriteIdx += bytes_write;
        }
    }
}