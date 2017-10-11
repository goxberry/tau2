/****************************************************************************
 **			TAU Portable Profiling Package			   **
 **			http://www.cs.uoregon.edu/research/tau	           **
 *****************************************************************************
 **    Copyright 1997-2006                                                  **
 **    Department of Computer and Information Science, University of Oregon **
 **    Advanced Computing Laboratory, Los Alamos National Laboratory        **
 ****************************************************************************/
/****************************************************************************
 **	File 		: LikwidLayer.cpp                                    **
 **	Description 	: TAU Profiling Package			           **
 **	Contact		: tau-team@cs.uoregon.edu 		 	   **
 **	Documentation	: See http://www.cs.uoregon.edu/research/tau       **
 ****************************************************************************/

#include <Profile/Profiler.h>
#include <Profile/TauSampling.h>
#include <Profile/UserEvent.h>

#ifdef TAU_DOT_H_LESS_HEADERS
#include <iostream>
using namespace std;
#else /* TAU_DOT_H_LESS_HEADERS */
#include <iostream.h>
#endif /* TAU_DOT_H_LESS_HEADERS */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef TAU_AT_FORK
#include <pthread.h>
#endif /* TAU_AT_FORK */

#ifdef TAU_BEACON
#include <Profile/TauBeacon.h>
#endif /* TAU_BEACON */

extern "C" {
#include <likwid.h>
}

#define dmesg(level, fmt, ...)

bool LikwidLayer::likwidInitialized = false;
ThreadValue * LikwidLayer::ThreadList[TAU_MAX_THREADS] = { 0 };

//string LikwidLayer::eventString;//[] = "L2_LINES_IN_ALL:PMC0,L2_TRANS_L2_WB:PMC1";
int* LikwidLayer::cpus;
int LikwidLayer::gid;
int LikwidLayer::err;
int LikwidLayer::numCounters = 0;
//int LikwidLayer::counterList[MAX_PAPI_COUNTERS];

int tauSampEvent = 0;

int LikwidLayer::initializeLikwidLayer() //Tau_initialize_likwid_library(void)
{

	//int* cpus;

	int w;
	int y;
	int z;
	LikwidLayer::err = topology_init();
	CpuInfo_t info = get_cpuInfo();
	CpuTopology_t topo = get_cpuTopology();
	affinity_init();

	LikwidLayer::cpus = (int*) malloc(topo->activeHWThreads * sizeof(int)); //vs numHWThreads
	if (!LikwidLayer::cpus)
		return 1;
	int w1 = 0;
	for (w = 0; w < topo->numHWThreads; w++) {
		if (topo->threadPool[w].inCpuSet == 1) {
			LikwidLayer::cpus[w1] = topo->threadPool[w].apicId;
			w1++;
		}
	}
	//perfmon_setVerbosity(3);

	LikwidLayer::err = perfmon_init(topo->activeHWThreads, LikwidLayer::cpus);

}

extern "C" int Tau_is_thread_fake(int tid);
extern "C" int TauMetrics_init(void);

/////////////////////////////////////////////////
int LikwidLayer::addEvents(const char *estr) {
	int code;
	/*
	 if(firstString){
	 eventString = string(estr);
	 firstString=false;
	 }
	 else{
	 eventString=eventString+","+string(estr);
	 }
	 */
	TAU_VERBOSE("TAU: LIKWID: Adding events %s\n", estr);

	//LikwidLayer::err = perfmon_stopCounters();
	LikwidLayer::gid = perfmon_addEventSet(estr);
	LikwidLayer::err = perfmon_setupCounters(LikwidLayer::gid);
	//printf("SetupCounters error: %d\n",LikwidLayer::err);
	LikwidLayer::err = perfmon_startCounters();
	//printf("StartCounters error: %d\n",LikwidLayer::err);
	numCounters = perfmon_getNumberOfEvents(LikwidLayer::gid);
	return LikwidLayer::gid;
}

////////////////////////////////////////////////////
int LikwidLayer::initializeThread(int tid) {
	int rc;

	if (tid >= TAU_MAX_THREADS) {
		fprintf(stderr, "TAU: Exceeded max thread count of TAU_MAX_THREADS\n");
		return -1;
	}

	if (!ThreadList[tid]) {
		RtsLayer::LockDB();
		if (!ThreadList[tid]) {
			dmesg(1, "TAU: LIKWID: Initializing Thread Data for TID = %d\n", tid);

			/* Task API does not have a real thread associated with it. It is fake */
			if (Tau_is_thread_fake(tid) == 1)
				tid = 0;

			ThreadList[tid] = new ThreadValue;
			ThreadList[tid]->ThreadID = tid;

			ThreadList[tid]->CounterValues = new long long[numCounters];
			memset(ThreadList[tid]->CounterValues, 0,
					numCounters * sizeof(long long));

		} /*if (!ThreadList[tid]) */
		RtsLayer::UnLockDB();
	} /*if (!ThreadList[tid]) */

	dmesg(10, "ThreadList[%d] = %p\n", tid, ThreadList[tid]);
	return 0;
}

/////////////////////////////////////////////////
long long *LikwidLayer::getAllCounters(int tid, int *numValues) {
	int rc = 0;
	long long tmpCounters[numCounters];
	//perfmon_setVerbosity(3);

	/* Task API does not have a real thread associated with it. It is fake */
	if (Tau_is_thread_fake(tid) == 1)
		tid = 0;

	if (!likwidInitialized) {
		if (initializeLikwidLayer()) {
			return NULL;
		}
	}

	if (numCounters == 0) {
		// adding must have failed, just return
		return NULL;
	}

	if (ThreadList[tid] == NULL) {
		if (initializeThread(tid)) {
			return NULL;
		}
	}

	*numValues = numCounters; //TODO: Warning. Likwid adds two additional counters to the specified counter list. This does not seem to matter but could cause unexpected issues in the future. Consider adjusting the value array to contain only the user specified counters.
	//printf("About to read tid:%d, cpu:%d, gid:%d\n",tid,LikwidLayer::cpus[tid],LikwidLayer::gid);
	int readres = perfmon_readCounters(); // GroupThread(LikwidLayer::gid,tid);
	//printf("Read returned %d\n",readres);

	for (int comp = 0; comp < numCounters; comp++) {
		//int comp=0;
		tmpCounters[comp] = perfmon_getLastResult(LikwidLayer::gid, comp, tid);

		//for (int j=0; j<numCounters; j++) {
		ThreadList[tid]->CounterValues[comp] += tmpCounters[comp];
		//printf("ThreadList[%d]->CounterValues[%d] = %lld\n", tid, comp, ThreadList[tid]->CounterValues[comp]);
		//}

	}

	return ThreadList[tid]->CounterValues;
}

