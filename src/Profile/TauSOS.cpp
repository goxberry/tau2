
#include "Profile/TauSOS.h"
#include "Profile/Profiler.h"
#include "Profile/TauMetrics.h"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <stdexcept>
#include <cassert>
#include "stdio.h"
#include "error.h"
#include "errno.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

//#include <mpi.h>

#include "sos.h"

SOS_pub *pub = NULL;
unsigned long fi_count = 0;
static bool done = false;
pthread_mutex_t _my_mutex; // for initialization, termination
pthread_t worker_thread;
bool _threaded = false;

void init_lock(void) {
    if (!_threaded) return;
    pthread_mutexattr_t Attr;
    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_ERRORCHECK);
    int rc;
    if ((rc = pthread_mutex_init(&_my_mutex, &Attr)) != 0) {
        errno = rc;
        perror("pthread_mutex_init error");
        exit(1);
    }
}
void do_lock(void) {
    if (!_threaded) return;
    int rc;
    if ((rc = pthread_mutex_lock(&_my_mutex)) != 0) {
        errno = rc;
        perror("pthread_mutex_lock error");
        exit(1);
    }
}

void do_unlock(void) {
    if (!_threaded) return;
    int rc;
    if ((rc = pthread_mutex_unlock(&_my_mutex)) != 0) {
        errno = rc;
        perror("pthread_mutex_unlock error");
        exit(1);
    }
}

class scoped_lock {
public:
    scoped_lock(void) { do_lock(); }
    ~scoped_lock(void) { do_unlock(); }
};

void * Tau_sos_thread_function(void* data) {
    //PMPI_Barrier( TAU_SOS_MAP_COMMUNICATOR(MPI_COMM_WORLD) ); // wait for everyone to join
    while (!done) {
        sleep(2);
        TAU_VERBOSE("%d Sending data from TAU thread...\n", RtsLayer::myNode()); fflush(stderr);
        do_lock();
        TAU_SOS_send_data();
        do_unlock();
        TAU_VERBOSE("%d Done.\n", RtsLayer::myNode()); fflush(stderr);
    }
    TAU_VERBOSE("TAU SOS thread exiting.\n"); fflush(stderr);
    pthread_exit((void*)0L);
}

extern "C" void TAU_SOS_init(int * argc, char *** argv, bool threaded) {
    static bool initialized = false;
    if (!initialized) {
        _threaded = threaded > 0 ? true : false;
        init_lock();
        scoped_lock mylock;  // lock from now to the end of this block
        SOS_init(argc, argv, SOS_ROLE_CLIENT);

        char pub_name[SOS_DEFAULT_STRING_LEN] = {0};
        char app_version[SOS_DEFAULT_STRING_LEN] = {0};

        TAU_VERBOSE("[TAU_SOS_init]: Creating new pub...\n");

/* Fixme! Replace these with values from TAU metadata. */
        sprintf(pub_name, "TAU_SOS_SUPPORT");
        sprintf(app_version, "v0.alpha");
/* Fixme! Replace these with values from TAU metadata. */
        pub = SOS_pub_create(pub_name);

        pub->prog_ver           = strdup(app_version);
        pub->meta.channel       = 1;
        pub->meta.nature        = SOS_NATURE_EXEC_WORK;
        pub->meta.layer         = SOS_LAYER_FLOW;
        pub->meta.pri_hint      = SOS_PRI_IMMEDIATE;
        pub->meta.scope_hint    = SOS_SCOPE_SELF;
        pub->meta.retain_hint   = SOS_RETAIN_SESSION;

        TAU_VERBOSE("[TAU_SOS_init]:   ... done.  (pub->guid == %ld)\n", pub->guid);

        if (_threaded) {
            TAU_VERBOSE("Spawning thread for SOS.\n");
            int ret = pthread_create(&worker_thread, NULL, &Tau_sos_thread_function, NULL);
            if (ret != 0) {
                errno = ret;
                perror("Error: pthread_create (1) fails\n");
                exit(1);
            }
        }
        initialized = true;
        /* Fixme! Insert all the data that was collected into Metadata */
    }
}

extern "C" void TAU_SOS_stop_worker(void) {
    //printf("%s\n", __func__); fflush(stdout);
    do_lock();
    done = true;
    do_unlock();
    if (_threaded) {
        TAU_VERBOSE("TAU SOS thread joining...\n"); fflush(stderr);
        int ret = pthread_join(worker_thread, NULL);
        if (ret != 0) {
            switch (ret) {
                case ESRCH:
                    // already exited.
                    break;
                case EINVAL:
                    // Didn't exist?
                    break;
                case EDEADLK:
                    // trying to join with itself?
                    break;
                default:
                    errno = ret;
                    perror("Warning: pthread_join failed\n");
                    break;
            }
        }
    }
}

extern "C" void TAU_SOS_finalize(void) {
    static bool finalized = false;
    //printf("%s\n", __func__); fflush(stdout);
    if (finalized) return;
    if (!done) {
        TAU_SOS_stop_worker();
    }
    SOS_finalize();
    finalized = true;
}

extern "C" int TauProfiler_updateAllIntermediateStatistics(void);

extern "C" void TAU_SOS_send_data(void) {
    assert(pub);
    if (done) { return; }
    // get the most up-to-date profile information
    TauProfiler_updateAllIntermediateStatistics();

    // get the FunctionInfo database, and iterate over it
    std::vector<FunctionInfo*>::iterator it;
  const char **counterNames;
  int numCounters;
  TauMetrics_getCounterList(&counterNames, &numCounters);
  RtsLayer::LockDB();
  bool keys_added = false;

  //foreach: TIMER
  for (it = TheFunctionDB().begin(); it != TheFunctionDB().end(); it++) {
    FunctionInfo *fi = *it;
    // get the number of calls
    int tid = 0; // todo: get ALL thread data.
    SOS_val calls;
    SOS_val inclusive, exclusive;
    calls.i_val = 0.0;
    inclusive.d_val = 0.0;
    exclusive.d_val = 0.0;

    //foreach: THREAD
    for (tid = 0; tid < RtsLayer::getTotalThreads(); tid++) {
        calls.i_val = fi->GetCalls(tid);
        std::stringstream calls_str;
        calls_str << "TAU::" << tid << "::calls::" << fi->GetName();
        const std::string& tmpcalls = calls_str.str();

        SOS_pack(pub, tmpcalls.c_str(), SOS_VAL_TYPE_INT, calls);

        // todo - subroutines
        // iterate over metrics 
        std::stringstream incl_str;
        std::stringstream excl_str;
        for (int m = 0; m < Tau_Global_numCounters; m++) {
            incl_str.clear();
            incl_str << "TAU::" << tid << "::inclusive_" << counterNames[m] << "::" << fi->GetName();
            const std::string& tmpincl = incl_str.str();
            excl_str.clear();
            excl_str << "TAU::" << tid << "::exclusive_" << counterNames[m] << "::" << fi->GetName();
            const std::string& tmpexcl = excl_str.str();

            inclusive.d_val = fi->getDumpInclusiveValues(tid)[m];
            exclusive.d_val = fi->getDumpExclusiveValues(tid)[m];
            
            SOS_pack(pub, tmpincl.c_str(), SOS_VAL_TYPE_DOUBLE, inclusive);
            SOS_pack(pub, tmpexcl.c_str(), SOS_VAL_TYPE_DOUBLE, exclusive);
        }
    }
  }
  if (TheFunctionDB().size() > fi_count) {
    keys_added = true;
    fi_count = TheFunctionDB().size();
  }
  RtsLayer::UnLockDB();
  if (keys_added) {
      TAU_VERBOSE("[TAU_SOS_send_data]: Announcing the pub...\n");
      SOS_announce(pub);
      TAU_VERBOSE("[TAU_SOS_send_data]:   ...done.\n");
  }
  TAU_VERBOSE("[TAU_SOS_send_data]: Publishing the values...\n");
  SOS_publish(pub);
  TAU_VERBOSE("[TAU_SOS_send_data]:   ...done.\n");
}

