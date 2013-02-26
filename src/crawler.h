// Larbin
// Sebastien Ailleret
// 18-11-99 -> 07-03-02

/* This class contains all crawler variables */

#ifndef CRAWLER_H
#define CRAWLER_H

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>

#include "adns.h"

#include "fetch/file.h"
#include "fetch/hashTable.h"
#include "utils/hashDup.h"
#include "utils/url.h"
#include "utils/Vector.h"
#include "utils/string.h"
#include "utils/PersistentFifo.h"
#include "utils/ConstantSizedFifo.h"
#include "utils/SyncFifo.h"
#include "utils/rendezvous.h"
#include "utils/Fifo.h"
#include "fetch/site.h"
#include "fetch/checker.h"

/** This represent a connection : we have a fixed number of them
 * fetchOpen links them with servers
 * fetchPipe reads those which are linked
 */
struct Connexion {
    char state;      // what about this socket : EMPTY, CONNECTING, WRITE, OPEN
    int pos;         // What part of the request has been sent
    FetchError err;  // How did the fetch terminates
    int socket;      // number of the fds
    int timeout;  // timeout for this connexion
    LarbinString request;  // what is the http request
    file *parser;    // parser for the connexion (a robots.txt or an html file)
    char buffer[maxPageSize];
    /** Constructor */
    Connexion ();
    /** Dectructor : it is never used since we reuse connections */
    ~Connexion ();
    /** Recycle a connexion
     */
    void recycle ();
};

struct Crawler {
    /** Constructor : see crawler.cc for details */
    Crawler (int id, Rendezvous *pThreadSync);
    /** Destructor : never used */
    ~Crawler ();
    /** current time : avoid to many calls to time(NULL) */
    time_t now;
    time_t next_call;

    /* used for mirror save */
    char *fileName;
    uint endFileName;
    /* crawler identifier */
    int crawlerId;
    /** List of pages allready seen (one bit per page) */
    hashTable *seen;
#ifdef NO_DUP
    /** Hashtable for suppressing duplicates */
    hashDup *hDuplicate;
#endif // NO_DUP
    /** URLs for the sequencer with high priority */
    SyncFifo<url> *URLsPriority;
    SyncFifo<url> *URLsPriorityWait;
    uint readPriorityWait;
    /** This one has a lower priority : see fetch/sequencer.cc */
    PersistentFifo *URLsDisk;
    PersistentFifo *URLsDiskWait;
    uint readWait;
    /** hashtables of the site we accessed (cache) */
    NamedSite *namedSiteList;
    IPSite *IPSiteList;
    /** Sites which have at least one url to fetch */
    Fifo<IPSite> *okSites;
    /** Sites which have at least one url to fetch
     * but need a dns call
     */
    Fifo<NamedSite> *dnsSites;
    /* Rendezvous object to syns all crawlers */
    Rendezvous *pRend;
    /** Informations for the fetch
     * This array contain all the connections (empty or not)
     */
    Connexion *connexions;
    /** Internal state of adns */
    adns_state ads;
    /* Number of pending dns calls */
    uint nbDnsCalls;
    /** free connection for fetchOpen : connections with state==EMPTY */
    ConstantSizedFifo<Connexion> *freeConns;
#ifdef THREAD_OUTPUT
    /** free connection for fetchOpen : connections waiting for end user */
    ConstantSizedFifo<Connexion> *userConns;
#endif
    /** Sum of the sizes of a fifo in Sites */
    Interval *inter;
    /** How deep should we go inside a site */
    int8_t depthInSite;
    /** Follow external links ? */
    bool externalLinks;
    /** how many seconds should we wait beetween 2 calls at the same server 
     * 0 if you are only on a personnal server, >=30 otherwise
     */
    time_t waitDuration;
    /** Name of the bot */
    char *userAgent;
    /** Name of the man who lauch the bot */
    char *sender;
    /** http headers to send with requests 
     * sends name of the robots, from field...
     */
    char *headers;
    char *headersRobots;  // used when asking a robots.txt
    /* internet address of the proxy (if any) */
    sockaddr_in *proxyAddr;
    /** connect to this server through a proxy using connection conn 
     * return >0 in case of success (connecting or connected), 0 otherwise
     */
    char getProxyFds (Connexion *conn);
    /** Limit to domain */
    Vector<char> *domains;
    /** forbidden extensions
     * extensions which are allways to avoid : .ps, .pdf...
     */
    Vector<char> forbExt;
    /** number of parallel connexions
     * your kernel must support a little more than nb_conn file descriptors
     */
    uint nb_conn;
    /** number of parallel dns calls */
    uint dnsConn;
    /** number of urls in IPSites */
    int IPUrl;
    /** parse configuration file */
    void parseFile (char *file);
    /** read the domain limit */
    void manageDomain (char **posParse);
    /** read the forbidden extensions */
    void manageExt (char **posParse);
    /////////// POLL ///////////////////////////////////
    /** array used by poll */
    struct pollfd *pollfds;
    /** pos of the max used field in pollfds */
    uint posPoll;
    /** size of pollfds */
    uint sizePoll;
    /** array used for dealing with answers */
    short *ansPoll;
    /** number of the biggest file descriptor */
    int maxFds;
    /** make sure the new socket is not too big for ansPoll */
    void verifMax (int fd);
#ifdef MAXBANDWIDTH
    /** number of bits still allowed during this second */
    long int remainBand;
#endif // MAXBANDWIDTH
};

#endif // GLOBAL_H
