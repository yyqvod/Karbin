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

#include "global.h"
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
  while (global::remainBand < 0) {
    poll(NULL, 0, 10);
    global::now = time(NULL);
    if (*old != global::now) {
      *old = global::now;
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
  global glob(argc, argv);

  // Start the search
  printf("%s is starting its search\n", global::userAgent);
  time_t old = global::now;

  for (;;) {
    // update time
    global::now = time(NULL);
    if (old != global::now) {
      // this block is called every second
      old = global::now;
      cron();
    }
    stateMain(-count);
    waitBandwidth(&old);
    stateMain(1);
    for (int i=0; i<global::maxFds; i++)
      global::ansPoll[i] = 0;
    for (uint i=0; i<global::posPoll; i++)
      global::ansPoll[global::pollfds[i].fd] = global::pollfds[i].revents;
    global::posPoll = 0;
    sequencer();
    fetchDns();
    fetchOpen();
    checkAll();
    // select
    poll(global::pollfds, global::posPoll, 10);
  }
}

// a lot of stats and profiling things
static void cron () {
  // shall we stop
#ifdef EXIT_AT_END
  if (global::URLsDisk->getLength() == 0
      && global::URLsDiskWait->getLength() == 0
      && debUrl == 0)
    exit(0);
#endif // EXIT_AT_END

  // look for timeouts
  checkTimeout();
  // see if we should read again urls in fifowait
  if ((global::now % 300) == 0) {
    global::readPriorityWait = global::URLsPriorityWait->getLength();
    global::readWait = global::URLsDiskWait->getLength();
  }
  if ((global::now % 300) == 150) {
    global::readPriorityWait = 0;
    global::readWait = 0;
  }

#ifdef MAXBANDWIDTH
  // give bandwidth
  if (global::remainBand > 0) {
    global::remainBand = MAXBANDWIDTH;
  } else {
    global::remainBand = global::remainBand + MAXBANDWIDTH;
  }
#endif // MAXBANDWIDTH
}
