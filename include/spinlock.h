//
// Created by Mingjie on 2022/2/19.
//

#ifndef _SPINLOCK_H
#define _SPINLOCK_H

#include <pthread.h>

//封装自旋锁
class SpinLock {
public:
    SpinLock();
    SpinLock(const SpinLock& spinLock) = delete;
    SpinLock& operator = (const SpinLock& spinLock) = delete;
    ~SpinLock();

    void Lock();
    void UnLock();

private:
    pthread_spinlock_t spin_lock_{};
};


#endif //_SPINLOCK_H
