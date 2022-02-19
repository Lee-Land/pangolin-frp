//
// Created by Mingjie on 2022/2/9.
//

#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <thread>
#include <memory>
#include <functional>
#include <queue>
#include <atomic>

#include "spinlock.h"

class ThreadPool {
public:
    explicit ThreadPool(size_t threadNumber);
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool();

    void AddTask(std::function<void()>&& task);

private:
    struct Pool {
        bool stop = false;                          //线程停止标志
        std::queue<std::function<void()>> tasks;    //任务队列
        SpinLock spinLock;                          //自旋锁
    };

    std::shared_ptr<Pool> pool_;
};


#endif //_THREAD_POOL_H
