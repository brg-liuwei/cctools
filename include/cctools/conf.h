#ifndef __CCTOOLS_CONF_H__
#define __CCTOOLS_CONF_H__

#include <string>
#include <vector>
#include <map>

#include <stdio.h>

using namespace std;

namespace cctools {
    class Conf {
        public:
            explicit Conf(FILE *fp);
            Conf(FILE *fp, char sep);
            virtual ~Conf();

            bool GetString(string key, string &val);
            bool GetStringVec(string key, vector<string> &val, char sep);
            bool GetInt(string key, int &val);
            bool GetIntVec(string key, vector<int> &val, char sep);

            string String(string key);
            vector<string> StringVec(string key, char sep);
            int Int(string key);
            vector<int> IntVec(string key, char sep);

        private:
            FILE *fp;
            map<string, string> m;

        private:
            bool split(string key, vector<string> &vec, char sep);

        private:
            // No copying allowed
            Conf(const Conf &);
            void operator=(const Conf &);
    };

Conf *NewConf(string filename);
Conf *NewConf(string filename, char sep);

};

#endif
