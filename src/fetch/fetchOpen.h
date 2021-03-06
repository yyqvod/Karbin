// Larbin
// Sebastien Ailleret
// 15-11-99 -> 12-12-01

#ifndef FETCHOPEN_H
#define FETCHOPEN_H

/* Opens sockets
 * this function perform dns calls, using adns
 */
void fetchDns (Crawler *pCrawler);

/* Opens sockets
 * Never block (only opens sockets on allready known sites)
 * work inside the main thread
 */
void fetchOpen (Crawler *pCrawler);

#endif // FETCHOPEN_H
