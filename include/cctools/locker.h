#ifndef __CCTOOLS_LOCK_H__
#define __CCTOOLS_LOCK_H__

#include <assert.h>
#include <pthread.h>

/*
 * usage:
 *
 * #include "cctools/locker.h"
 *
 * using namespace cctools;
 *
 * Mutex m;
 *
 * void foo() {
 *     Lock l(m);
 *     // do something
 * }
 *
 * RWMutex rwm;
 *
 * void bar() {
 *     RLock rl(rwm);
 *     // do something
 * }
 *
 */

namespace cctools {

class Mutex {
    public:
        Mutex(): mutex(new pthread_mutex_t) {
            int rc = pthread_mutex_init(mutex, NULL);
            assert(rc == 0);
        }
        ~Mutex() {
            int rc = pthread_mutex_destroy(mutex);
            assert(rc == 0);
            delete mutex;
        }
        void Lock() {
            int rc = pthread_mutex_lock(mutex);
            assert(rc == 0);
        }
        void Unlock() {
            int rc = pthread_mutex_unlock(mutex);
            assert(rc == 0);
        }
    public:
        pthread_mutex_t *mutex;
    private:
        Mutex(const Mutex &);
        void operator=(const Mutex &);
};

class RWMutex {
    public:
        RWMutex(): lock(new pthread_rwlock_t) {
            int rc = pthread_rwlock_init(lock, NULL);
            assert(rc == 0);
        }
        ~RWMutex() {
            int rc = pthread_rwlock_destroy(lock);
            assert(rc == 0);
            delete lock;
        }
        void RLock() {
            int rc = pthread_rwlock_rdlock(lock);
            assert(rc == 0);
        }
        void Lock() {
            int rc = pthread_rwlock_wrlock(lock);
            assert(rc == 0);
        }
        void Unlock() {
            int rc = pthread_rwlock_unlock(lock);
            assert(rc == 0);
        }
    public:
        pthread_rwlock_t *lock;
    private:
        RWMutex(const RWMutex &);
        void operator=(const RWMutex &);
};

class Lock {
    public:
        explicit Lock(Mutex &m): mut(m) {
            mut.Lock();
        }
        ~Lock() {
            mut.Unlock();
        }
    private:
        Mutex &mut;
    private:
        Lock(const Lock &);
        void operator=(const Lock &);
};

class RLock {
    public:
        explicit RLock(RWMutex &lock): rwMut(lock) {
            rwMut.RLock();
        }
        ~RLock() {
            rwMut.Unlock();
        }
    private:
        RWMutex &rwMut;
    private:
        RLock(const RLock &);
        void operator=(const RLock &);
};

class WLock {
    public:
        explicit WLock(RWMutex &lock): rwMut(lock) {
            rwMut.Lock();
        }
        ~WLock() {
            rwMut.Unlock();
        }
    private:
        RWMutex &rwMut;
    private:
        WLock(const WLock &);
        void operator=(const WLock &);
};

};

#endif
