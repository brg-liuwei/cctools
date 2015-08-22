#ifndef __CCTOOLS_POOL_H__
#define __CCTOOLS_POOL_H__

#include "cctools_config.h"

#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#include <vector>

#ifndef NDEBUG
#include <iostream>
#endif

using namespace std;

namespace cctools {

    struct Base {
#ifdef NDEBUG
        Base() {}
        virtual ~Base() {}
#else
        Base() { cout << "Base()" << endl; }
        virtual ~Base() { cout << "~Base()" << endl; }
#endif
    };

    class Pool {
        public:
            explicit Pool(size_t size = PAGESIZE);
            virtual ~Pool();
            Base *Add(Base *base);
            void *Alloc(size_t size);

        private:
            void *pool;
            vector<Base *> objs;

        private:
            // No copying allowed
            Pool(const Pool&);
            void operator=(const Pool&);
    };

    /*
     * Usage:
     *     class A : public cctools::Base { ... };
     *     cctools::Pool *mempool = new cctools::Pool();
     *     A *a;
     *
     *     NewClassFromPool(mempool, A, a);
     *
     *     // do some staff with a
     *
     *     delete mempool;
     */
#define NewClassFromPool(pool, derivedClass, ptr) do { \
    void *buff = pool->Alloc(sizeof(derivedClass)); \
    assert(buff != NULL); \
    cctools::Base *basePtr = pool->Add(new (buff) derivedClass); \
    ptr = dynamic_cast<derivedClass *>(basePtr); \
    assert(ptr != NULL); \
} while (0)

};

#endif
