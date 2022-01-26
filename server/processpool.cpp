//
// Created by xiamingjie on 2021/12/14.
//

#include "processpool.h"
#include "logger.h"
#include "fdwrapper.h"
#include "connpool.h"
#include "connector.h"
#include "epoller.h"

#include <cassert>
#include <unistd.h>
#include <cerrno>
#include <wait.h>
#include <cstring>
#include <stdexcept>

using std::pair;
using std::vector;

namespace server {

    ProcessPool *ProcessPool::instance = nullptr;

    ProcessPool *ProcessPool::create(size_t processNumber) {
        if (!instance) {
            instance = new ProcessPool(processNumber);
        }
        return instance;
    }

    ProcessPool::ProcessPool(size_t processNumber) : idx_(-1), processNumber_(processNumber), epoller_(nullptr),
                                                     stop_(false) {
        assert(processNumber > 0 && processNumber <= MAX_PROCESS_NUMBER);

        subProcess_ = new Process[processNumber];
        assert(subProcess_);

        int ret;
        for (int i = 0; i < processNumber; ++i) {
            ret = socketpair(PF_UNIX, SOCK_STREAM, 0, subProcess_[i].pipeFd);
            assert(ret == 0);

            subProcess_[i].pid = fork();
            assert(subProcess_[i].pid >= 0);

            if (subProcess_[i].pid > 0) {    //主进程
                close(subProcess_[i].pipeFd[1]);    //关闭写端
                continue;
            } else {  //子进程
                close(subProcess_[i].pipeFd[0]);    //关闭读端
                idx_ = i;
                break;
            }
        }
    }

    static int sigPipeFd[2];    //信号通知管道
/**
 * 信号处理函数,统一事件源,将信号在 epoll 事件流程中处理
 * */
    static void sigHandler(int sig) {
        int saveErrno = errno;
        int msg = sig;
        send(sigPipeFd[1], (char *) &msg, 1, 0);
        errno = saveErrno;
    }

    void ProcessPool::run(const vector<pair<Host, Host>> &mappings) {
        if (idx_ != -1) {
            runChild_(mappings[idx_]);
            return;
        }
        runParent_();
    }

    void ProcessPool::setupStep_() {
        try {
            epoller_ = new Epoller(EPOLL_EVENT_NUMBER);
        } catch (...) {
            throw std::runtime_error("New Epoller failure.\n");
        }

        if (idx_ != -1) {
            LOG_INFO("child process %d create epoll", idx_);
        } else {
            LOG_INFO("main process create epoll");
        }

        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sigPipeFd);
        assert(ret != -1);

        epoller_->addReadFd(sigPipeFd[0], true);    //注册信号管道读端事件
        fdwrapper::setNonBlocking(sigPipeFd[1]);

        fdwrapper::addSigHandler(SIGCHLD, sigHandler);    //子进程退出信号
        fdwrapper::addSigHandler(SIGTERM, sigHandler);    //程序终止信号
        fdwrapper::addSigHandler(SIGINT, sigHandler);     //中断信号
        fdwrapper::addSigHandler(SIGPIPE, SIG_IGN);       //忽略 SIGPIPE 信号
    }

/**
 * 主进程主要负责管理子进程,子进程退出后回收资源,接收信号即使处理,关闭子进程等
 * */
    void ProcessPool::runParent_() {
        setupStep_();

        for (int i = 0; i < processNumber_; ++i) {
            epoller_->addReadFd(subProcess_[i].pipeFd[0], true);
        }

        ssize_t ret = -1;
        int eventNumber;
        while (!stop_) {
            eventNumber = epoller_->wait(EPOLL_WAIT_TIME);
            if ((eventNumber == -1) && (errno != EINTR)) {
                LOG_ERROR("master process epoll failure");
                break;
            }

            for (int i = 0; i < eventNumber; ++i) {
                int evFd = epoller_->eventFd(i);
                uint32_t evEvent = epoller_->event(i);

                /* 信号事件 */
                if ((evFd == sigPipeFd[0]) && (evEvent & EPOLLIN)) {
                    char signals[1024];
                    ret = recv(evFd, signals, sizeof(signals), 0);
                    if (ret <= 0) { continue; }
                    else {
                        for (int j = 0; j < ret; ++j) {
                            switch (signals[j]) {
                                case SIGCHLD: {     //子进程退出,回收子进程资源
                                    pid_t pid;
                                    int stat;
                                    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                                        for (int k = 0; k < processNumber_; ++k) {
                                            if (subProcess_[k].pid == pid) {
                                                epoller_->closeFd(subProcess_[k].pipeFd[0]);
                                                subProcess_[k].pid = -1;
                                                LOG_INFO("child process %d join", k);
                                            }
                                        }
                                    }

                                    stop_ = true;
                                    for (int k = 0; k < processNumber_; ++k) {     //还有子进程未退出
                                        if (subProcess_[k].pid != -1) {
                                            stop_ = false;
                                        }
                                    }
                                    break;
                                }
                                    /* 终止程序 */
                                case SIGTERM:
                                case SIGINT: {
                                    LOG_INFO("kill all the child process");
                                    /* 给每个子进程发送终止信号 */
                                    for (int k = 0; k < processNumber_; ++k) {
                                        pid_t cPid = subProcess_[k].pid;
                                        if (cPid != -1) {
                                            LOG_DEBUG("kill SIGTERM to child:%d", cPid);
                                            kill(cPid, SIGTERM);
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        for (int i = 0; i < processNumber_; ++i) {
            if (subProcess_[i].pid != -1) {
                epoller_->closeFd(subProcess_[i].pipeFd[0]);
            }
        }

        epoller_->closeFd(sigPipeFd[0]);
        delete epoller_;
        epoller_ = nullptr;
    }

    ProcessPool::~ProcessPool() {
        delete[] subProcess_;
    }

/**
 * 子进程负责具体的映射数据交换
 * */
    void ProcessPool::runChild_(const std::pair<Host, Host> &mapping) {
        setupStep_();

        Host publicHost = mapping.first;    //公共端口
        Host forwardHost = mapping.second;  //转发端口

        int listenFd = fdwrapper::listenTcp(publicHost);      //本地监听端 sock
        int forwardFd = fdwrapper::listenTcp(forwardHost);    //映射端 sock

        if (listenFd == -1) {
            LOG_ERROR("address: %s:%d listen fail. %s", publicHost.hostName.c_str(), publicHost.port, strerror(errno));
            return;
        }

        if (forwardFd == -1) {
            LOG_ERROR("address: %s:%d forward of listen fail. %s", forwardHost.hostName.c_str(), forwardHost.port,
                      strerror(errno));
            return;
        }

        //连接池
        auto connPool = new server::ConnPool(epoller_);

        epoller_->addReadFd(listenFd, true);
        epoller_->addReadFd(forwardFd, true);

        LOG_INFO("Mapping: [%s:%d] -> [%s:%d]", publicHost.hostName.c_str(), publicHost.port,
                 forwardHost.hostName.c_str(), forwardHost.port);

        ssize_t ret = -1;
        int eventNumber;
        while (!stop_) {
            eventNumber = epoller_->wait(EPOLL_WAIT_TIME);
            if ((eventNumber) < 0 && (errno != EINTR)) {
                LOG_ERROR("child process %d epoll failure", idx_);
                break;
            }

            for (int i = 0; i < eventNumber; ++i) {
                int evFd = epoller_->eventFd(i);
                uint32_t evEvent = epoller_->event(i);

                /* 监听端或转发端连接到来 */
                if (((evFd == listenFd) && (evEvent & EPOLLIN)) ||
                    ((evFd == forwardFd) && (evEvent & EPOLLIN))) {
                    struct sockaddr_in peerAddr = {};
                    memset(&peerAddr, 0, sizeof(peerAddr));
                    socklen_t peerLen = sizeof(peerAddr);

                    int connFd = accept(evFd, (struct sockaddr *) &peerAddr, &peerLen);
                    if (connFd <= 0) continue;

                    /* 转发端有新连接，将它加入连接池 */
                    if (evFd == forwardFd) {
                        /* 添加服务端连接 */
                        connPool->addConn(connFd, peerAddr);
                    }
                        /* 监听端有新连接，从连接池中选取一个转发端的连接并将该新连接初始化为客户端连接 */
                    else {
                        Connector *conn = connPool->pickConn(connFd);
                        if (conn == nullptr) {  //连接池没有连接
                            fdwrapper::closeFd(connFd);
                            continue;
                        }

                        //将该连接初始化为客户端连接
                        conn->initClt(connFd, peerAddr);
                    }
                }
                    /* 信号事件 */
                else if ((evFd == sigPipeFd[0]) && (evEvent & EPOLLIN)) {
                    char signals[1024];
                    ret = recv(evFd, signals, sizeof(signals), 0);
                    if (ret <= 0) { continue; }
                    else {
                        for (int j = 0; j < ret; ++j) {
                            switch (signals[j]) {
                                case SIGCHLD: {
                                    pid_t pid;
                                    int stat;
                                    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
                                    break;
                                }
                                case SIGTERM:
                                case SIGINT: {
                                    stop_ = true;
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                    }
                } else if (evEvent & EPOLLIN) {     //可读事件
                    LOG_DEBUG("read event.");
                    connPool->process(evFd, server::OP_TYPE::READ);
                } else if (evEvent & EPOLLOUT) {    //可写事件
                    LOG_DEBUG("write event.");
                    connPool->process(evFd, server::OP_TYPE::WRITE);
                } else {}
            }
        }

        epoller_->closeFd(listenFd);
        epoller_->closeFd(forwardFd);
        epoller_->closeFd(sigPipeFd[0]);
        delete connPool;
        delete epoller_;
    }
}