#ifndef __CCTOOLS_THREAD_SAFE_MAP_H__
#define __CCTOOLS_THREAD_SAFE_MAP_H__

#include "cctools/locker.h"

#include <map>

#include <pthread.h>
#include <assert.h>

using namespace std;

namespace cctools {
    template<typename K, typename V>
    class ThreadSafeMap {
        public:
            ThreadSafeMap() {}
            virtual ~ThreadSafeMap() {}

            void UnlockPut(K &key, V &val) {
                unsafeMap[key] = val;
            }
            void Put(K &key, V &val) {
                WLock wl(rwmut);
                UnlockPut(key, val);
            }

            bool UnlockGet(K &key, V &val) {
                typename map<K, V>::iterator itr = unsafeMap.find(key);
                if (itr == unsafeMap.end()) {
                    return false;
                }
                val = itr->second;
                return true;
            }
            bool Get(K &key, V &val) {
                RLock rl(rwmut);
                return UnlockGet(key, val);
            }

            void UnlockDel(K &key) {
                unsafeMap.erase(key);
            }
            void Del(K &key) {
                WLock wl(rwmut);
                UnlockDel(key);
            }

            RWMutex &GetRWMutexRef() {
                return rwmut;
            }
        protected:
            RWMutex rwmut;
            map<K, V> unsafeMap;
        private:
            // No copying allowed
            ThreadSafeMap(const ThreadSafeMap &obj);
            void operator=(const ThreadSafeMap &obj);
    };
};

#endif
