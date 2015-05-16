#include "cctools/pool.h"
#include "fy_alloc.h"

#include <assert.h>

#include <iostream>

using namespace std;

namespace cctools {
    Pool::Pool(size_t size) {
        pool = (void *)fy_pool_create(size);
        assert(pool != NULL);
    }

    Pool::~Pool() {
        for (vector<Base *>::iterator itr = objs.begin();
                itr != objs.end(); ++itr)
        {
            (*itr)->~Base();
        }
        fy_pool_destroy((fy_pool_t *)pool);
    }

    void Pool::Add(Base *base) {
        if (base != NULL) {
            objs.push_back(base);
        }
    }

    void *Pool::Alloc(size_t size) {
        return fy_pool_alloc((fy_pool_t *)pool, size);
    }
};
