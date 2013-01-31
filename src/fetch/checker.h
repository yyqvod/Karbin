// Larbin
// Sebastien Ailleret
// 15-11-99 -> 13-04-00

#ifndef CHECKER_H
#define CHECKER_H

#include "types.h"
#include "utils/Vector.h"

extern int nrCrawler;
extern int map[nrVNode]; //Map: Virtual-Node --> Crawler
extern Crawler *crawlers[nrVNode]; //record all crawlers

/** check if an url is allready known
 * if not send it
 * @param u the url to check
 */
void check (url *u, Crawler *pCrawler);

/** Check the extension of an url
 * @return true if it might be interesting, false otherwise
 */
bool filter1 (char *host, char *file, Crawler *pCrawler);

#endif // CHECKER_H
