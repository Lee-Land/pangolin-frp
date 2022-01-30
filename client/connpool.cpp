//
// Created by xiamingjie on 2022/1/22.
//

#include "connpool.h"
#include "logger.h"
#include "proto.h"
#include "connector.h"
#include "fdwrapper.h"
#include "epoller.h"

namespace client {

    ConnPool::ConnPool(Epoller *epoller, const std::pair<Host, Host> &mapping) : epoller_(epoller) {

        Host internal = mapping.first;
        Host external = mapping.second;

        for (int i = 0; i < CONN_NUMBER; ++i) {
            int inFd = fdwrapper::connTcp(internal);
            if (inFd == -1) {
                LOG_ERROR("connect to server[%s:%d] fail.", internal.hostName.c_str(), internal.port);
                continue;
            }

            int outFd = fdwrapper::connTcp(external);
            if (outFd == -1) {
                LOG_ERROR("connect to server[%s:%d] fail.", external.hostName.c_str(), external.port);
                continue;
            }

            Connector *conn;
            try {
                conn = new Connector;
            } catch (...) {
                fdwrapper::closeFd(inFd);
                fdwrapper::closeFd(outFd);
                LOG_ERROR("create Connector object fail.");
                continue;
            }

            epoller_->addReadFd(outFd, true);

            sockaddr_in outAddr = fdwrapper::getAddress(external);
            sockaddr_in inAddr = fdwrapper::getAddress(internal);
            conn->initClt(outFd, outAddr);
            conn->initSrv(inFd, inAddr);
            conns_.insert(std::pair<int, Connector *>(outFd, conn));
            LOG_DEBUG("Add one connection to connection pool.");
        }
    }

    bool ConnPool::exist(int fd) {
        if (used_.count(fd) > 0) {
            return true;
        } else {
            return false;
        }
    }

    Connector *ConnPool::pickConn(int cltFd) {
        if (used_.count(cltFd) > 0) {   //判断连接是否已被使用
            return used_[cltFd];
        }

        if (conns_.count(cltFd) <= 0) {   //未被使用也不存在于连接池中
            LOG_ERROR("Illegal connection sock %d.", cltFd);
            return nullptr;
        }

        return conns_[cltFd];
    }

    void ConnPool::freeConn(Connector *conn) {
        int srvFd = conn->srvFd;
        int cltFd = conn->cltFd;

        epoller_->closeFd(srvFd);
        epoller_->closeFd(cltFd);
        conn->reset();

        used_.erase(srvFd);
        used_.erase(cltFd);

        frees_.insert(std::pair<int, Connector *>(cltFd, conn));
    }

    void ConnPool::usingConn(Connector *conn) {
        if (conn == nullptr) return;

        int cltFd = conn->cltFd;
        int srvFd = conn->srvFd;

        epoller_->addReadFd(srvFd, true);

        conns_.erase(cltFd);
        used_.insert(std::pair<int, Connector *>(cltFd, conn));
        used_.insert(std::pair<int, Connector *>(srvFd, conn));

        LOG_INFO("Bind server sock %d with client sock %d.", srvFd, cltFd);
    }

    void ConnPool::recycleConn() {
        if (frees_.empty()) {
            return;
        }

        for (auto &iter: frees_) {     //将释放的连接重新建立并加入连接池
            int cltFd = iter.first;
            Connector *conn = iter.second;

            cltFd = fdwrapper::connTcp(conn->cltAddr);
            int srvFd = fdwrapper::connTcp(conn->srvAddr);
            if (cltFd == -1 || srvFd == -1) {
                LOG_ERROR("fix connection failed.");
            } else {
                LOG_INFO("fix connection success.");
                conn->initClt(cltFd, conn->cltAddr);
                conn->initSrv(srvFd, conn->srvAddr);
                epoller_->addReadFd(cltFd, true);
                conns_.insert(std::pair<int, Connector *>(cltFd, conn));
            }
        }

        frees_.clear();
    }

    void ConnPool::process(int fd, OP_TYPE type) {
        Connector *conn = used_[fd];

        if (!conn) {
            return;
        }

        if (conn->cltFd == fd) {    //客户端事件
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
                    RET_CODE res = conn->writeClt();
                    switch (res) {
                        case RET_CODE::TRY_AGAIN: {     //数据未写完，继续写
                            epoller_->modifyFd(fd, EPOLLOUT);
                            break;
                        }
                        case RET_CODE::BUFFER_EMPTY: {
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
                    RET_CODE res = conn->writeSrv();
                    switch (res) {
                        case RET_CODE::TRY_AGAIN: {
                            epoller_->modifyFd(fd, EPOLLOUT);
                            break;
                        }
                        case RET_CODE::BUFFER_EMPTY: {
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