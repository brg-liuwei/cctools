#include "cctools/conf.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <algorithm>

#ifndef NDEBUG
#include <iostream>
#endif

namespace cctools {
namespace {
    string &trim(string &s) {
        string::size_type pos = s.find_first_not_of(' ');
        if (pos != string::npos) {
            s.erase(0, pos);
            pos = s.find_last_not_of(' ');
            if (pos != string::npos) {
                s.erase(pos+1);
            }
        } else {
            s.erase(s.begin(), s.end());
        }
        return s;
    }

    typedef unsigned int state_t;
    const static state_t s_sign = 0;
    const static state_t s_num = 1;

#define IS_NUM(v) ((v) >= '0' && (v) <= '9')
#define APPEND_INT(v, i) ((v) * 10 + i)

    bool strAtoi(string s, int &v) {
        int sign = 1;
        state_t state = s_sign;
        v = 0;
        for (string::iterator itr = s.begin(); itr != s.end(); ++itr) {
            switch (state) {
                case s_sign:
                    if (*itr == '+') {
                        // do nothing
                    } else if (*itr == '-') {
                        sign = -1;
                    } else if (IS_NUM(*itr)) {
                        v = APPEND_INT(v, *itr - '0');
                    } else {
                        // error
                        return false;
                    }
                    state = s_num;
                    break;
                case s_num:
                    if (IS_NUM(*itr)) {
                        v = APPEND_INT(v, *itr - '0');
                    } else {
                        return false;
                    }
                    break;
                default:
                    return false;
            }
        }
        v *= sign;
        return true;
    }
}; // end of anonymous namespace

Conf::Conf(FILE *openFp) {
    new (this) Conf(openFp, ';');
}

Conf::Conf(FILE *openFp, char sep) : fp(openFp) {
    char lineBuff[4096], line[4096];
    setvbuf(fp, lineBuff, _IOLBF, 4096);

    while (fscanf(fp, "%4096[^\n]\n", line) != EOF) {
        char *psep = strchr(line, sep);
        if (psep == NULL) {
            fprintf(stderr, " >>> error line: %s\n", line);
            continue;
        }
        *psep = '\0';
        string key(line), val(psep+1);
        m.insert(make_pair<string,string>(trim(key), trim(val)));
    }
#ifndef NDEBUG
    // dump conf file
    for (map<string, string>::iterator itr = m.begin(); itr != m.end(); ++itr) {
        cout << "key: " << itr->first << " val: " << itr->second << endl;
    }
#endif
    fclose(fp);
    fp = NULL;
}

Conf::~Conf() {
    if (fp != NULL) {
        fclose(fp);
    }
}

bool Conf::split(string key, vector<string> &vec, char sep) {
    vec.clear();
    map<string, string>::iterator itr = m.find(key);
    string sub;
    if (itr != m.end()) {
        string v(itr->second);
        string::size_type pos(0), new_pos;
        for ( ; ; ) {
            new_pos = v.find(sep, pos);
            if (new_pos == string::npos) {
                break;
            }
            sub = v.substr(pos, new_pos - pos);
            vec.push_back(trim(sub));
            pos = new_pos + 1;
        }
        sub = v.substr(pos);
        vec.push_back(trim(sub));
        return true;
    }
    return false;
}

bool Conf::GetString(string key, string &val) {
    map<string, string>::iterator itr = m.find(key);
    if (itr == m.end()) {
        return false;
    }
    val.assign(itr->second);
    return true;
}

string Conf::String(string key) {
    string val;
    GetString(key, val);
    return val;
}

bool Conf::GetStringVec(string key, vector<string> &val, char sep) {
    return split(key, val, sep);
}

vector<string> Conf::StringVec(string key, char sep) {
    vector<string> vec;
    GetStringVec(key, vec, sep);
    return vec;
}

bool Conf::GetInt(string key, int &val) {
    val = 0;
    map<string, string>::iterator itr = m.find(key);
    if (itr == m.end()) {
        return false;
    }
    return strAtoi(itr->second, val);
}

int Conf::Int(string key) {
    int val(0);
    GetInt(key, val);
    return val;
}

vector<int> Conf::IntVec(string key, char sep) {
    vector<int> vec;
    GetIntVec(key, vec, sep);
    return vec;
}

bool Conf::GetIntVec(string key, vector<int> &val, char sep) {
    val.clear();
    vector<string> svec;
    if (!split(key, svec, sep)) {
        return false;
    }
    int ival;
    for (vector<string>::iterator itr = svec.begin(); itr != svec.end(); ++itr) {
        if (strAtoi(*itr, ival)) {
            val.push_back(ival);
        }
    }
    return true;
}

Conf *NewConf(string filename) {
    return NewConf(filename, ':');
}

Conf *NewConf(string filename, char sep) {
    FILE *fp = fopen(filename.c_str(), "r");
    if (fp == NULL) {
        perror(filename.c_str());
        return NULL;
    }
    return new Conf(fp, sep);
}

};
