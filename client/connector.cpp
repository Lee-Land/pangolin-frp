//
// Created by xiamingjie on 2022/1/26.
//

#include "connector.h"

#include <sys/socket.h>
#include <stdexcept>
#include <cstring>
#include <cerrno>

namespace client {

    Connector::Connector() : cltAddr(), srvAddr(), cltFd(-1), srvFd(-1), srvClosed(true) {
        cltBuffer = new char[BUFFER_SIZE];
        if (!cltBuffer) {
            throw std::runtime_error("create client buffer failed.");
        }

        srvBuffer = new char[BUFFER_SIZE];
        if (!srvBuffer) {
            throw std::runtime_error("create server buffer failed.");
        }
        cltReadIdx = cltWriteIdx = srvReadIdx = srvWriteIdx = 0;
    }

    Connector::~Connector() {
        delete[] cltBuffer;
        delete[] srvBuffer;
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
        cltFd = -1;
        cltReadIdx = cltWriteIdx = srvReadIdx = srvWriteIdx = 0;
        memset(cltBuffer, 0, BUFFER_SIZE);
        memset(srvBuffer, 0, BUFFER_SIZE);
    }

    RET_CODE Connector::readClt() {
        ssize_t bytesRead;

        while (true) {
            if (cltReadIdx >= BUFFER_SIZE) {
                return RET_CODE::BUFFER_FULL;
            }

            bytesRead = recv(cltFd, cltBuffer + cltReadIdx, BUFFER_SIZE - cltReadIdx, 0);

            if (bytesRead == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                return RET_CODE::IO_ERR;
            } else if (bytesRead == 0) {
                return RET_CODE::CLOSED;
            }

            cltReadIdx += bytesRead;
        }

        return (cltReadIdx - cltWriteIdx) > 0 ? RET_CODE::OK : RET_CODE::NOTHING;
    }

    RET_CODE Connector::readSrv() {
        ssize_t bytesRead;

        while (true) {
            if (srvReadIdx >= BUFFER_SIZE) {
                return RET_CODE::BUFFER_FULL;
            }

            bytesRead = recv(srvFd, srvBuffer + srvReadIdx, BUFFER_SIZE - srvReadIdx, 0);

            if (bytesRead == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                return RET_CODE::IO_ERR;
            } else if (bytesRead == 0) {
                return RET_CODE::CLOSED;
            }

            srvReadIdx += bytesRead;
        }

        return (srvReadIdx - srvWriteIdx) > 0 ? RET_CODE::OK : RET_CODE::NOTHING;
    }

    RET_CODE Connector::writeClt() {
        ssize_t bytesWrite;
        while (true) {
            if (srvWriteIdx >= srvReadIdx) {
                srvWriteIdx = srvReadIdx = 0;
                return RET_CODE::BUFFER_EMPTY;
            }

            bytesWrite = send(cltFd, srvBuffer + srvWriteIdx, BUFFER_SIZE - srvWriteIdx, 0);

            if (bytesWrite == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return RET_CODE::TRY_AGAIN;
                }
                return RET_CODE::IO_ERR;
            } else if (bytesWrite == 0) {
                return RET_CODE::CLOSED;
            }

            srvWriteIdx += bytesWrite;
        }
    }

    RET_CODE Connector::writeSrv() {
        ssize_t bytesWrite;
        while (true) {
            if (cltWriteIdx >= cltReadIdx) {
                cltWriteIdx = cltReadIdx = 0;
                return RET_CODE::BUFFER_EMPTY;
            }

            bytesWrite = send(srvFd, cltBuffer + cltWriteIdx, BUFFER_SIZE - cltWriteIdx, 0);

            if (bytesWrite == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return RET_CODE::TRY_AGAIN;
                }
                return RET_CODE::IO_ERR;
            } else if (bytesWrite == 0) {
                return RET_CODE::CLOSED;
            }

            cltWriteIdx += bytesWrite;
        }
    }
}