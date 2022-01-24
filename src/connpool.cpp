//
// Created by xiamingjie on 2022/1/22.
//

#include "connpool.h"
#include "fdwrapper.h"
#include "epoller.h"
#include "logger.h"

namespace pangolin {
    Connector::Connector(RunType type_) : cltFd(-1), srvFd(-1), cltAddr(), srvAddr(), type(type_) {}

    void Connector::initClt(int fd, struct sockaddr_in &addr) {
        cltAddr = addr;
        cltFd = fd;
    }

    void Connector::initSrv(int fd, struct sockaddr_in &addr) {
        srvAddr = addr;
        srvFd = fd;
    }

    RET_CODE Connector::readClt() {
        ssize_t len = -1;
        int saveErrno;
        while (true) {
            len = cltBuffer.readFd(cltFd, &saveErrno);
            LOG_DEBUG("read %ld bytes data.", len);
            if (len == -1) {
                if (saveErrno == EAGAIN || saveErrno == EWOULDBLOCK) {
                    break;
                }
                LOG_ERROR("client reading error: %s", strerror(errno));
                return RET_CODE::IO_ERR;
            } else if (len == 0) {
                LOG_INFO("client closed.");
                return RET_CODE::CLOSED;
            }
        }
        return cltBuffer.readableBytes() > 0 ? RET_CODE::OK : RET_CODE::NOTHING;
    }

    RET_CODE Connector::writeClt() {
        int saveErrno;
        while (true) {
            ssize_t len = srvBuffer.writeFd(cltFd, &saveErrno);
            if (len < 0) {
                if (saveErrno == EAGAIN || saveErrno == EWOULDBLOCK) {
                    return RET_CODE::TRY_AGAIN;
                }
                LOG_ERROR("client writing error: %s", strerror(errno));
                return RET_CODE::IO_ERR;
            } else if (len == 0) {
                LOG_INFO("client closed.");
                return RET_CODE::CLOSED;
            }

            if (srvBuffer.readableBytes() <= 0) {
                return RET_CODE::BUFFER_EMPTY;
            }
        }
    }

    RET_CODE Connector::readSrv() {
        ssize_t len;
        int saveErrno;
        while (true) {
            len = srvBuffer.readFd(srvFd, &saveErrno);
            if (len == -1) {
                if (saveErrno == EAGAIN || saveErrno == EWOULDBLOCK) {
                    break;
                }
                LOG_ERROR("server reading error: %s", strerror(errno));
                return RET_CODE::IO_ERR;
            } else if (len == 0) {
                LOG_INFO("server closed.");
                return RET_CODE::CLOSED;
            }
        }
        return srvBuffer.readableBytes() > 0 ? RET_CODE::OK : RET_CODE::NOTHING;
    }

    RET_CODE Connector::writeSrv() {
        ssize_t len;
        int saveErrno;
        while (true) {

            len = cltBuffer.writeFd(srvFd, &saveErrno);
            if (len < 0) {
                if (saveErrno == EAGAIN || saveErrno == EWOULDBLOCK) {
                    return RET_CODE::TRY_AGAIN;
                }
                LOG_ERROR("server writing error: %s", strerror(errno));
                return RET_CODE::IO_ERR;
            } else if (len == 0) {
                LOG_INFO("server closed.");
                return RET_CODE::CLOSED;
            }

            if (cltBuffer.readableBytes() <= 0) {
                return RET_CODE::BUFFER_EMPTY;
            }
        }
    }

    void Connector::reset() {
        cltBuffer.clear();
        srvBuffer.clear();

        if (type == RunType::SERVER) {
            cltFd = -1;
        } else if (type == RunType::CLIENT) {
            srvFd = -1;
        } else {}
    }

    ConnPool::ConnPool(Epoller *epoller, RunType type) : epoller_(epoller), type_(type) {}

    int ConnPool::conn2srv(const Host &host) {
        if (type_ != RunType::CLIENT) {
            LOG_ERROR("This function should be called by the client.");
            return -1;
        }

        int connFd = fdwrapper::connTcp(host);  //连接服务器
        if (connFd == -1) {
            LOG_ERROR("connection server for %s:%d fail.", host.hostName.c_str(), host.port);
            return -1;
        }

        return connFd;
    }

    void ConnPool::establish(const Host &host) {
        if (type_ != RunType::CLIENT) {
            return;
        }

        for (int i = 0; i < CONN_NUMBER; ++i) {
            int connFd = conn2srv(host);
            if (connFd < 0) {
                continue;
            }

            Connector *conn;
            try {
                conn = new Connector(type_);
            } catch (...) {
                LOG_ERROR("new connector %d fail.", i);
                fdwrapper::closeFd(connFd);
                continue;
            }

            struct sockaddr_in address = fdwrapper::getAddress(host);
            conn->initClt(connFd, address);
            epoller_->addFd(connFd, true);

            conns_.insert(std::pair<int, Connector *>(connFd, conn));
            LOG_DEBUG("build connection fd: %d to server [%s:%d]", connFd, host.hostName.c_str(), host.port);
        }
    }

    void ConnPool::srv4conn(int fd, struct sockaddr_in &addr) {
        if (type_ != RunType::SERVER) return;

        Connector *conn;
        try {
            conn = new Connector(type_);
        } catch (...) {
            LOG_ERROR("new connector fail.");
            fdwrapper::closeFd(fd);
            return;
        }

        conn->initSrv(fd, addr);
        conns_.insert(std::pair<int, Connector *>(fd, conn));
    }

    Connector *ConnPool::pickConn4Server(int cltFd) {
        if (type_ != RunType::SERVER) return nullptr;

        if (conns_.empty()) {
            LOG_ERROR("not enough srv connections to server.");
            return nullptr;
        }

        auto iter = conns_.begin();
        int srvFd = iter->first;
        auto conn = iter->second;
        if (conn == nullptr) {
            LOG_ERROR("empty server connection object.");
            return nullptr;
        }

        conns_.erase(iter);
        used_.insert(std::pair<int, Connector *>(cltFd, conn));
        used_.insert(std::pair<int, Connector *>(srvFd, conn));

        epoller_->addFd(cltFd, true);
        epoller_->addFd(srvFd, true);
        LOG_INFO("bind client sock %d with server sock %d", cltFd, srvFd);
        return conn;
    }

    void ConnPool::pickConn4Client(int cltFd, const Host &internalHost) {
        if (type_ != RunType::CLIENT) return;

        if (conns_.empty() || conns_.count(cltFd) == 0) {
            LOG_ERROR("not cltFd:%d connections of client.", cltFd);
            return;
        }

        //已分配连接池中不存在到内网服务器的连接，新建一个
        if (used_.count(cltFd) == 0) {
            int srvFd = conn2srv(internalHost);
            if (srvFd == -1) {
                return;
            }

            auto conn = conns_[cltFd];

            struct sockaddr_in srvAddr = fdwrapper::getAddress(internalHost);
            conn->initSrv(srvFd, srvAddr);

            conns_.erase(cltFd);
            used_.insert(std::pair<int, Connector *>(cltFd, conn));
            used_.insert(std::pair<int, Connector *>(srvFd, conn));

            epoller_->addFd(srvFd, true);
            LOG_INFO("bind client sock %d with server sock %d", cltFd, srvFd);
        }
    }

    void ConnPool::freeConn(Connector *conn) {
        int cltFd = conn->cltFd;
        int srvFd = conn->srvFd;
        epoller_->closeFd(cltFd);
        epoller_->closeFd(srvFd);

        used_.erase(cltFd);
        used_.erase(srvFd);

        conn->reset();
        freed_.insert(std::pair<int, Connector *>(srvFd, conn));
    }

    void ConnPool::recycleConns() {
        if (freed_.empty()) {
            return;
        }

        for (auto &iter: freed_) {
            int cltFd = iter.first;
            Connector *conn = iter.second;

            cltFd = fdwrapper::connTcp(conn->cltAddr);
            if (cltFd < 0) {
                LOG_ERROR("fix connection failed.");
            } else {
                LOG_INFO("fix connection success.");
                conn->initClt(cltFd, conn->cltAddr);
                conns_.insert(std::pair<int, Connector *>(cltFd, conn));
            }
        }
        freed_.clear();
    }

    void ConnPool::processServer(int fd, OP_TYPE type) {
        Connector *conn = used_[fd];

        if (conn == nullptr) {
            LOG_ERROR("not bind mapping of connection fd: %d.", fd);
            return;
        }

        if (conn->cltFd == fd) {    //客户端事件
            int srvFd = conn->srvFd;
            switch (type) {
                case OP_TYPE::READ: {   //客户端发消息到监听端
                    RET_CODE ret = conn->readClt();
                    LOG_DEBUG("ret: %d", ret);
                    switch (ret) {
                        case RET_CODE::OK: {
                            LOG_DEBUG("receive %ld bytes data from client", conn->cltBuffer.readableBytes());
                            LOG_DEBUG("mod fd: %d is a EPOLLOUT.", srvFd);
                            epoller_->modifyFd(srvFd, EPOLLOUT);    //激活服务端写事件
                            break;
                        }
                        case RET_CODE::IO_ERR:
                        case RET_CODE::CLOSED: {
                            freeConn(conn);   //释放连接
                            return;
                        }
                        default:
                            break;
                    }
                    break;
                }
                case OP_TYPE::WRITE: {  //发送消息到客户端
                    size_t len = conn->srvBuffer.readableBytes();
                    RET_CODE ret = conn->writeClt();
                    switch (ret) {
                        case RET_CODE::OK:
                        case RET_CODE::BUFFER_EMPTY: {
                            LOG_DEBUG("send %ld bytes data to client.", len);
                            epoller_->modifyFd(srvFd, EPOLLIN);
                            epoller_->modifyFd(fd, EPOLLIN);
                            break;
                        }
                        case RET_CODE::TRY_AGAIN: { //数据未发送完毕，继续写
                            epoller_->modifyFd(fd, EPOLLOUT);
                            break;
                        }
                        case RET_CODE::IO_ERR:
                        case RET_CODE::CLOSED: {
                            LOG_DEBUG("close client connection.");
                            freeConn(conn);
                            return;
                        }
                        default:
                            break;
                    }
                    break;
                }
                default: {
                    LOG_ERROR("other operation not support yet.");
                    break;
                }
            }
        } else if (conn->srvFd == fd) { //服务端事件
            int cltFd = conn->cltFd;
            switch (type) {
                case OP_TYPE::READ: {
                    RET_CODE ret = conn->readSrv();
                    switch (ret) {
                        case RET_CODE::OK: {
                            LOG_DEBUG("receive %ld bytes data from server", conn->srvBuffer.readableBytes());
                            epoller_->modifyFd(cltFd, EPOLLOUT);
                            break;
                        }
                        case RET_CODE::IO_ERR:
                        case RET_CODE::CLOSED: {
                            epoller_->modifyFd(cltFd, EPOLLOUT);
                            break;

                        }
                        default:
                            break;
                    }
                    break;
                }
                case OP_TYPE::WRITE: {
                    size_t len = conn->cltBuffer.readableBytes();
                    RET_CODE ret = conn->writeSrv();
                    switch (ret) {
                        case RET_CODE::BUFFER_EMPTY: {
                            LOG_DEBUG("send %ld bytes data to server.", len);
                            epoller_->modifyFd(cltFd, EPOLLIN);
                            epoller_->modifyFd(fd, EPOLLIN);
                            break;
                        }
                        case RET_CODE::TRY_AGAIN: {
                            epoller_->modifyFd(fd, EPOLLOUT);
                            break;
                        }
                        case RET_CODE::IO_ERR:
                        case RET_CODE::CLOSED: {
                            epoller_->modifyFd(cltFd, EPOLLOUT);
                            break;
                        }
                        default:
                            break;
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

    /**
     * 在客户端程序中，外网服务器程序称为客户端连接，内网程序称为服务端连接
     * */
    void ConnPool::processClient(int fd, OP_TYPE type) {
        Connector *conn = used_[fd];

        if (conn == nullptr) {
            LOG_ERROR("not bind mapping of connection fd: %d.", fd);
            return;
        }

        if (conn->cltFd == fd) {    //客户端事件
            int srvFd = conn->srvFd;

            switch (type) {
                case OP_TYPE::READ: {   //外网客户端发消息来
                    RET_CODE ret = conn->readClt();
                    LOG_DEBUG("ret: %d", ret);
                    switch (ret) {
                        case RET_CODE::OK: {
                            LOG_DEBUG("receive %ld bytes data from client.", conn->cltBuffer.readableBytes());
                            LOG_DEBUG("mod fd: %d is a EPOLLOUT.", srvFd);
                            epoller_->modifyFd(srvFd, EPOLLOUT);
                            break;
                        }
                        case RET_CODE::IO_ERR:
                        case RET_CODE::CLOSED: {
                            LOG_DEBUG("close client connection.");
                            freeConn(conn);
                            return;
                        }
                        default: break;
                    }
                    break;
                }
                case OP_TYPE::WRITE: {  //应答消息给客户端
                    size_t len = conn->srvBuffer.readableBytes();
                    RET_CODE ret = conn->writeClt();
                    switch (ret) {
                        case RET_CODE::OK:
                        case RET_CODE::BUFFER_EMPTY: {
                            LOG_DEBUG("send %ld bytes data to client.", len);
                            epoller_->modifyFd(srvFd, EPOLLIN);
                            epoller_->modifyFd(fd, EPOLLIN);
                            break;
                        }
                        case RET_CODE::TRY_AGAIN: { //数据未发送完毕，继续写入
                            epoller_->modifyFd(fd, EPOLLOUT);
                            break;
                        }
                        case RET_CODE::IO_ERR:
                        case RET_CODE::CLOSED: {
                            freeConn(conn);
                            return;
                        }
                        default: break;
                    }
                    break;
                }
                default: {
                    LOG_DEBUG("other operation not support yet.");
                    break;
                }
            }
        } else if (conn->srvFd == fd) { //服务端事件
            int cltFd = conn->cltFd;
            switch (type) {
                case OP_TYPE::READ: {
                    RET_CODE ret = conn->readSrv();
                    switch (ret) {
                        case RET_CODE::OK: {
                            LOG_DEBUG("receive %ld bytes data from server.", conn->srvBuffer.readableBytes());
                            epoller_->modifyFd(cltFd, EPOLLOUT);
                            break;
                        }
                        case RET_CODE::IO_ERR:
                        case RET_CODE::CLOSED: {
                            epoller_->modifyFd(cltFd, EPOLLOUT);
                            break;
                        }
                        default: break;
                    }
                    break;
                }
                case OP_TYPE::WRITE: {
                    size_t len = conn->cltBuffer.readableBytes();
                    RET_CODE ret = conn->writeSrv();
                    switch (ret) {
                        case RET_CODE::BUFFER_EMPTY: {
                            LOG_DEBUG("send %ld bytes data to server.", len);
                            epoller_->modifyFd(cltFd, EPOLLIN);
                            epoller_->modifyFd(fd, EPOLLIN);
                            break;
                        }
                        case  RET_CODE::TRY_AGAIN: {
                            epoller_->modifyFd(fd, EPOLLOUT);
                            break;
                        }
                        case RET_CODE::IO_ERR:
                        case RET_CODE::CLOSED: {
                            epoller_->modifyFd(cltFd, EPOLLOUT);
                            break;
                        }
                        default: break;
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