//
// Created by xiamingjie on 2021/12/14.
//

#ifndef PROCESS_POOL_H
#define PROCESS_POOL_H

#include <vector>
#include "proto.h"

class Host;
class Epoller;

/**
 * 子进程描述信息
 * */
struct Process {
    int pid;          //进程号
    int pipeFd[2];    //与主进程通信管道
};

class ProcessPool final {
private:
    explicit ProcessPool(size_t processNumber);

public:
    ~ProcessPool();

    /**
     * 单例模式
     * */
    static ProcessPool *instance;

    /**
     * 单例创建进程池对象
     * */
    static ProcessPool *create(size_t processNumber);

    /**
     * 运行进程池
     * */
    void run(const std::vector<std::pair<Host, Host>> &mappings, RunType type);

private:
    /**
     * 运行主进程
     * */
    void runMaster_();

    /**
     * 服务端主程序
     * */
    void runServer_(const std::pair<Host, Host> &);

    /**
     * 客户端主程序
     * */
    void runClient_(const std::pair<Host, Host> &);

    /**
     * 统一事件源
     * */
    void setupStep_();

private:
    static const int MAX_PROCESS_NUMBER = 16;       //最大进程数
    static const int EPOLL_EVENT_NUMBER = 10000;    //epoll 最大处理事件
    static const int EPOLL_WAIT_TIME = 5000;        //epoll_wait 超时时间

    int idx_;                                       //进程编号 -1为主进程
    size_t processNumber_;                          //进程数量
    Epoller* epoller_;                              //每个进程一个 epoll
    Process *subProcess_;                           //子进程描述信息
    bool stop_;                                     //进程是否停止
};

#endif //PROCESS_POOL_H
