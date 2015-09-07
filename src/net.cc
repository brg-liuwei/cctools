#include "cctools/net.h"
#include "cctools/util.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "cctools_config.h"

#ifdef HAVE_EPOLL
    #include <sys/epoll.h>
#elif defined HAVE_KQUEUE
    #include <sys/types.h>
    #include <sys/event.h>
    #include <sys/time.h>
#endif

#include <string>

using namespace std;

namespace cctools {

ListenEvent::ListenEvent(string ip, int port, IOEventCreater func, Logger *l) :
    IOEvent(), listenAddr(ip), listenPort(port), ioevCreater(func)
{
    type = EVT_READ;
    SetLogger(l);
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        logger->Crit("fail to create socket: " + Ctos(strerror(errno)));
    }

    int flag = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1) {
        logger->Crit(Name() + " fcntl set nonblock error: " + Ctos(strerror(errno)));
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        logger->Crit("fail to set socket reuse: " + Ctos(strerror(errno)));
    }

    sockaddr_in addr;
    addr.sin_family = PF_INET;
    addr.sin_port = htons(listenPort);
    addr.sin_addr.s_addr = inet_addr(listenAddr.c_str());

    if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
        logger->Crit("fail to bind socket: " + Ctos(strerror(errno)));
    }

    if (listen(fd, 128) == -1) {
        logger->Crit("fail to listen: " + Ctos(strerror(errno)));
    }
}

string ListenEvent::Name() {
    string name("ListenEvent[" + listenAddr + ":" + Itos(listenPort) + "]");
    return name;
}

void ListenEvent::acceptProc() {
    if (net == NULL) {
        logger->Crit(Name() + " net is NULL");
    }
    sockaddr_in remoteAddr;
    socklen_t socklen;
    while (true) {
	bzero(reinterpret_cast<void *>(&remoteAddr), sizeof(sockaddr_in));
        int sock = accept(fd, reinterpret_cast<sockaddr *>(&remoteAddr), &socklen);
        if (sock == -1) {
            if (errno != EWOULDBLOCK) {
                logger->Error(Name() + " accept error: " + Ctos(strerror(errno)));
            }
            return;
        }
        IOEvent *e = ioevCreater(sock, reinterpret_cast<sockaddr *>(&remoteAddr), logger);
        if (e == NULL) {
            logger->Error(Name() + "\'s IOEventCreater create NULL event");
            close(sock);
        } else if (net->AddIOEvent(e) == false) {
            logger->Error(Name() + " add IOEvent error, event: " + e->Name());
            delete e;
        }
    }
}

void ListenEvent::Proc(EventType type) {
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

CommonIOEvent::CommonIOEvent(int fd, int exp, string cliAddr, int cliPort, Logger *l) :
    IOEvent(fd, exp), cliAddr(cliAddr), cliPort(cliPort),
    headIdx(0), rremain(-1)
{
    SetLogger(l);
    type = EVT_READ;
    head[8] = '\0';
    int flag = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1) {
        logger->Crit(Name() + " fcntl set nonblock error: " + Ctos(strerror(errno)));
    }
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) == -1) {
        logger->Crit(Name() + " set keepalive error: " + Ctos(strerror(errno)));
    }
}

string CommonIOEvent::Name() {
    string name("CommonIOEvent[" + cliAddr + ":" + Itos(cliPort) + "]");
    return name;
}

void CommonIOEvent::Proc(EventType type) {
    switch (type) {
        case EVT_ERROR:
            if (fd >= 0) {
                close(fd);
                fd = -1;
                logger->Error(Name() + " error, close socket");
            }
            break;
        case EVT_EXPIRE:
            if (fd >= 0) {
                close(fd);
                fd = -1;
            }
            logger->Debug(Name() + " expired, close socket");
            break;
        case EVT_READ:
            readProc();
            break;
        case EVT_WRITE:
            writeProc();
            break;
        default:
            logger->Error(Name() + "Unexpect event type: " + Itos(type));
    }
}

void CommonIOEvent::readProc() {
    if (rremain < 0) {
        // read header
        assert(headIdx >= 0 && headIdx < 8);
        while (headIdx < 8) {
            int n = read(fd, head + headIdx, 8 - headIdx);
            if (n == 0) {
                logger->Debug(Name() + " remote peer close socket");
                goto error;
            }
            if (n == -1) {
                if (errno == EAGAIN) {
                    return;
                } else {
                    logger->Error(Name() + " read header error: " + Ctos(strerror(errno)));
                    goto error;
                }
            }
            headIdx += n;
        }
        assert(headIdx == 8);
        head[8] = '\0';
        rremain = atoi(head);
        if (rremain == 0) {
            logger->Error(Name() + " read socket rremain zero, buffer: " + Ctos(head));
            goto error;
        }
    }

    char buf[1024];
    while (rremain > 0) {
        int nread = rremain > 1024 ? 1024 : rremain;
        int n = read(fd, buf, nread);
        if (n == 0) {
            logger->Debug(Name() + " remote peer close socket");
            goto error;
        }
        if (n == -1) {
            if (errno == EAGAIN) {
                return;
            } else {
                logger->Error(Name() + " read socket error: " + Ctos(strerror(errno)));
                goto error;
            }
        }
        rremain -= n;
        rbuf.append(buf, n);
    }

    // read ok
    assert(rremain == 0);

    // bufProc();
    wbuf.assign("{\"msg\":\"read ok\"}");

#ifdef HAVE_KQUEUE
    net->DelIOEvent(this);
#endif

    type = EVT_WRITE;
    if (net->ModIOEvent(this) == false) {
        logger->Error(Name() + " ModIOEvent to EVT_WRITE error: " + Ctos(strerror(errno)));
        goto error;
    }

    headIdx = -1;
    rremain = -1;
    rbuf.clear();
    return;

error:
    close(fd);
    fd = -1;
}

void CommonIOEvent::writeProc() {
    if (headIdx != 8) {
        logger->Debug(Name() + " writeProc headIdx: " + Itos(headIdx));
        if (headIdx == -1) {
            snprintf(head, 9, "%08d", static_cast<int>(wbuf.size()));
            head[8] = '\0';
            headIdx = 0;
        }
        logger->Debug(Name() + " writeProc head: " + Ctos(head));
        while (headIdx < 8) {
            int n = write(fd, head + headIdx, 8 - headIdx);
            assert(n != 0);
            if (n == -1) {
                if (errno == EAGAIN) {
                    return;
                } else {
                    logger->Error(Name() + " write socket head error: " + Ctos(strerror(errno)));
                    goto error;
                }
            }
            headIdx += n;
            logger->Debug(Name() + " writeProc [while] headIdx: " + Itos(headIdx));
        }
    }

    while (!wbuf.empty()) {
        logger->Debug(Name() + " write buf: " + wbuf);
        int n = write(fd, wbuf.c_str(), wbuf.size());
        assert(n != 0);
        if (n == -1) {
            if (errno == EAGAIN) {
                return;
            } else {
                logger->Error(Name() + " write socket body error: " + Ctos(strerror(errno)));
                goto error;
            }
        }
        wbuf.erase(0, n);
    }

    logger->Debug(Name() + " write buf ok");
    // write ok
    headIdx = 0;

#ifdef HAVE_KQUEUE
    net->DelIOEvent(this);
#endif

    type = EVT_READ;
    if (net->ModIOEvent(this) == false) {
        logger->Error(Name() + " ModIOEvent to EVT_READ error: " + Ctos(strerror(errno)));
        goto error;
    }
    return;

error:
    close(fd);
    fd = -1;
}

IOEvent *CommonIOEventCreater(int fd, sockaddr *addr, Logger *l) {
    assert(fd >= 0);
    assert(addr != NULL);
    assert(l != NULL);

    string cliAddr(inet_ntoa(reinterpret_cast<sockaddr_in *>(addr)->sin_addr));
    int cliPort(ntohs(reinterpret_cast<sockaddr_in *>(addr)->sin_port));
    return new CommonIOEvent(fd, 1 * 60 * 1000, cliAddr, cliPort, l); // expire: 1min
}

#define EVPOOLSIZE 1024

Net::Net(Logger *l) :
    logger(l)
{
    logger->Debug("New Net Server");
#ifdef HAVE_EPOLL
    evfd = epoll_create(EVPOOLSIZE);
#elif defined HAVE_KQUEUE
    evfd = kqueue();
#else
    #error Need to support epoll or kqueue
#endif
    if (evfd == -1) {
        logger->Crit(strerror(errno));
    }
}

Net::~Net() {
    logger->Debug("Destroy Net Server");
    if (evfd >= 0) {
        close(evfd);
    }
    delete logger;
}

void Net::PushEvent(Event *e) {
    if (e != NULL) {
        this->heap.push(e);
    }
    e->SetNet(this);
}

Event *Net::PopEvent() {
    if (this->heap.empty()) {
        return NULL;
    }
    Event *e = this->heap.top();
    this->heap.pop();
    return e;
}

bool Net::AddIOEvent(IOEvent *e) {
#ifdef HAVE_EPOLL
    struct epoll_event ev;
    ev.data.ptr = e;
    switch (e->Type()) {
        case EVT_READ:
            ev.events = EPOLLIN;
            break;
        case EVT_WRITE:
            ev.events = EPOLLOUT;
            break;
        default:
            logger->Crit("AddIOEvent Illegal event[" + e->Name() + "] type of " + Itos(e->Type()));
    }
    ev.events |= EPOLLET;
    if (epoll_ctl(evfd, EPOLL_CTL_ADD, e->GetFd(), &ev) == -1) {
        logger->Error("Fail to Add event[" + e->Name() + "]: " + Ctos(strerror(errno)));
        return false;
    }
#elif defined HAVE_KQUEUE
    int16_t filter;
    switch (e->Type()) {
        case EVT_READ:
            filter = EVFILT_READ;
            break;
        case EVT_WRITE:
            filter = EVFILT_WRITE;
            break;
        default:
            logger->Crit("AddIOEvent Illegal event[" + e->Name() + "] type of " + Itos(e->Type()));
    }
    struct kevent ev;
    EV_SET(&ev, e->GetFd(), filter, EV_ADD, 0, 0, e);
    if (kevent(evfd, &ev, 1, NULL, 0, NULL) == -1) {
        logger->Error("Fail to Add event[" + e->Name() + "]: " + Ctos(strerror(errno)));
        return false;
    }
#else
    #error Need to support epoll or kqueue
#endif
    PushEvent(e);
    logger->Debug("Add IOEvent: " + e->Name());
    return true;
}

bool Net::ModIOEvent(IOEvent *e) {
#ifdef HAVE_EPOLL
    struct epoll_event ev;
    ev.data.ptr = e;
    switch (e->Type()) {
        case EVT_READ:
            ev.events = EPOLLIN;
            break;
        case EVT_WRITE:
            ev.events = EPOLLOUT;
            break;
        default:
            logger->Crit("ModIOEvent Illegal event[" + e->Name() + "] type of " + Itos(e->Type()));
    }
    ev.events |= EPOLLET;
    if (epoll_ctl(evfd, EPOLL_CTL_MOD, e->GetFd(), &ev) == -1) {
        logger->Error("Fail to Mod event[" + e->Name() + "]: " + Ctos(strerror(errno)));
        return false;
    }
#elif defined HAVE_KQUEUE
    int16_t filter;
    switch (e->Type()) {
        case EVT_READ:
            filter = EVFILT_READ;
            break;
        case EVT_WRITE:
            filter = EVFILT_WRITE;
            break;
        default:
            logger->Crit("ModIOEvent Illegal event[" + e->Name()
                    + "] type of " + Itos(e->Type()));
    }
    struct kevent ev;
    EV_SET(&ev, e->GetFd(), filter, EV_ADD, 0, 0, e);
    if (kevent(evfd, &ev, 1, NULL, 0, NULL) == -1) {
        logger->Error("Fail to Add event[" + e->Name() + "]: " + Ctos(strerror(errno)));
        return false;
    }
#else
    #error Need to support epoll or kqueue
#endif
    return true;
}

bool Net::DelIOEvent(IOEvent *e) {
#ifdef HAVE_EPOLL
    struct epoll_event ev;
    ev.events = 0;
    ev.data.ptr = e;
    if (epoll_ctl(evfd, EPOLL_CTL_DEL, e->GetFd(), &ev) == -1) {
        logger->Error("Fail to Del event[" + e->Name() + "]: " + strerror(errno));
        return false;
    }
#elif defined HAVE_KQUEUE
    struct kevent ev;
    if (e->Type() == EVT_READ) {
        EV_SET(&ev, e->GetFd(), EVFILT_READ, EV_DELETE, 0, 0, NULL);
    } else if (e->Type() == EVT_WRITE) {
        EV_SET(&ev, e->GetFd(), EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    } else {
        logger->Crit("DelIOEvent Illegal event[" + e->Name()
                + "] type of " + Itos(e->Type()));
    }
    if (kevent(evfd, &ev, 1, NULL, 0, NULL) == -1) {
        logger->Error("Fail to Del event[" + e->Name() + "], type: "
                + Itos(e->Type()) + "error: " + Ctos(strerror(errno)));
        return false;
    }
#else
    #error Need to support epoll or kqueue
#endif
    return true;
}

void Net::Start() {
    if (heap.empty()) {
        logger->Crit("No Event Registed in Net Server");
    }
    logger->Debug("Start Net Server");

    sigset_t mask;
    struct sigaction sa;

    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGUSR1);
    if (sigaction(SIGPIPE, &sa, 0) == -1) {
        logger->Crit(strerror(errno));
    }

    running = true;

#ifdef HAVE_EPOLL
    struct epoll_event evs[EVPOOLSIZE];
#elif defined HAVE_KQUEUE
    struct kevent *evs[EVPOOLSIZE];
#else
    #error Need to support epoll or kqueue
#endif


    while (running && !heap.empty()) {
        int milli = heap.top()->ExpireTimestamp() - Now();
        if (milli < 0) {
            milli = 0;
        }
        logger->Debug(">>> heap size: " + Itos(heap.size()) + ", wait: " + Itos(milli));
#ifdef HAVE_EPOLL
        int nevs = epoll_wait(evfd, (struct epoll_event *)evs, EVPOOLSIZE, milli);
#elif defined HAVE_KQUEUE
        struct timespec ts;
        ts.tv_sec = milli / 1000;
        ts.tv_nsec = (milli % 1000) * 1000 * 1000;
        int nevs = kevent(evfd, NULL, 0, (struct kevent *)evs, EVPOOLSIZE, &ts);
#else
    #error Need to support epoll or kqueue
#endif
        if (nevs < 0) {
            logger->Warn("epoll wait error: " + Ctos(strerror(errno)));
        }
        if (nevs > 0) {
            for (int i = 0; i != nevs; ++i) {
#ifdef HAVE_EPOLL
                Event *e = (Event *)evs[i].data.ptr;
                uint32_t ev = evs[i].events;
                if (ev & (EPOLLERR | EPOLLHUP)) {
                    e->Proc(EVT_ERROR);
                } else if (ev & EPOLLIN) {
                    e->Proc(EVT_READ);
                } else if (ev & EPOLLOUT) {
                    e->Proc(EVT_WRITE);
                }
#elif defined HAVE_KQUEUE
                struct kevent *kev = reinterpret_cast<struct kevent *>(&evs[i]);
                Event *e = (Event *)kev->udata;
                switch (kev->filter) {
                    case EVFILT_READ:
                        e->Proc(EVT_READ);
                        break;
                    case EVFILT_WRITE:
                        e->Proc(EVT_WRITE);
                        break;
                    default:
                        if (kev->flags != EV_ERROR) {
                            logger->Error(e->Name() 
                                    + " kevent error, filter:" + Itos(kev->filter)
                                    + " flags: " + Itos(kev->flags));
                        }
                        e->Proc(EVT_ERROR);
                }
#else
    #error Need to support epoll or kqueue
#endif
            }
        }
        int now = Now();
        while (!heap.empty() && now >= heap.top()->ExpireTimestamp()) {
            Event *e = PopEvent();
            e->Proc(EVT_EXPIRE);
            if (e->Active()) {
                e->UpdateTimestamp();
                PushEvent(e);
            } else {
                delete e;
            }
        }
    }

    if (heap.empty()) {
        logger->Warn("event heap empty");
    } else {
        logger->Debug("stop event loop");
    }

    while (!heap.empty()) {
        Event *e = heap.top();
        heap.pop();
        delete e;
    }
    close(evfd);
    evfd = -1;
    logger->Debug("Stop Net Server");
}

void Net::Stop() {
    running = false;
}

};
