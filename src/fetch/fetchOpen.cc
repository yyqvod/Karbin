// Larbin
// Sebastien Ailleret
// 15-11-99 -> 04-12-01

#include <iostream>
#include <errno.h>
#include <sys/types.h>

#include "adns.h"

#include "options.h"

#include "crawler.h"
#include "utils/Fifo.h"
#include "utils/debug.h"
#include "fetch/site.h"

using namespace std;

/* Opens sockets
 * Never block (only opens sockets on already known sites)
 * work inside the main thread
 */
void fetchOpen(Crawler *pCrawler)
{
    pCrawler->next_call = 0;
    if (pCrawler->now < pCrawler->next_call) { // too early to come back
        return;
    }
    int cont = 1;
    while (cont && pCrawler->freeConns->isNonEmpty()) {
        IPSite *s = pCrawler->okSites->tryGet();
        if (s == NULL) {
            cont = 0;
        } else {
            pCrawler->next_call = s->fetch();
            cont = (pCrawler->next_call == 0);
        }
    }
}

/* Opens sockets
 * this function perform dns calls, using adns
 */
void fetchDns(Crawler *pCrawler)
{
    // Submit queries
    while (pCrawler->nbDnsCalls<pCrawler->dnsConn
            && pCrawler->freeConns->isNonEmpty()
            && pCrawler->IPUrl < maxIPUrls) { // try to avoid too many dns calls
        NamedSite *site = pCrawler->dnsSites->tryGet();
        if (site == NULL) {
            break;
        } else {
            site->newQuery();
        }
    }

    // Read available answers
    while (pCrawler->nbDnsCalls && pCrawler->freeConns->isNonEmpty()) {
        NamedSite *site;
        adns_query quer = NULL;
        adns_answer *ans;
        int res = adns_check(pCrawler->ads, &quer, &ans, (void**)&site);
        if (res == ESRCH || res == EAGAIN) {
            // No more query or no more answers
            break;
        }
        pCrawler->nbDnsCalls--;
        site->dnsAns(ans);
        free(ans); // ans has been allocated with malloc
    }
}
