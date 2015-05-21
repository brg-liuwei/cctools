#ifndef __CCTOOLS_THREAD_SAFE_MAP_H__
#define __CCTOOLS_THREAD_SAFE_MAP_H__

#include <map>

#include <pthread.h>
#include <assert.h>

using namespace std;

namespace cctools {
    namespace {
        class rLock {
            public:
                explicit rLock(pthread_rwlock_t *l): lock(l) {
                    pthread_rwlock_rdlock(lock);
                }
                virtual ~rLock() {
                    pthread_rwlock_unlock(lock);
                }
            private:
                pthread_rwlock_t *lock;
            // No copying allowed
            private:
                rLock(const rLock &obj);
                void operator=(const rLock &obj);
        };
        class wLock {
            public:
                explicit wLock(pthread_rwlock_t *l): lock(l) {
                    pthread_rwlock_wrlock(lock);
                }
                virtual ~wLock() {
                    pthread_rwlock_unlock(lock);
                }
            private:
                pthread_rwlock_t *lock;
            // No copying allowed
            private:
                wLock(const wLock &obj);
                void operator=(const wLock &obj);
        };
    };
    template<typename K, typename V>
    class ThreadSafeMap {
        public:
            ThreadSafeMap() {
                lock = new pthread_rwlock_t;
                assert(lock != NULL);
                pthread_rwlock_init(lock, NULL);
            }
            virtual ~ThreadSafeMap() {
                pthread_rwlock_destroy(lock);
                delete lock;
            }
            void Put(K &key, V &val) {
                wLock l(lock);
                unsafeMap[key] = val;
            }
            bool Get(K &key, V &val) {
                rLock l(lock);
                typename map<K, V>::iterator itr = unsafeMap.find(key);
                if (itr == unsafeMap.end()) {
                    return false;
                }
                val = itr->second;
                return true;
            }
            void Del(K &key) {
                wLock l(lock);
                unsafeMap.erase(key);
            }
        private:
            pthread_rwlock_t *lock;
            map<K, V> unsafeMap;
        private:
            // No copying allowed
            ThreadSafeMap(const ThreadSafeMap &obj);
            void operator=(const ThreadSafeMap &obj);
    };
};

#endif
