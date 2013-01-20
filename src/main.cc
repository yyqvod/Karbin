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

static void cron ();

///////////////////////////////////////////////////////////

// wait to limit bandwidth usage
#ifdef MAXBANDWIDTH
static void waitBandwidth (time_t *old) {
    while (crawler::remainBand < 0) {
        poll(NULL, 0, 10);
        crawler::now = time(NULL);
        if (*old != crawler::now) {
            *old = crawler::now;
            cron ();
        }
    }
}
#else
#define waitBandwidth(x) ((void) 0)
#endif // MAXBANDWIDTH

#ifndef NDEBUG
static uint count = 0;
#endif // NDEBUG

///////////////////////////////////////////////////////////
// If this thread terminates, the whole program exits
int main (int argc, char *argv[]) {
    // create all the structures
    crawler glob(argc, argv);

    // Start the search
    printf("%s is starting its search\n", crawler::userAgent);
    time_t old = crawler::now;

    for (;;) {
        // update time
        crawler::now = time(NULL);
        if (old != crawler::now) {
            // this block is called every second
            old = crawler::now;
            cron();
        }
        waitBandwidth(&old);
        for (int i=0; i<crawler::maxFds; i++)
            crawler::ansPoll[i] = 0;
        for (uint i=0; i<crawler::posPoll; i++)
            crawler::ansPoll[crawler::pollfds[i].fd] = crawler::pollfds[i].revents;
        crawler::posPoll = 0;
        sequencer();
        fetchDns();
        fetchOpen();
        checkAll();
        // select
        poll(crawler::pollfds, crawler::posPoll, 10);
    }
}

// a lot of stats and profiling things
static void cron () {
    // shall we stop
#ifdef EXIT_AT_END
    if (crawler::URLsDisk->getLength() == 0
            && crawler::URLsDiskWait->getLength() == 0
            && debUrl == 0)
        exit(0);
#endif // EXIT_AT_END

    // look for timeouts
    checkTimeout();
    // see if we should read again urls in fifowait
    if ((crawler::now % 300) == 0) {
        crawler::readPriorityWait = crawler::URLsPriorityWait->getLength();
        crawler::readWait = crawler::URLsDiskWait->getLength();
    }
    if ((crawler::now % 300) == 150) {
        crawler::readPriorityWait = 0;
        crawler::readWait = 0;
    }

#ifdef MAXBANDWIDTH
    // give bandwidth
    if (crawler::remainBand > 0) {
        crawler::remainBand = MAXBANDWIDTH;
    } else {
        crawler::remainBand = crawler::remainBand + MAXBANDWIDTH;
    }
#endif // MAXBANDWIDTH
}
