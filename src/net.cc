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
#include <sys/epoll.h>

#include <string>

using namespace std;

namespace cctools {

ListenEvent::ListenEvent(string ip, int port, IOEventCreater func, Logger *l) :
    IOEvent(), listenAddr(ip), listenPort(port), ioevCreater(func)
{
    type = EV_READ;
    SetLogger(l);
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        logger->Crit("fail to create socket: " + Ctos(strerror(errno)));
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
    int sock = accept(fd, reinterpret_cast<sockaddr *>(&remoteAddr), &socklen);
    if (sock == -1) {
        logger->Error(Name() + " accept error: " + Ctos(strerror(errno)));
        return;
    }
    IOEvent *e = ioevCreater(sock, reinterpret_cast<sockaddr *>(&remoteAddr), logger);
    if (e == NULL) {
        logger->Error(Name() + "\'s IOEventCreater create NULL event");
        close(sock);
        return;
    }
    if (net->AddIOEvent(e) == false) {
        delete e;
    }
}

void ListenEvent::Proc(EventType type) {
    switch (type) {
        case EV_ERROR:
            logger->Error(Name() + " error, close socket " + Itos(fd));
            if (fd >= 0) {
                close(fd);
                fd = -1;
            }
            break;
        case EV_READ:
            acceptProc();
            break;
        case EV_WRITE:
            logger->Crit(Name() + " write event cannot be happened on listen event");
            break;
        case EV_EXPIRE:
            logger->Info(Name() + " expire event happened");
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
    type = EV_READ;
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
        case EV_ERROR:
            if (fd >= 0) {
                close(fd);
                fd = -1;
                logger->Info(Name() + " error, close socket");
            }
            break;
        case EV_EXPIRE:
            if (fd >= 0) {
                close(fd);
                fd = -1;
            }
            logger->Info(Name() + " expired, close socket");
            break;
        case EV_READ:
            readProc();
            break;
        case EV_WRITE:
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
    logger->Info(">>>>>>>>>> " + rbuf);
    wbuf.assign("{\"msg\":\"read ok\"}");
    type = EV_WRITE;
    net->ModIOEvent(this);

    headIdx = -1;
    rremain = -1;
    rbuf.clear();
    return;

error:
    close(fd);
    fd = -1;
}

void CommonIOEvent::writeProc() {
    while (!wbuf.empty()) {
        int n = write(fd, wbuf.c_str(), wbuf.size());
        if (n == -1) {
            if (errno == EAGAIN) {
                return;
            } else {
                logger->Error(Name() + " write socket error: " + Ctos(strerror(errno)));
                goto error;
            }
        }
        wbuf.erase(0, n);
    }

    // write ok
    headIdx = 0;
    type = EV_READ;
    net->ModIOEvent(this);
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
    return new CommonIOEvent(fd, 5 * 60 * 1000, cliAddr, cliPort, l); // expire: 5min
}

#define EPSIZE 1024

Net::Net(Logger *l) :
    logger(l)
{
    logger->Debug("New Net Server");
    epfd = epoll_create(EPSIZE);
    if (epfd == -1) {
        logger->Crit(strerror(errno));
    }
}

Net::~Net() {
    logger->Debug("Destroy Net Server");
    if (epfd >= 0) {
        close(epfd);
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
    struct epoll_event ev;
    ev.data.ptr = e;
    switch (e->Type()) {
        case EV_READ:
            ev.events = EPOLLIN;
            break;
        case EV_WRITE:
            ev.events = EPOLLOUT;
            break;
        default:
            logger->Crit("Illegal event[" + e->Name() + "] type of " + Itos(e->Type()));
    }
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, e->GetFd(), &ev) == -1) {
        logger->Error("Fail to Add event[" + e->Name() + "]: " + Ctos(strerror(errno)));
        return false;
    }
    PushEvent(e);
    logger->Debug("Add IOEvent: " + e->Name());
    return true;
}

bool Net::ModIOEvent(IOEvent *e) {
    struct epoll_event ev;
    ev.data.ptr = e;
    switch (e->Type()) {
        case EV_READ:
            ev.events = EPOLLIN;
            break;
        case EV_WRITE:
            ev.events = EPOLLOUT;
            break;
        default:
            logger->Crit("Illegal event[" + e->Name() + "] type of " + Itos(e->Type()));
    }
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, e->GetFd(), &ev) == -1) {
        logger->Error("Fail to Mod event[" + e->Name() + "]: " + strerror(errno));
        return false;
    }
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
    struct epoll_event evs[EPSIZE];

    while (running && !heap.empty()) {
        int milli = heap.top()->ExpireTimestamp() - Now();
        if (milli < 0) {
            milli = 0;
        }
        int nevs = epoll_wait(epfd, (struct epoll_event *)evs, EPSIZE, milli);
        if (nevs < 0) {
            logger->Warn("epoll wait error: " + Ctos(strerror(errno)));
        }
        if (nevs > 0) {
            for (int i = 0; i != nevs; ++i) {
                Event *e = (Event *)evs[i].data.ptr;
                uint32_t ev = evs[i].events;
                if (ev & (EPOLLERR | EPOLLHUP)) {
                    e->Proc(EV_ERROR);
                } else if (ev & EPOLLIN) {
                    e->Proc(EV_READ);
                } else if (ev & EPOLLOUT) {
                    e->Proc(EV_WRITE);
                }
            }
        }
        int now = Now();
        while (!heap.empty() && now >= heap.top()->ExpireTimestamp()) {
            Event *e = PopEvent();
            e->Proc(EV_EXPIRE);
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
        logger->Info("stop event loop");
    }

    while (!heap.empty()) {
        Event *e = heap.top();
        heap.pop();
        delete e;
    }
    close(epfd);
    epfd = -1;
    logger->Debug("Stop Net Server");
}

void Net::Stop() {
    running = false;
}

};
