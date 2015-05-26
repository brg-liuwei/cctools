#include "cctools/conf.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#ifndef NDEBUG
#include <iostream>
#endif

namespace cctools {

    Conf::Conf(FILE *openFp) {
        new (this) Conf(openFp, ';');
    }

    Conf::Conf(FILE *openFp, char sep): fp(openFp) {
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
            m.insert(make_pair<string,string>(key, val));
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
