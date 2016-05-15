#ifndef __CCTOOLS_NET_H__
#define __CCTOOLS_NET_H__

#include <string>
#include <queue>
#include <vector>

#include <errno.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "cctools/util.h"
#include "cctools/logger.h"

struct sockaddr;
struct sockaddr_in;

using namespace std;

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
        volatile int GetTimestamp() const { return ts; }
        volatile int ExpireTimestamp() const { return ts + exp; }
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
        volatile int ts;
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
        virtual ~IOEvent() {
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
        Net(const Net &);
        void operator=(const Net &);
};

template <typename T>
class ListenEvent : public IOEvent {
    public:
        ListenEvent(string ip, int port, Logger *l, int backlog = 1024) 
            : IOEvent(), listenAddr(ip), listenPort(port)
        {
            type = EVT_READ;

            SetLogger(l);
            fd = socket(PF_INET, SOCK_STREAM, 0);
            if (fd == -1) {
                logger->Crit("fail to create socket: " + Ctos(strerror(errno)));
            }

            int flag = fcntl(fd, F_GETFL, 0);
            if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1) {
                logger->Crit(Name() + 
                        " fcntl set nonblock error: " + Ctos(strerror(errno)));
            }

            int opt = 1;
            if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
                logger->Crit("fail to set socket reuse: " + Ctos(strerror(errno)));
            }

            if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == -1) {
                logger->Crit("fail to set socket no delay: " + Ctos(strerror(errno)));
            }

            sockaddr_in addr;
            addr.sin_family = PF_INET;
            addr.sin_port = htons(listenPort);
            addr.sin_addr.s_addr = inet_addr(listenAddr.c_str());

            if (::bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
                logger->Crit("fail to bind socket: " + Ctos(strerror(errno)));
            }

            if (listen(fd, backlog) == -1) {
                logger->Crit("fail to listen: " + Ctos(strerror(errno)));
            }
        }

        virtual ~ListenEvent() {}

        virtual string Name() {
            string name("ListenEvent[" + listenAddr + ":" + Itos(listenPort) + "]");
            return name;
        }

        virtual void Proc(EventType type) {
            switch (type) {
                case EVT_ERROR:
                    logger->Error(Name() + " error, close socket " + Itos(fd));
                    if (fd >= 0) {
                        close(fd);
                        fd = -1;
                    }
                    break;
                case EVT_READ:
                    acceptProc();
                    break;
                case EVT_WRITE:
                    logger->Crit(Name() + " write event cannot be happened on listen event");
                    break;
                case EVT_EXPIRE:
                    logger->Debug(Name() + " expire event happened");
                    break;
                default:
                    logger->Error(Name() + "Unexpect event type: " + Itos(type));
            }
        }

    private:
        string listenAddr;
        int listenPort;
    private:
        ListenEvent(const ListenEvent &);
        void operator=(const ListenEvent &);
    private:
        void acceptProc() {
            if (net == NULL) {
                logger->Crit(Name() + " net is NULL");
            }
            sockaddr_in remoteAddr;
            socklen_t socklen;
            while (true) {
                bzero(reinterpret_cast<void *>(&remoteAddr), sizeof(sockaddr_in));
                int sock = accept(fd,
                        reinterpret_cast<sockaddr *>(&remoteAddr), &socklen);
                if (sock == -1) {
                    if (errno != EWOULDBLOCK) {
                        logger->Error(Name() + " accept error: " +
                                Ctos(strerror(errno)));
                    }
                    return;
                }

                int flag = fcntl(sock, F_GETFL, 0);
                if (fcntl(sock, F_SETFL, flag | O_NONBLOCK) == -1) {
                    logger->Crit(Name() + 
                            " fcntl set accepted sock nonblock error: " +
                            Ctos(strerror(errno)));
                }

                int opt = 1;
                if (setsockopt(sock, IPPROTO_TCP,
                            TCP_NODELAY, &opt, sizeof(opt)) == -1)
                {
                    logger->Crit("fail to set accepted socket no delay: " +
                            Ctos(strerror(errno)));
                }

                string cliAddr(inet_ntoa(
                            reinterpret_cast<sockaddr_in *>(&remoteAddr)->sin_addr));
                int cliPort(ntohs(
                            reinterpret_cast<sockaddr_in *>(&remoteAddr)->sin_port));
                IOEvent *e = new T(sock, 10 * 60 * 1000, 
                        cliAddr, cliPort, logger); // expire: 10min

                if (e == NULL) {
                    logger->Error(Name() + ": create IOEvent NULL");
                    close(sock);
                } else if (net->AddIOEvent(e) == false) {
                    logger->Error(Name() + " add IOEvent error, event: " + e->Name());
                    delete e;
                }
            }
        }
};

class CommonIOEvent : public IOEvent {
    public:
        CommonIOEvent(int fd, int exp, string cliAddr, int cliPort, Logger *l);
        ~CommonIOEvent() {}
        virtual string Name();
        virtual void Proc(EventType type);
    protected:
        virtual void bufProc(string &rbuf, string &wbuf);
    private:
        void readProc();
        void writeProc();
    protected:
        string cliAddr;
        int cliPort;
    private:
        int headIdx;
        char head[9]; // format eg: "00000123" => rbuf length is 123
        int rremain;
        string rbuf;
        string wbuf;
    private:
        CommonIOEvent(const CommonIOEvent &);
        void operator=(const CommonIOEvent &);
};

};

#endif
