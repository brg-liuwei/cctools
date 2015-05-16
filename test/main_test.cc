#include "cctools/pool.h"

#include <new>
#include <iostream>
#include <assert.h>

using namespace std;
using namespace cctools;

struct Derived : public Base {
    Derived() { cout << "Derived()" << endl; }
    virtual ~Derived() { cout << "~Derived()" << endl; }

    static Base *NewDerived(Pool *pool) {
        void *buff = pool->Alloc(sizeof(Derived));
        assert(buff != NULL);
        Base *obj = new (buff) Derived;
        assert(obj != NULL);
        pool->Add(obj);
        return obj;
    }
};

int main() {
    Pool *p = new Pool();

    Derived::NewDerived(p);
    Derived::NewDerived(p);
    Derived::NewDerived(p);

    delete p;
    return 0;
}
