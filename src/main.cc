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

#define CRAWLERNUM 1

static void cron(Crawler *pCrawler);
static void *startCrawler(void *pData);

///////////////////////////////////////////////////////////

// wait to limit bandwidth usage
#ifdef MAXBANDWIDTH
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
#else
#define waitBandwidth(x, y) ((void) 0)
#endif // MAXBANDWIDTH

///////////////////////////////////////////////////////////
// If this thread terminates, the whole program exits
int main(int argc, char *argv[])
{
    int i;
    Rendezvous *pRend = new Rendezvous(CRAWLERNUM);

    for (i = 0; i < CRAWLERNUM; i++) {
        // create all the structures
        Crawler *pCrawler = new Crawler(i+1, pRend);
        startThread(startCrawler, pCrawler);
        printf("Start %d\n", i+1);
        sleep(5);
    }

    pthread_exit(NULL);
}

static void *startCrawler(void *pData)
{
    Crawler *pCrawler = (Crawler *)pData;
    Rendezvous *pRend = pCrawler->pRend;

    //Wait for all crawlers to get ready to Start the search
    pRend->meet();
    printf("Crawler NO.%d is starting its search\n", pCrawler->crawlerId);
    time_t old = pCrawler->now;

    for (;;) {
        // update time
        pCrawler->now = time(NULL);
        if (old != pCrawler->now) {
            // this block is called every second
            old = pCrawler->now;
            cron(pCrawler);
        }
        waitBandwidth(&old, pCrawler);
        for (int i=0; i<pCrawler->maxFds; i++)
            pCrawler->ansPoll[i] = 0;
        for (uint i=0; i<pCrawler->posPoll; i++)
            pCrawler->ansPoll[pCrawler->pollfds[i].fd] = pCrawler->pollfds[i].revents;
        pCrawler->posPoll = 0;
        sequencer(pCrawler);
        fetchDns(pCrawler);
        fetchOpen(pCrawler);
        checkAll(pCrawler);
        // select
        poll(pCrawler->pollfds, pCrawler->posPoll, 10);
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
