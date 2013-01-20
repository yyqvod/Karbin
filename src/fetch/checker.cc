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
void check (url *u) {
    if (crawler::seen->testSet(u)) {
        hashUrls();  // stat
        // where should this link go ?
#ifdef SPECIFICSEARCH
        if (privilegedExts[0] != NULL
                && matchPrivExt(u->getFile())) {
            interestingExtension();
            crawler::URLsPriority->put(u);
        } else {
            crawler::URLsDisk->put(u);
        }
#else // not a SPECIFICSEARCH
        printf("URLsDisk->put %s:%d/%s %d\n", u->getHost(),u->getPort(),u->getFile(),u->getDepth());
        crawler::URLsDisk->put(u);
#endif
    } else {
        // This url has already been seen
        answers(urlDup);
        delete u;
    }
}

/** Check the extension of an url
 * @return true if it might be interesting, false otherwise
 */
bool filter1 (char *host, char *file) {
    int i=0;
    if (crawler::domains != NULL) {
        bool ok = false;
        while ((*crawler::domains)[i] != NULL) {
            ok = ok || endWith((*crawler::domains)[i], host);
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
    while (crawler::forbExt[i] != NULL) {
        if (endWithIgnoreCase(crawler::forbExt[i], file, l)) {
            return false;
        }
        i++;
    }
    return true;
}
