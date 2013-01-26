#ifndef RENDEZVOUS_H
#define RENDEZVOUS_H

#include <pthread.h>

#define Rendezvous_INFINITE -1

class Rendezvous {
    private:
        int orig;
        int count;
        int forceState;
        pthread_mutex_t mutex;
        pthread_cond_t  cond;
    public:
        /** threadNum: number of threads to synchronize.
         *  Use Rendezvous_INFINITE for a Rendezvous that can
         *  only be unblocked with force() or forceAndReset()
         */
        Rendezvous(int threadNum);
        ~Rendezvous();
        /** Called by a thread when it's ready to meet up with other
         *  threads. This will register that the thread is ready, and
         *  will block the calling thread until a total number of count
         *  threads have called this function on this Rendezvous object.
         *  When this happens, all threads are unblocked at once and the
         *  Rendezvous object will be reset to it's original count.
         */
        void meet();
        /** This call forces all threads blocking in meet() to
         *  unblock no matter what the state of the Rendezvous object.
         *  Useful for e.g. error cleanup. To reuse a Rendezvous object
         *  after this call, reset() must be called.
         */
        void force();
        /** This call resets the Rendezvous object to it's original 
         *   count at opening time
         */
        void reset();
        /** This call is equivalent to calling force() followed by
         *  reset() in a single operation. This will ensure that calls
         *  to meet() will not fail to block in-between the calls
         *  to force() and reset() if they were called separately.
         */
        void forceAndReset();
};

#endif // RENDEZVOUS_H
