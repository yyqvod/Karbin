// Larbin
// Sebastien Ailleret
// 09-11-99 -> 09-03-02

#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "options.h"

#include "crawler.h"
#include "utils/text.h"
#include "fetch/checker.h"
#include "fetch/sequencer.h"
#include "fetch/fetchOpen.h"
#include "fetch/fetchPipe.h"
#include "interf/output.h"
#include "utils/mypthread.h"
#include "utils/rendezvous.h"

#include "utils/debug.h"

int nrCrawler = 3; //crawler numbers
int map[nrVNode]; //Map: Virtual-Node --> Crawler
Crawler *crawlers[nrVNode]; //record all crawlers

static void cron(Crawler *pCrawler);
static void *startCrawler(void *pData);
static void mainThread();

#define stateMain(i) (printf("Craw(%d): stateMain  %d\n", pCrawler->crawlerId, i))
//#define stateMain(i) ((void) 0)

#ifdef MAXBANDWIDTH
static void waitBandwidth(time_t *old, Crawler *pCrawler);
#else
#define waitBandwidth(x, y) ((void) 0)
#endif // MAXBANDWIDTH

// main thread: control crawler threads
int main(int argc, char *argv[])
{
    int i, cNodes;
    Rendezvous *pRend = new Rendezvous(nrCrawler);

    cNodes = nrVNode / nrCrawler;
    for (i = 0; i < nrVNode; i++) {
        map[i] = i / cNodes;
        //printf("map[%d] = %d\n", i, map[i]);
    }

    for (i = 0; i < nrCrawler; i++) {
        // create all the structures
        crawlers[i] = new Crawler(i, pRend);
        startThread(startCrawler, crawlers[i]);
        printf("Start %d\n", i);
        sleep(5);
    }

    for (i = nrCrawler; i < nrVNode; i++)
        crawlers[i] = NULL;

    mainThread();
}

static void mainThread()
{
    while (1) {
        sleep(5);
        printf("In main thread\n");
    }
}

static void *startCrawler(void *pData)
{
    int count = 0;
    Crawler *pCrawler = (Crawler *)pData;
    Rendezvous *pRend = pCrawler->pRend;

    //Wait for all crawlers to get ready to Start the search
    pRend->meet();
    printf("Crawler NO.%d starts searching\n", pCrawler->crawlerId);
    time_t old = pCrawler->now;

    for (;;) {
        // update time
        pCrawler->now = time(NULL);
        if (old != pCrawler->now) {
            // this block is called every second
            old = pCrawler->now;
            cron(pCrawler);
        }
        stateMain(-count);
        waitBandwidth(&old, pCrawler);
        stateMain(1);
        for (int i=0; i<pCrawler->maxFds; i++)
            pCrawler->ansPoll[i] = 0;
        for (uint i=0; i<pCrawler->posPoll; i++)
            pCrawler->ansPoll[pCrawler->pollfds[i].fd] = pCrawler->pollfds[i].revents;
        pCrawler->posPoll = 0;
        stateMain(2);
        sequencer(pCrawler);
        stateMain(3);
        fetchDns(pCrawler);
        stateMain(4);
        fetchOpen(pCrawler);
        stateMain(5);
        checkAll(pCrawler);
        // select
        stateMain(count++);
        poll(pCrawler->pollfds, pCrawler->posPoll, 10);
        stateMain(6);
    }

    return NULL;
}

// a lot of stats and profiling things
static void cron(Crawler *pCrawler)
{
    // shall we stop
#ifdef EXIT_AT_END
    if (pCrawler->URLsDisk->getLength() == 0
            && pCrawler->URLsDiskWait->getLength() == 0
            && debUrl == 0)
        exit(0);
#endif // EXIT_AT_END

    // look for timeouts
    checkTimeout(pCrawler);
    // see if we should read again urls in fifowait
    if ((pCrawler->now % 300) == 0) {
        pCrawler->readPriorityWait = pCrawler->URLsPriorityWait->getLength();
        pCrawler->readWait = pCrawler->URLsDiskWait->getLength();
    }
    if ((pCrawler->now % 300) == 150) {
        pCrawler->readPriorityWait = 0;
        pCrawler->readWait = 0;
    }

#ifdef MAXBANDWIDTH
    // give bandwidth
    if (pCrawler->remainBand > 0) {
        pCrawler->remainBand = MAXBANDWIDTH;
    } else {
        pCrawler->remainBand = pCrawler->remainBand + MAXBANDWIDTH;
    }
#endif // MAXBANDWIDTH
}

#ifdef MAXBANDWIDTH
// wait to limit bandwidth usage
static void waitBandwidth(time_t *old, Crawler *pCrawler)
{
    while (pCrawler->remainBand < 0) {
        poll(NULL, 0, 10);
        pCrawler->now = time(NULL);
        if (*old != pCrawler->now) {
            *old = pCrawler->now;
            cron(pCrawler);
        }
    }
}
#endif
