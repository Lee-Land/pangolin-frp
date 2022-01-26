//
// Created by xiamingjie on 2022/1/22.
//
#include "connpool.h"
#include "connector.h"
#include "fdwrapper.h"
#include "logger.h"
#include "epoller.h"

namespace server {
    ConnPool::ConnPool(Epoller *epoller) : epoller_(epoller) {

    }

    void ConnPool::addConn(int srvFd, const sockaddr_in& addr) {
        Connector *conn = nullptr;

        while (conn == nullptr) {
            if (frees_.empty()) {
                try {
                    conn = new Connector;
                } catch (...) {
                    LOG_ERROR("Create Connector object fail in connPool addConn.");
                    fdwrapper::closeFd(srvFd);
                    return;
                }
            } else {
                auto iter = frees_.begin();
                conn = iter->second;
                frees_.erase(iter);

                if (conn == nullptr) {
                    continue;
                }
            }
        }

        conn->initSrv(srvFd, addr);
        conns_.insert(std::pair<int, Connector*>(srvFd, conn));

        LOG_DEBUG("Add one connection to connection pool.");
    }

    server::Connector *ConnPool::pickConn(int cltFd) {
        if (conns_.empty()) {
            LOG_ERROR("There are no free connections in the connection pool.");
            return nullptr;
        }

        auto iter = conns_.begin();
        int srvFd = iter->first;
        auto conn = iter->second;

        if (!conn) {
            LOG_ERROR("Empty server connection object.");
            return nullptr;
        }

        conns_.erase(iter);
        used_.insert(std::pair<int, Connector*>(srvFd, conn));
        used_.insert(std::pair<int, Connector*>(cltFd, conn));
        epoller_->addReadFd(srvFd, true);
        epoller_->addReadFd(cltFd, true);
        LOG_INFO("Bind client sock %d with server sock %d.", cltFd, srvFd);
        return conn;
    }

    void ConnPool::freeConn(Connector *conn) {
        int srvFd = conn->srvFd;
        int cltFd = conn->cltFd;

        epoller_->closeFd(srvFd);
        epoller_->closeFd(cltFd);

        used_.erase(srvFd);
        used_.erase(cltFd);
        conn->reset();

        frees_.insert(std::pair<int, Connector*>(srvFd, conn));
    }

    void ConnPool::process(int fd, OP_TYPE type) {
        Connector* conn = used_[fd];

        if (!conn) {
            return;
        }

        if (conn->cltFd == fd) {
            int srvFd = conn->srvFd;
            switch (type) {
                case OP_TYPE::READ: {
                    RET_CODE res = conn->readClt();
                    switch (res) {
                        case RET_CODE::OK: {
                            LOG_INFO("receive %ld bytes data from client.", conn->cltReadIdx);
                        }
                        case RET_CODE::BUFFER_FULL: {
                            epoller_->modifyFd(srvFd, EPOLLOUT);
                            break;
                        }
                        case RET_CODE::IO_ERR: {
                            LOG_ERROR("read client sock %d error:%s", fd, strerror(errno));
                        }
                        case RET_CODE::CLOSED: {
                            LOG_INFO("closed client sock %d", fd);
                            freeConn(conn);
                            return;
                        }
                        default: {
                            break;
                        }
                    }
                    if (conn->srvClosed) {
                        freeConn(conn);
                        return;
                    }
                    break;
                }
                case OP_TYPE::WRITE: {
                    size_t len = conn->srvWriteIdx;
                    RET_CODE res = conn->writeClt();
                    switch (res) {
                        case RET_CODE::TRY_AGAIN: {     //数据未写完，继续写
                            epoller_->modifyFd(fd, EPOLLOUT);
                            break;
                        }
                        case RET_CODE::BUFFER_EMPTY: {
                            LOG_INFO("send %ld data to server.", len);
                            epoller_->modifyFd(srvFd, EPOLLIN);
                            epoller_->modifyFd(fd, EPOLLIN);
                            break;
                        }
                        case RET_CODE::IO_ERR: {
                            LOG_ERROR("write client sock %d error:%s", fd, strerror(errno));
                        }
                        case RET_CODE::CLOSED: {
                            freeConn(conn);
                            return;
                        }
                        default: {
                            break;
                        }
                    }
                    if (conn->srvClosed) {
                        freeConn(conn);
                        return;
                    }
                    break;
                }
                default: {
                    LOG_ERROR("other operation not support yet.");
                    break;
                }
            }
        } else if (conn->srvFd == fd) {     //服务端事件
            int cltFd = conn->cltFd;
            switch (type) {
                case OP_TYPE::READ: {
                    RET_CODE res = conn->readSrv();
                    switch (res) {
                        case RET_CODE::OK: {
                            LOG_INFO("receive %ld bytes data from server.", conn->srvReadIdx);
                        }
                        case RET_CODE::BUFFER_FULL: {
                            epoller_->modifyFd(cltFd, EPOLLOUT);
                            break;
                        }
                        case RET_CODE::IO_ERR: {
                            LOG_ERROR("read server sock %d error:%s", fd, strerror(errno));
                        }
                        case RET_CODE::CLOSED: {
                            epoller_->modifyFd(cltFd, EPOLLOUT);
                            conn->srvClosed = true;
                            LOG_INFO("closed server sock %d.", fd);
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    break;
                }
                case OP_TYPE::WRITE: {
                    size_t len = conn->cltWriteIdx;
                    RET_CODE res = conn->writeSrv();
                    switch (res) {
                        case RET_CODE::TRY_AGAIN: {
                            epoller_->modifyFd(fd, EPOLLOUT);
                            break;
                        }
                        case RET_CODE::BUFFER_EMPTY: {
                            LOG_INFO("send %ld data to client.", len);
                            epoller_->modifyFd(cltFd, EPOLLIN);
                            epoller_->modifyFd(fd, EPOLLIN);
                            break;
                        }
                        case RET_CODE::IO_ERR: {
                            LOG_ERROR("write server sock %d error:%s", fd, strerror(errno));
                        }
                        case RET_CODE::CLOSED: {
                            epoller_->modifyFd(cltFd, EPOLLOUT);
                            conn->srvClosed = true;
                            LOG_INFO("closed server sock %d.", fd);
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    break;
                }
                default: {
                    LOG_ERROR("other operation not support yet.");
                    break;
                }
            }
        } else {
            return;
        }
    }
}