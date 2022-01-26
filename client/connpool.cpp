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

    ConnPool::ConnPool(Epoller *epoller, const Host &host) : epoller_(epoller) {
        for (int i = 0; i < CONN_NUMBER; ++i) {
            int connFd = fdwrapper::connTcp(host);

            if (connFd == -1) {
                LOG_ERROR("connect to server[%s:%d] fail.", host.hostName.c_str(), host.port);
                continue;
            }
            Connector* conn;
            try {
                conn = new Connector;
            } catch (...) {
                fdwrapper::closeFd(connFd);
                LOG_ERROR("create Connector object fail.");
                continue;
            }

            epoller_->addReadFd(connFd, true);

            sockaddr_in addr = fdwrapper::getAddress(host);
            conn->initClt(connFd, addr);
            conns_.insert(std::pair<int, Connector*>(connFd, conn));
            LOG_DEBUG("Add one connection to connection pool.");
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

        frees_.insert(std::pair<int, Connector*>(cltFd, conn));
    }

    void ConnPool::recycleConn() {
        if (frees_.empty()) {
            return;
        }

        for (auto& iter : frees_) {     //将释放的连接重新建立并加入连接池
            int cltFd = iter.first;
            Connector* conn = iter.second;

            cltFd = fdwrapper::connTcp(conn->cltAddr);
            if (cltFd == -1) {
                LOG_ERROR("fix connection failed.");
            } else {
                LOG_INFO("fix connection success.");
                conn->initClt(cltFd, conn->cltAddr);
                conns_.insert(std::pair<int, Connector*>(cltFd, conn));
            }
        }

        frees_.clear();
    }
}