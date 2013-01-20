// Larbin
// Sebastien Ailleret
// 15-11-99 -> 04-01-02

#include <iostream>

#include "options.h"

#include "crawler.h"
#include "types.h"
#include "utils/url.h"
#include "utils/debug.h"
#include "fetch/site.h"

using namespace std;

static bool canGetUrl (bool *testPriority);
uint space = 0;

#define maxPerCall 100

/** start the sequencer
 */
void sequencer () {
    bool testPriority = true;
    if (space == 0) {
        space = crawler::inter->putAll();
    }
    int still = space;
    if (still > maxPerCall) still = maxPerCall;
    while (still) {
        if (canGetUrl(&testPriority)) {
            space--; still--;
        } else {
            still = 0;
        }
    }
}

/* Get the next url
 * here is defined how priorities are handled
 */
static bool canGetUrl (bool *testPriority) {
    url *u;

    if (crawler::readPriorityWait) {
        crawler::readPriorityWait--;
        u = crawler::URLsPriorityWait->get();
        crawler::namedSiteList[u->hostHashCode()].putPriorityUrlWait(u);
        return true;
    } else if (*testPriority && (u=crawler::URLsPriority->tryGet()) != NULL) {
        // We've got one url (priority)
        crawler::namedSiteList[u->hostHashCode()].putPriorityUrl(u);
        return true;
    } else {
        *testPriority = false;
        // Try to get an ordinary url
        if (crawler::readWait) {
            crawler::readWait--;
            u = crawler::URLsDiskWait->get();
            crawler::namedSiteList[u->hostHashCode()].putUrlWait(u);
            return true;
        } else {
            u = crawler::URLsDisk->tryGet();
            if (u != NULL) {
                crawler::namedSiteList[u->hostHashCode()].putUrl(u);
                return true;
            } else {
                return false;
            }
        }
    }
}
