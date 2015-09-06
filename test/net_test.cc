#include "cctools/net.h"
#include "cctools/logger.h"
#include "cctools/util.h"

#include <assert.h>
#include <string>

using namespace std;
using namespace cctools;

class CronEvent : public Event {
    public:
        explicit CronEvent(Logger *l) : Event(1000), logger(l), procCnt(0) {}
        virtual string Name() {
            string name("CronJobEvent");
            return name;
        }
        virtual void Proc() { logger->Info("Proc cnt: " + Itos(procCnt++)); }
        virtual bool Active() { return procCnt < 10; }
    private:
        int procCnt;
        Logger *logger;
};

int main() {
    Logger *l = new Logger("/dev/stderr");
    assert(l != NULL);

    Net service(l);
    IOEvent *lev = new ListenEvent("127.0.0.1", "6666", CommonIOEventCreater, l);
    Event *cron = new CronEvent(l);

    service.AddIOEvent(lev);
    service.PushEvent(cron);

    service.Start();

    delete l;
    return 0;
}
