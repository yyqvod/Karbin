// Larbin
// Sebastien Ailleret
// 29-11-99 -> 09-03-02

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include "adns.h"
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#include "options.h"

#include "types.h"
#include "crawler.h"
#include "utils/text.h"
#include "utils/Fifo.h"
#include "utils/debug.h"
#include "fetch/site.h"
#include "interf/output.h"


using namespace std;

///////////////////////////////////////////////////////////
// Struct crawler
///////////////////////////////////////////////////////////

// define all the static variables
time_t crawler::now;
hashTable *crawler::seen;
#ifdef NO_DUP
hashDup *crawler::hDuplicate;
#endif // NO_DUP
SyncFifo<url> *crawler::URLsPriority;
SyncFifo<url> *crawler::URLsPriorityWait;
uint crawler::readPriorityWait=0;
PersistentFifo *crawler::URLsDisk;
PersistentFifo *crawler::URLsDiskWait;
uint crawler::readWait=0;
IPSite *crawler::IPSiteList;
NamedSite *crawler::namedSiteList;
Fifo<IPSite> *crawler::okSites;
Fifo<NamedSite> *crawler::dnsSites;
Connexion *crawler::connexions;
adns_state crawler::ads;
uint crawler::nbDnsCalls = 0;
ConstantSizedFifo<Connexion> *crawler::freeConns;
#ifdef THREAD_OUTPUT
ConstantSizedFifo<Connexion> *crawler::userConns;
#endif
Interval *crawler::inter;
int8_t crawler::depthInSite;
bool crawler::externalLinks = true;
time_t crawler::waitDuration;
char *crawler::userAgent;
char *crawler::sender;
char *crawler::headers;
char *crawler::headersRobots;
sockaddr_in *crawler::proxyAddr;
Vector<char> *crawler::domains;
Vector<char> crawler::forbExt;
uint crawler::nb_conn;
uint crawler::dnsConn;
struct pollfd *crawler::pollfds;
uint crawler::posPoll;
uint crawler::sizePoll;
short *crawler::ansPoll;
int crawler::maxFds;
#ifdef MAXBANDWIDTH
long int crawler::remainBand = MAXBANDWIDTH;
#endif // MAXBANDWIDTH
int crawler::IPUrl = 0;

/** Constructor : initialize almost everything
 * Everything is read from the config file (larbin.conf by default)
 */
crawler::crawler (int argc, char *argv[]) {
    char *configFile = (char *)"larbin.conf";
#ifdef RELOAD
    bool reload = true;
#else
    bool reload = false;
#endif
    now = time(NULL);
    // verification of arguments
    int pos = 1;
    while (pos < argc) {
        if (!strcmp(argv[pos], "-c") && argc > pos+1) {
            configFile = argv[pos+1];
            pos += 2;
        } else if (!strcmp(argv[pos], "-scratch")) {
            reload = false;
            pos++;
        } else {
            break;
        }
    }
    if (pos != argc) {
        cerr << "usage : " << argv[0];
        cerr << " [-c configFile] [-scratch]\n";
        exit(1);
    }

    // Standard values
    waitDuration = 60;
    depthInSite = 5;
    userAgent = (char *)"larbin";
    sender = (char *)"larbin@unspecified.mail";
    nb_conn = 20;
    dnsConn = 3;
    proxyAddr = NULL;
    domains = NULL;
    // FIFOs
    URLsDisk = new PersistentFifo(reload, fifoFile);
    URLsDiskWait = new PersistentFifo(reload, fifoFileWait);
    URLsPriority = new SyncFifo<url>;
    URLsPriorityWait = new SyncFifo<url>;
    inter = new Interval(ramUrls);
    namedSiteList = new NamedSite[namedSiteListSize];
    IPSiteList = new IPSite[IPSiteListSize];
    okSites = new Fifo<IPSite>(2000);
    dnsSites = new Fifo<NamedSite>(2000);
    seen = new hashTable(!reload);
#ifdef NO_DUP
    hDuplicate = new hashDup(dupSize, dupFile, !reload);
#endif // NO_DUP
    // Read the configuration file
    crash("Read the configuration file");
    parseFile(configFile);
    // Initialize everything
    crash("Create crawler values");
    // Headers
    LarbinString strtmp;
    strtmp.addString("\r\nUser-Agent: ");
    strtmp.addString(userAgent);
    strtmp.addString(" ");
    strtmp.addString(sender);
#ifdef SPECIFICSEARCH
    strtmp.addString("\r\nAccept: text/html");
    int i=0;
    while (contentTypes[i] != NULL) {
        strtmp.addString(", ");
        strtmp.addString(contentTypes[i]);
        i++;
    }
#elif !defined(IMAGES) && !defined(ANYTYPE)
    strtmp.addString("\r\nAccept: text/html");
#endif // SPECIFICSEARCH
    strtmp.addString("\r\n\r\n");
    headers = strtmp.giveString();
    // Headers robots.txt
    strtmp.recycle();
    strtmp.addString("\r\nUser-Agent: ");
    strtmp.addString(userAgent);
    strtmp.addString(" (");
    strtmp.addString(sender);
    strtmp.addString(")\r\n\r\n");
    headersRobots = strtmp.giveString();
#ifdef THREAD_OUTPUT
    userConns = new ConstantSizedFifo<Connexion>(nb_conn);
#endif
    freeConns = new ConstantSizedFifo<Connexion>(nb_conn);
    connexions = new Connexion [nb_conn];
    for (uint i=0; i<nb_conn; i++) {
        freeConns->put(connexions+i);
    }
    // init poll structures
    sizePoll = nb_conn + maxInput;
    pollfds = new struct pollfd[sizePoll];
    posPoll = 0;
    maxFds = sizePoll;
    ansPoll = new short[maxFds];
    // init non blocking dns calls
    adns_initflags flags =
        adns_initflags (adns_if_nosigpipe | adns_if_debug); //adns_if_noerrprint
    adns_init(&ads, flags, NULL);
    // call init functions of all modules
    initSpecific();
    initOutput();
    initSite();
    // let's ignore SIGPIPE
    static struct sigaction sn, so;
    sigemptyset(&sn.sa_mask);
    sn.sa_flags = SA_RESTART;
    sn.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sn, &so)) {
        cerr << "Unable to disable SIGPIPE : " << strerror(errno) << endl;
    }
}

/** Destructor : never used because the program should never end !
 */
crawler::~crawler () {
    assert(false);
}

/** parse configuration file */
void crawler::parseFile (char *file) {
    int fds = open(file, O_RDONLY);
    if (fds < 0) {
        cerr << "cannot open config file (" << file << ") : "
            << strerror(errno) << endl;
        exit(1);
    }
    char *tmp = readfile(fds);
    close(fds);
    // suppress commentary
    bool eff = false;
    for (int i=0; tmp[i] != 0; i++) {
        switch (tmp[i]) {
            case '\n': eff = false; break;
            case '#': eff = true; // no break !!!
            default: if (eff) tmp[i] = ' ';
        }
    }
    char *posParse = tmp;
    char *tok = nextToken(&posParse);
    while (tok != NULL) {
        if (!strcasecmp(tok, "UserAgent")) {
            userAgent = newString(nextToken(&posParse));
        } else if (!strcasecmp(tok, "From")) {
            sender = newString(nextToken(&posParse));
        } else if (!strcasecmp(tok, "startUrl")) {
            tok = nextToken(&posParse);
            url *u = new url(tok, crawler::depthInSite, (url *) NULL);
            if (u->isValid()) {
                printf("Start from: %s\n", tok);
                check(u);
            } else {
                cerr << "the start url " << tok << " is invalid\n";
                exit(1);
            }
        } else if (!strcasecmp(tok, "waitduration")) {
            tok = nextToken(&posParse);
            waitDuration = atoi(tok);
        } else if (!strcasecmp(tok, "proxy")) {
            // host name and dns call
            tok = nextToken(&posParse);
            struct hostent* hp;
            proxyAddr = new sockaddr_in;
            memset((char *) proxyAddr, 0, sizeof (struct sockaddr_in));
            if ((hp = gethostbyname(tok)) == NULL) {
                endhostent();
                cerr << "Unable to find proxy ip address (" << tok << ")\n";
                exit(1);
            } else {
                proxyAddr->sin_family = hp->h_addrtype;
                memcpy ((char*) &proxyAddr->sin_addr, hp->h_addr, hp->h_length);
            }
            endhostent();
            // port number
            tok = nextToken(&posParse);
            proxyAddr->sin_port = htons(atoi(tok));
        } else if (!strcasecmp(tok, "pagesConnexions")) {
            tok = nextToken(&posParse);
            nb_conn = atoi(tok);
        } else if (!strcasecmp(tok, "dnsConnexions")) {
            tok = nextToken(&posParse);
            dnsConn = atoi(tok);
        } else if (!strcasecmp(tok, "depthInSite")) {
            tok = nextToken(&posParse);
            depthInSite = atoi(tok);
        } else if (!strcasecmp(tok, "limitToDomain")) {
            manageDomain(&posParse);
        } else if (!strcasecmp(tok, "forbiddenExtensions")) {
            manageExt(&posParse);
        } else if (!strcasecmp(tok, "noExternalLinks")) {
            externalLinks = false;
        } else {
            cerr << "bad configuration file : " << tok << "\n";
            exit(1);
        }
        tok = nextToken(&posParse);
    }
    delete [] tmp;
}

/** read the domain limit */
void crawler::manageDomain (char **posParse) {
    char *tok = nextToken(posParse);
    if (domains == NULL) {
        domains = new Vector<char>;
    }
    while (tok != NULL && strcasecmp(tok, "end")) {
        domains->addElement(newString(tok));
        tok = nextToken(posParse);
    }
    if (tok == NULL) {
        cerr << "Bad configuration file : no end to limitToDomain\n";
        exit(1);
    }
}

/** read the forbidden extensions */
void crawler::manageExt (char **posParse) {
    char *tok = nextToken(posParse);
    while (tok != NULL && strcasecmp(tok, "end")) {
        int l = strlen(tok);
        int i;
        for (i=0; i<l; i++) {
            tok[i] = tolower(tok[i]);
        }
        if (!matchPrivExt(tok))
            forbExt.addElement(newString(tok));
        tok = nextToken(posParse);
    }
    if (tok == NULL) {
        cerr << "Bad configuration file : no end to forbiddenExtensions\n";
        exit(1);
    }
}

/** make sure the max fds has not been reached */
void crawler::verifMax (int fd) {
    if (fd >= maxFds) {
        int n = 2 * maxFds;
        if (fd >= n) {
            n = fd + maxFds;
        }
        short *tmp = new short[n];
        for (int i=0; i<maxFds; i++) {
            tmp[i] = ansPoll[i];
        }
        for (int i=maxFds; i<n; i++) {
            tmp[i] = 0;
        }
        delete (ansPoll);
        maxFds = n;
        ansPoll = tmp;
    }
}

///////////////////////////////////////////////////////////
// Struct Connexion
///////////////////////////////////////////////////////////

/** put Connection in a coherent state
 */
Connexion::Connexion () {
    state = emptyC;
    parser = NULL;
}

/** Destructor : never used : we recycle !!!
 */
Connexion::~Connexion () {
    assert(false);
}

/** Recycle a connexion
 */
void Connexion::recycle () {
    delete parser;
    request.recycle();
}
