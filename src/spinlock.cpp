//
// Created by Mingjie on 2022/2/19.
//

#include "spinlock.h"

SpinLock::SpinLock() {
    pthread_spin_init(&spin_lock_, PTHREAD_PROCESS_PRIVATE);
}

void SpinLock::Lock() {
    pthread_spin_lock(&spin_lock_);
}

void SpinLock::UnLock() {
    pthread_spin_unlock(&spin_lock_);
}

SpinLock::~SpinLock() {
    pthread_spin_destroy(&spin_lock_);
}