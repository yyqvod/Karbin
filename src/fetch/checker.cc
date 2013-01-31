// Larbin
// Sebastien Ailleret
// 15-11-99 -> 09-12-01

/* This modules is a filter
 * given some normalized URLs, it makes sure their extensions are OK
 * and send them if it didn't see them before
 */

#include <iostream>
#include <string.h>

#include "options.h"

#include "types.h"
#include "crawler.h"
#include "utils/url.h"
#include "utils/text.h"
#include "utils/Vector.h"
#include "fetch/hashTable.h"
#include "fetch/file.h"

#include "utils/debug.h"

using namespace std;

/** check if an url is allready known
 * if not send it
 * @param u the url to check
 */
void check(url *u, Crawler *pCrawler)
{
    if (pCrawler->seen->testSet(u)) {
        int index = u->hashCode() % nrVNode;
        hashUrls();  // stat
        /* printf("URLsDisk->put %s:%d/%s %d\n", u->getHost(), u->getPort(),\
           u->getFile(),u->getDepth()); */
        // where should this link go ?
        if (map[index] == pCrawler->crawlerId) {
            //printf("crawler(%d): url(%d) map(%d)\n", pCrawler->crawlerId, index, map[index]);
            pCrawler->URLsDisk->put(u);
        } else {
            //printf("crawler(%d): url(%d) map(%d)\n", pCrawler->crawlerId, index, map[index]);
//            pCrawler = crawlers[map[index]];
            if (pCrawler)
                pCrawler->URLsDisk->put(u);
            else {
                delete u;
                cerr<<"** ERR: Wrong Crawler pointer ! **"<<endl;
            }
        }
    } else {
        // This url has already been seen
        answers(urlDup);
        delete u;
    }
}

/** Check the extension of an url
 * @return true if it might be interesting, false otherwise
 */
bool filter1(char *host, char *file, Crawler *pCrawler)
{
    int i=0;
    if (pCrawler->domains != NULL) {
        bool ok = false;
        while ((*pCrawler->domains)[i] != NULL) {
            ok = ok || endWith((*pCrawler->domains)[i], host);
            i++;
        }
        if (!ok) {
            return false;
        }
    }
    i=0;
    int l = strlen(file);
    if (endWithIgnoreCase("html", file, l)
            || file[l-1] == '/'
            || endWithIgnoreCase("htm", file, l)) {
        return true;
    }
    while (pCrawler->forbExt[i] != NULL) {
        if (endWithIgnoreCase(pCrawler->forbExt[i], file, l)) {
            return false;
        }
        i++;
    }
    return true;
}
