#include "cctools/pool.h"
#include "cctools/threadSafeMap.h"

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

int test_pool() {
    Pool *p = new Pool();

    Derived::NewDerived(p);
    Derived::NewDerived(p);
    Derived::NewDerived(p);

    delete p;
    return 0;
}

int test_thread_safe_map() {
    string val;
    ThreadSafeMap<string, string> m;
    string key1("key1"), key2("key2"), val1("val1"), val2("val2");

    m.Put(key1, val1);
    m.Put(key2, val1);
    cout << "key1: " << m.Get(key1, val) << ", val: " << val << endl;
    cout << "key2: " << m.Get(key2, val) << ", val: " << val << endl;
    cout << "val1: " << m.Get(val1, val) << ", val: " << val << endl;

    m.Put(key2, val2);
    cout << "update key2: " << m.Get(key2, val) << ", val: " << val << endl;

    m.Del(key1);
    cout << "delete key1: " << m.Get(key1, val) << ", val: " << val << endl;

    return 0;
}

int main() {
    test_pool();
    test_thread_safe_map();
}
