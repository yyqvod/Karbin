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

#include "utils/debug.h"
#include "utils/histogram.h"

#define MAXCRAWLER  20

static void cron (Crawler *pCrawler);

///////////////////////////////////////////////////////////

// wait to limit bandwidth usage
#ifdef MAXBANDWIDTH
static void waitBandwidth (time_t *old, Crawler *pCrawler) {
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

#ifndef NDEBUG
static uint count = 0;
#endif // NDEBUG

///////////////////////////////////////////////////////////
// If this thread terminates, the whole program exits
int main (int argc, char *argv[])
{
    // create all the structures
    Crawler crawler(argc, argv);

    // Start the search
    printf("%s is starting its search\n", crawler.userAgent);
    time_t old = crawler.now;

    for (;;) {
        // update time
        crawler.now = time(NULL);
        if (old != crawler.now) {
            // this block is called every second
            old = crawler.now;
            cron(&crawler);
        }
        waitBandwidth(&old, &crawler);
        for (int i=0; i<crawler.maxFds; i++)
            crawler.ansPoll[i] = 0;
        for (uint i=0; i<crawler.posPoll; i++)
            crawler.ansPoll[crawler.pollfds[i].fd] = crawler.pollfds[i].revents;
        crawler.posPoll = 0;
        sequencer(&crawler);
        fetchDns(&crawler);
        fetchOpen(&crawler);
        checkAll(&crawler);
        // select
        poll(crawler.pollfds, crawler.posPoll, 10);
    }
}

// a lot of stats and profiling things
static void cron (Crawler *pCrawler) {
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
