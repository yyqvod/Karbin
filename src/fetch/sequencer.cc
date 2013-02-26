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

static bool canGetUrl(bool *testPriority, Crawler *pCrawler);
uint space = 0;

#define maxPerCall 100

/** start the sequencer
 */
void sequencer(Crawler *pCrawler)
{
    bool testPriority = true;
    if (space == 0) {
        space = pCrawler->inter->putAll();
    }
    int still = space;
    if (still > maxPerCall) still = maxPerCall;
    while (still) {
        if (canGetUrl(&testPriority, pCrawler)) {
            space--; still--;
        } else {
            still = 0;
        }
    }
}

/* Get the next url
 * here is defined how priorities are handled
 */
static bool canGetUrl(bool *testPriority, Crawler *pCrawler)
{
    url *u;

    if (pCrawler->readPriorityWait) {
        pCrawler->readPriorityWait--;
        u = pCrawler->URLsPriorityWait->get();
        pCrawler->namedSiteList[u->hostHashCode()].putPriorityUrlWait(u);
        return true;
    } else if (*testPriority && (u=pCrawler->URLsPriority->tryGet()) != NULL) {
        // We've got one url (priority)
        pCrawler->namedSiteList[u->hostHashCode()].putPriorityUrl(u);
        return true;
    } else {
        *testPriority = false;
        // Try to get an ordinary url
        if (pCrawler->readWait) {
            pCrawler->readWait--;
            printf("Crwaler[%d]: DiskWait get\n", pCrawler->crawlerId);
            u = pCrawler->URLsDiskWait->get();
            printf("Crwaler[%d]: Got %s\n", pCrawler->crawlerId, u->getHost());
            pCrawler->namedSiteList[u->hostHashCode()].putUrlWait(u);
            return true;
        } else {
            printf("Crwaler[%d]: Disk tryGet\n", pCrawler->crawlerId);
            u = pCrawler->URLsDisk->tryGet();
            if (u != NULL) {
                printf("Crwaler[%d]: Got %s\n", pCrawler->crawlerId, u->getHost());
                pCrawler->namedSiteList[u->hostHashCode()].putUrl(u);
                return true;
            } else {
                printf("Crwaler[%d]: Got NULL\n", pCrawler->crawlerId);
                return false;
            }
        }
    }
}
