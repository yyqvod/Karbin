// Larbin
// Sebastien Ailleret
// 03-02-00 -> 10-12-01

#include <iostream>
#include <string.h>
#include <unistd.h>

#include "options.h"

#include "types.h"
#include "crawler.h"
#include "fetch/file.h"
#include "utils/text.h"
#include "utils/debug.h"
#include "interf/useroutput.h"
#include "utils/mypthread.h"


using namespace std;

/** The fetch failed
 * @param u the URL of the doc
 * @param reason reason of the fail
 */
void fetchFail(url *u, FetchError err, bool interesting=false)
{
#ifdef SPECIFICSEARCH
    if (interesting
            || (privilegedExts[0] != NULL && matchPrivExt(u->getFile()))) {
        failure(u, err);
    }
#else // not a SPECIFICSEARCH
    failure(u, err);
#endif
}

/** It's over with this file
 * report the situation ! (and make some stats)
 */
void endOfLoad(html *parser, FetchError err)
{
    answers(err);
    switch (err) {
        case success:
#ifdef SPECIFICSEARCH
            if (parser->isInteresting) {
                interestingPage();
                loaded(parser);
            }
#else // not a SPECIFICSEARCH
            loaded(parser);
#endif // SPECIFICSEARCH
            break;
        default:
            fetchFail(parser->getUrl(), err, parser->isInteresting);
            break;
    }
}

void initOutput(int num)
{
    initUserOutput(num);
}
