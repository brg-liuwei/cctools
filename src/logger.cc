#include "cctools/logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include <string>

using namespace std;

namespace cctools {

namespace {
    char levels[][sizeof("[DEBUG]")] = {
        "[DEBUG]",
        "[WARN]",
        "[INFO]",
        "[ERROR]",
        "[CRIT]"
    };
    const size_t bufsize = 1024;

    void log(int fd, int level, const string &msg) {
        char buf[bufsize];
        size_t nprefix = (sizeof("2015-08-17 22:31:59 888 ") - 1) + (strlen(levels[level]) + 1);

        // two more bytes, one for '\n', other one for '\0'
        size_t record_size = nprefix + msg.size() + 2;
        bool alloc = false;
        char *p;

        if (record_size >= bufsize) {
            p = (char *)calloc(record_size, sizeof(char));
            assert(p != NULL);
            alloc = true;
        } else {
            p = (char *)buf;
        }

        struct timeval tv;
        struct tm now;

        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &now);

        int n = sprintf(p, "%04d-%02d-%02d %02d:%02d:%02d %03d %s %s\n",
                now.tm_year + 1990,
                now.tm_mon + 1,
                now.tm_mday,
                now.tm_hour,
                now.tm_min,
                now.tm_sec,
                tv.tv_usec / 1000,
                levels[level],
                msg.c_str());

        write(fd, p, n);

        if (alloc) {
            free(p);
        }
    }
}; // end of amonious namespace

Logger::Logger(const string path, LogLevel level) {
    assert(level >= DEBUG && level <= CRIT);
    this->level = level;
    this->path = path;
    this->fd = open(path.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0666);
    assert(this->fd >= 0);
}

Logger::~Logger() {
    if (this->fd >= 0) {
        close(this->fd);
    }
    this->fd = -1;
}

void Logger::Debug(string msg) {
    if (this->level <= DEBUG) {
        log(this->fd, DEBUG, msg);
    }
}

void Logger::Info(string msg) {
    if (this->level <= INFO) {
        log(this->fd, INFO, msg);
    }
}

void Logger::Warn(string msg) {
    if (this->level <= WARN) {
        log(this->fd, WARN, msg);
    }
}

void Logger::Error(string msg) {
    if (this->level <= ERROR) {
        log(this->fd, ERROR, msg);
    }
}

void Logger::Crit(string msg) {
    log(this->fd, CRIT, msg);
    abort();
}

};
