#ifndef __CCTOOLS_LOGGER_H__
#define __CCTOOLS_LOGGER_H__

#include <assert.h>

#include <string>

using namespace std;

namespace cctools {

    enum LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERROR,
        CRIT
    };

    class Logger {
        public:
            explicit Logger(const string path, LogLevel level = DEBUG);
            virtual ~Logger();

        private:
            int fd;
            string path;
            LogLevel level;

        public:
            void SetLevel(LogLevel level) {
                assert(level >= DEBUG && level <= CRIT);
                this->level = level;
            }
            LogLevel GetLevel() { return this->level; }
            string GetPath() { return this->path; }

            void Debug(string msg);
            void Info(string msg);
            void Warn(string msg);
            void Error(string msg);
            void Crit(string msg);

        private:
            // No copying allowed
            Logger(const Logger &);
            void operator=(const Logger &);

    };
};

#endif
