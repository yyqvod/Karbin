// Larbin
// Sebastien Ailleret
// 15-11-99 -> 04-12-01

#include <iostream>
#include <errno.h>
#include <sys/types.h>

#include <adns.h>

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
void fetchOpen () {
    static time_t next_call = 0;
    if (crawler::now < next_call) { // too early to come back
        return;
    }
    int cont = 1;
    while (cont && crawler::freeConns->isNonEmpty()) {
        IPSite *s = crawler::okSites->tryGet();
        if (s == NULL) {
            cont = 0;
        } else {
            next_call = s->fetch();
            cont = (next_call == 0);
        }
    }
}

/* Opens sockets
 * this function perform dns calls, using adns
 */
void fetchDns () {
    // Submit queries
    while (crawler::nbDnsCalls<crawler::dnsConn
            && crawler::freeConns->isNonEmpty()
            && crawler::IPUrl < maxIPUrls) { // try to avoid too many dns calls
        NamedSite *site = crawler::dnsSites->tryGet();
        if (site == NULL) {
            break;
        } else {
            site->newQuery();
        }
    }

    // Read available answers
    while (crawler::nbDnsCalls && crawler::freeConns->isNonEmpty()) {
        NamedSite *site;
        adns_query quer = NULL;
        adns_answer *ans;
        int res = adns_check(crawler::ads, &quer, &ans, (void**)&site);
        if (res == ESRCH || res == EAGAIN) {
            // No more query or no more answers
            break;
        }
        crawler::nbDnsCalls--;
        site->dnsAns(ans);
        free(ans); // ans has been allocated with malloc
    }
}
