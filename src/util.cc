#include "cctools/util.h"

#include <stdio.h>
#include <sys/time.h>

#include <string>

using namespace std;

namespace cctools {
    int Now() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000 + tv.tv_usec / 1000;
    }
    string Itos(int i) {
        char buf[16];
        int n = snprintf(buf, 16, "%d", i);
        string rt(buf, n);
        return rt;
    }
    string Ctos(const char *s) {
        string rt(s);
        return rt;
    }
};
