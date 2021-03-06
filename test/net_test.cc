#include "cctools/net.h"
#include "cctools/logger.h"
#include "cctools/util.h"

#include <assert.h>
#include <string>

using namespace std;
using namespace cctools;

class CronEvent : public Event {
    public:
        explicit CronEvent(Logger *l) : Event(1000), procCnt(0) {
	    SetLogger(l);
	    type = EVT_EXPIRE;
        }
        virtual string Name() {
            string name("CronJobEvent");
            return name;
        }
        virtual void Proc(EventType type) {
	    if (type == EVT_EXPIRE) {
	        logger->Info("Proc cnt: " + Itos(procCnt++));
	    } else {
		logger->Crit("Unexpected event type of :" + Itos(type));
	    }
	}
        virtual bool Active() { return procCnt < 2; }
    private:
        int procCnt;
};

int main() {
    Logger *l = new Logger("/dev/stderr");
    // l->SetLevel(INFO);
    assert(l != NULL);

    Net service(l);

    IOEvent *lev = new ListenEvent<CommonIOEvent>("127.0.0.1", 6666, l);
    service.AddIOEvent(lev);

    Event *cron = new CronEvent(l);
    service.PushEvent(cron);

    service.Start();

    delete l;
    return 0;
}
