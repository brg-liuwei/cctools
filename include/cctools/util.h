#ifndef __CCTOOLS_UTIL_H__
#define __CCTOOLS_UTIL_H__

#include <string>
#include <sstream>

using namespace std;

namespace cctools {
    int Now();
    string Itos(int i);
    string Ctos(const char *s);
    template <typename T>
    string Ttos(T t) {
        stringstream ss;
        ss << t;
        string str;
        ss >> str;
        return str;
    }
};

#endif
