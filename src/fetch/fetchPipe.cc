// Larbin
// Sebastien Ailleret
// 15-11-99 -> 19-11-01

#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>

#include "options.h"

#include "types.h"
#include "crawler.h"
#include "utils/url.h"
#include "utils/text.h"
#include "utils/string.h"
#include "utils/connexion.h"
#include "fetch/site.h"
#include "fetch/file.h"
#include "interf/output.h"

#include "utils/debug.h"

using namespace std;

static void pipeRead(Connexion *conn, Crawler *pCrawler);
static void pipeWrite(Connexion *conn, Crawler *pCrawler);
static void endOfFile(Connexion *conn, Crawler *pCrawler);


/** Check timeout
 */
void checkTimeout(Crawler *pCrawler)
{
    for (uint i=0; i<pCrawler->nb_conn; i++) {
        Connexion *conn = pCrawler->connexions+i;
        if (conn->state != emptyC) {
            if (conn->timeout > 0) {
                if (conn->timeout > timeoutPage) {
                    conn->timeout = timeoutPage;
                } else {
                    conn->timeout--;
                }
            } else {
                // This server doesn't answer (time out)
                conn->err = timeout;
                endOfFile(conn, pCrawler);
            }
        }
    }
}

/** read all data available
 * fill fd_set for next select
 * give back max fds
 */
void checkAll(Crawler *pCrawler)
{
    // read and write what can be
    for (uint i=0; i<pCrawler->nb_conn; i++) {
        Connexion *conn = pCrawler->connexions+i;
        switch (conn->state) {
            case connectingC:
            case writeC:
                if (pCrawler->ansPoll[conn->socket]) {
                    // trying to finish the connection
                    pipeWrite(conn, pCrawler);
                }
                break;
            case openC:
                if (pCrawler->ansPoll[conn->socket]) {
                    // The socket is open, let's try to read it
                    pipeRead(conn, pCrawler);
                }
                break;
        }
    }

    // update fd_set for the next select
    for (uint i=0; i<pCrawler->nb_conn; i++) {
        int n = (pCrawler->connexions+i)->socket;
        switch ((pCrawler->connexions+i)->state) {
            case connectingC:
            case writeC:
                pCrawler->pollfds[pCrawler->posPoll].fd = n;
                pCrawler->pollfds[pCrawler->posPoll].events = POLLOUT;
                pCrawler->posPoll++;
                break;
            case openC:
                pCrawler->pollfds[pCrawler->posPoll].fd = n;
                pCrawler->pollfds[pCrawler->posPoll].events = POLLIN;
                pCrawler->posPoll++;
                break;
        }
    }
}

/** The socket is finally open !
 * Make sure it's all right, and write the request
 */
static void pipeWrite(Connexion *conn, Crawler *pCrawler)
{
    int res = 0;
    int wrtn, len;
    socklen_t size = sizeof(int);
    switch (conn->state) {
        case connectingC:
            // not connected yet
            getsockopt(conn->socket, SOL_SOCKET, SO_ERROR, &res, &size);
            if (res) {
                // Unable to connect
                conn->err = noConnection;
                endOfFile(conn, pCrawler);
                return;
            }
            // Connection succesfull
            conn->state = writeC;
            // no break
        case writeC:
            // writing the first string
            len = strlen(conn->request.getString());
            wrtn = write(conn->socket, conn->request.getString()+conn->pos,
                    len - conn->pos);
            if (wrtn >= 0) {
                addWrite(wrtn);
#ifdef MAXBANDWIDTH
                pCrawler->remainBand -= wrtn;
#endif // MAXBANDWIDTH
                conn->pos += wrtn;
                if (conn->pos < len) {
                    // Some chars of this string are not written yet
                    return;
                }
            } else {
                if (errno == EAGAIN || errno == EINTR || errno == ENOTCONN) {
                    // little error, come back soon
                    return;
                } else {
                    // unrecoverable error, forget it
                    conn->err = earlyStop;
                    endOfFile(conn, pCrawler);
                    return;
                }
            }
            // All the request has been written
            conn->state = openC;
    }
}

/** Is there something to read on this socket
 * (which is open)
 */
static void pipeRead(Connexion *conn, Crawler *pCrawler)
{
    int p = conn->parser->pos;
    int size = read (conn->socket, conn->buffer+p, maxPageSize-p-1);
    switch (size) {
        case 0:
            // End of file
            //printf("\t\t==> page size %d <==\n", conn->parser->pos);
            if (conn->parser->endInput())
                conn->err = (FetchError) errno;
            endOfFile(conn, pCrawler);
            break;
        case -1:
            switch (errno) {
                case EAGAIN:
                    // Nothing to read now, we'll try again later
                    break;
                default:
                    // Error : let's forget this page
                    conn->err = earlyStop;
                    endOfFile(conn, pCrawler);
                    break;
            }
            break;
        default:
            // Something has been read
            conn->timeout += size / timeoutIncr;
            addRead(size);
#ifdef MAXBANDWIDTH
            pCrawler->remainBand -= size;
#endif // MAXBANDWIDTH
            if (conn->parser->inputHeaders(size) == 0) {
                // nothing special
                if (conn->parser->pos >= maxPageSize-1) {
                    // We've read enough...
                    printf("*** page size %d is to big ! ***\n", conn->parser->pos);
                    conn->err = tooBig;
                    endOfFile(conn, pCrawler);
                }
            } else {
                // The parser does not want any more input (errno explains why)
                conn->err = (FetchError) errno;
                endOfFile(conn, pCrawler);
            }
            break;
    }
}


/* What are we doing when it's over with one file ? */

#ifdef THREAD_OUTPUT
#define manageHtml() pCrawler->userConns->put(conn)
#else // THREAD_OUTPUT
#define manageHtml() \
    endOfLoad((html *)conn->parser, conn->err); \
conn->recycle(); \
pCrawler->freeConns->put(conn)
#endif // THREAD_OUTPUT

static void endOfFile(Connexion *conn, Crawler *pCrawler)
{
    crash("End of file");
    conn->state = emptyC;
    close(conn->socket);
    if (conn->parser->isRobots) {
        // That was a robots.txt
        robots *r = ((robots *) conn->parser);
        r->parse(conn->err != success);
        r->server->robotsResult(conn->err);
        conn->recycle();
        pCrawler->freeConns->put(conn);
    } else {
        // that was an html page
        manageHtml();
    }
}
