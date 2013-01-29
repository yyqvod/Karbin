#include <iostream>
#include <stdlib.h>

#include "utils/rendezvous.h"

using namespace std;

Rendezvous::Rendezvous(int threadNum)
{
    if ((threadNum < 0) && (threadNum != Rendezvous_INFINITE)) {
        cerr<<"Count ("<<threadNum<<") should be > 0"<<endl;
        count = Rendezvous_INFINITE;
    }

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    count       = threadNum;
    orig        = count;
    forceState  = 0;
}

Rendezvous::~Rendezvous()
{
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

void Rendezvous::meet()
{
    pthread_mutex_lock(&mutex);
    if (!forceState) {
        if (count > 0) {
            count--;
        }
        if ((count > 0) || (count == Rendezvous_INFINITE)) {
            pthread_cond_wait(&cond, &mutex);
        } 
        else {
            pthread_cond_broadcast(&cond);
            count = orig;
        }
    }
    pthread_mutex_unlock(&mutex);
}

void Rendezvous::force()
{
    pthread_mutex_lock(&mutex);
    forceState = 1;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}

void Rendezvous::reset()
{
    pthread_mutex_lock(&mutex);
    count = orig;
    forceState = 0;
    pthread_mutex_unlock(&mutex);
}

void Rendezvous::forceAndReset()
{
    pthread_mutex_lock(&mutex);
    pthread_cond_broadcast(&cond);
    count = orig;
    forceState = 0;
    pthread_mutex_unlock(&mutex);
}
