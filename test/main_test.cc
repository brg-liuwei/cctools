#include "cctools/pool.h"
#include "cctools/threadSafeMap.h"
#include "cctools/conf.h"
#include "cctools/logger.h"

#include <new>
#include <iostream>
#include <assert.h>

using namespace std;
using namespace cctools;

struct Derived : public Base {
    Derived() { cout << "Derived():" << this << endl; }
    void Offset() { cout << "offset: " << this << endl; }
    virtual ~Derived() { cout << "~Derived():" << this << endl; }
};

int test_pool() {
    Pool *p = new Pool();
    Derived *ptr1, *ptr2, *ptr3;

    NewClassFromPool(p, Derived, ptr1);
    NewClassFromPool(p, Derived, ptr2);
    NewClassFromPool(p, Derived, ptr3);

    ptr1->Offset();
    ptr2->Offset();
    ptr3->Offset();

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

void test_conf() {
    Conf *c = NewConf("test/cctools.cfg");
    vector<string> val;
    if (c->GetStringVec("stringVec", val, ';')) {
        for (vector<string>::iterator itr = val.begin(); itr != val.end(); ++itr) {
            cout << " >>>>> vec1: " << *itr << " len: " << itr->size() << endl;
        }
    }
    if (c->GetStringVec("stringVec2", val, ';')) {
        for (vector<string>::iterator itr = val.begin(); itr != val.end(); ++itr) {
            cout << " >>>>> vec2: " << *itr << " len: " << itr->size() << endl;
        }
    }
    int ival;
    if (c->GetInt("intKey", ival)) {
        cout << ">>> intVal:" << ival << endl;
    }
    vector<int> ivec;
    if (c->GetIntVec("intKeyVec", ivec, ',')) {
        for (vector<int>::iterator itr = ivec.begin(); itr != ivec.end(); ++itr) {
            cout << "-------> vec int: " << *itr << endl;
        }
    }
    delete c;
    // Conf *c2 = NewConf("test/notexist.cfg");
    // delete c2;
}

void test_logger() {
    Logger *log = new Logger("log_test.log");
    log->Debug("this is debug message");
    log->Info("this is info message");
    string long_msg("this is long message ");
    string msg;
    while (msg.size() <= 1024) {
        msg.append(long_msg);
    }
    log->Warn(msg);
    log->Error("this is error message");
    log->Crit("this is crit message, program will abort after this record");
    delete log;
}

int main() {
    test_pool();
    test_thread_safe_map();
    test_conf();
    test_logger();
    return 0;
}
