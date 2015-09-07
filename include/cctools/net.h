#ifndef __CCTOOLS_NET_H__
#define __CCTOOLS_NET_H__

#include <string>
#include <queue>
#include <vector>

#include <assert.h>
#include <unistd.h>

#include "cctools/util.h"
#include "cctools/logger.h"

using namespace std;

struct sockaddr;
struct sockaddr_in;

namespace cctools {

class Net;

enum EventType {
    EVT_ERROR = 0,
    EVT_READ,
    EVT_WRITE,
    EVT_EXPIRE,
};

class Event {
    public:
        explicit Event(int exp) : exp(exp) {
            UpdateTimestamp();
        }
        virtual ~Event() {}
        int GetTimestamp() const { return ts; }
        int ExpireTimestamp() const { return ts + exp; }
        void SetExpire(int exp) {
            assert(exp > 0);
            this->exp = exp;
        }
        void UpdateTimestamp() { ts = Now(); }
        EventType Type() const { return type; }
        void SetNet(Net *net) { this->net = net; }
        void SetLogger(Logger *l) { logger = l; }
        virtual string Name() = 0;
        virtual void Proc(EventType type) = 0;
        virtual bool Active() = 0;

    protected:
        Net *net;
        EventType type;
        Logger *logger;

    private:
        int ts;
        int exp; // expire in milliseconds

    private:
        // No copy allowed
        Event(const Event &);
        void operator=(const Event &);
};

struct pEventComparator {
    bool operator()(const Event *e1, const Event *e2) {
        return e1->ExpireTimestamp() > e2->ExpireTimestamp();
    }
};

class IOEvent : public Event {
    public:
        IOEvent(int fd, int exp) : Event(exp), fd(fd) {}
        ~IOEvent() {
            if (fd >= 0) {
                close(fd);
                fd = -1;
            }
        }
        int GetFd() const { return fd; }
        virtual string Name() = 0;
        virtual void Proc(EventType type) = 0;
        virtual bool Active() { return fd >= 0; }
    protected:
        IOEvent() : Event(1000 * 10) {} /* 10s */
    protected:
        int fd;
    private:
        IOEvent(const IOEvent &);
        void operator=(const IOEvent &);
};

typedef IOEvent* (*IOEventCreater)(int fd, sockaddr *addr, Logger *l);

class ListenEvent : public IOEvent {
    public:
        ListenEvent(string ip, int port, IOEventCreater func, Logger *l);
        virtual string Name();
        virtual void Proc(EventType type);
    private:
        void acceptProc();
    private:
        string listenAddr;
        int listenPort;
        IOEventCreater ioevCreater;
    private:
        ListenEvent(const ListenEvent &);
        void operator=(const ListenEvent &);
};

class CommonIOEvent : public IOEvent {
    public:
        CommonIOEvent(int fd, int exp, string cliAddr, int cliPort, Logger *l);
        virtual string Name();
        virtual void Proc(EventType type);
    private:
        void readProc();
        void writeProc();
    private:
        string cliAddr;
        int cliPort;
        int headIdx;
        char head[9]; // format eg: "00000123" => rbuf length is 123
        int rremain;
        string rbuf;
        string wbuf;
    private:
        CommonIOEvent(const CommonIOEvent &);
        void operator=(const CommonIOEvent &);
};

IOEvent *CommonIOEventCreater(int fd, sockaddr *addr, Logger *l);

class Net {
    public:
        explicit Net(Logger *l);
        virtual ~Net();
        void PushEvent(Event *e);
        Event *PopEvent();
        bool AddIOEvent(IOEvent *e);
        bool ModIOEvent(IOEvent *e);
        bool DelIOEvent(IOEvent *e);
        void Start();
        void Stop();

    private:
        int evfd;
        bool running;
        Logger *logger;
        priority_queue<Event *, vector<Event *>, pEventComparator> heap;

    private:
        // No copy allowed
        Net(const Net &);
        void operator=(const Net &);
};

};

#endif
