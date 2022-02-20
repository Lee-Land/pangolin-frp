//
// Created by Mingjie on 2022/2/9.
//

#include "threadpool.h"
#include <cassert>

using std::thread;

ThreadPool::ThreadPool(size_t threadNumber) : pool_(new Pool) {
    assert(threadNumber > 0);

    for (size_t i = 0; i < threadNumber; ++i) {
        thread([p = pool_]() {
            while (true) {
                p->spinLock.Lock();        //获得锁
                //获得锁
                while (!p->tasks.empty()) {
                    auto task = std::move(p->tasks.front());
                    p->tasks.pop();
                    p->spinLock.UnLock();  //释放锁
                    task();
                    p->spinLock.Lock();    //继续自旋，尝试获取锁
                }

                p->spinLock.UnLock();      //任务队列空，释放锁
                if (p->stop) {             //线程结束
                    break;
                }
            }
        }).detach();
    }
}

void ThreadPool::AddTask(std::function<void()> &&task) {
    pool_->spinLock.Lock();
    pool_->tasks.emplace(std::forward<decltype(task)>(task));
    pool_->spinLock.UnLock();
}

ThreadPool::~ThreadPool() {
    if (pool_) {
        pool_->spinLock.Lock();
        pool_->stop = true;
        pool_->spinLock.UnLock();
        delete pool_;
        pool_ = nullptr;
    }
}