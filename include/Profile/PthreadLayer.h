/****************************************************************************
**			TAU Portable Profiling Package			   **
**			http://www.acl.lanl.gov/tau		           **
*****************************************************************************
**    Copyright 1997  						   	   **
**    Department of Computer and Information Science, University of Oregon **
**    Advanced Computing Laboratory, Los Alamos National Laboratory        **
****************************************************************************/
/***************************************************************************
**	File 		: PthreadLayer.h				  **
**	Description 	: TAU Profiling Package Pthread Support Layer	  **
**	Author		: Sameer Shende					  **
**	Contact		: sameer@cs.uoregon.edu sameer@acl.lanl.gov 	  **
**	Flags		: Compile with				          **
**			  -DPROFILING_ON to enable profiling (ESSENTIAL)  **
**			  -DPROFILE_STATS for Std. Deviation of Excl Time **
**			  -DSGI_HW_COUNTERS for using SGI counters 	  **
**			  -DPROFILE_CALLS  for trace of each invocation   **
**			  -DSGI_TIMERS  for SGI fast nanosecs timer	  **
**			  -DTULIP_TIMERS for non-sgi Platform	 	  **
**			  -DPOOMA_STDSTL for using STD STL in POOMA src   **
**			  -DPOOMA_TFLOP for Intel Teraflop at SNL/NM 	  **
**			  -DPOOMA_KAI for KCC compiler 			  **
**			  -DDEBUG_PROF  for internal debugging messages   **
**                        -DPROFILE_CALLSTACK to enable callstack traces  **
**	Documentation	: See http://www.acl.lanl.gov/tau	          **
***************************************************************************/

#ifndef _PTHREADLAYER_H_
#define _PTHREADLAYER_H_

//////////////////////////////////////////////////////////////////////
//
// class PthreadLayer
//
// This class is used for supporting pthreads in RtsLayer class.
//////////////////////////////////////////////////////////////////////

#ifdef PTHREADS
class PthreadLayer 
{ // Layer for RtsLayer to interact with pthreads 
  public:
 	
 	PthreadLayer () { }  // defaults
	~PthreadLayer () { } 

	static int RegisterThread(void); // called before any profiling code
        static int InitializeThreadData(void);     // init thread mutexes
        static int InitializeDBMutexData(void);     // init tauDB mutex
	static int GetThreadId(void); 	 // gets 0..N-1 thread id
	static int LockDB(void);	 // locks the tauDBMutex
	static int UnLockDB(void);	 // unlocks the tauDBMutex

  private:
	static pthread_key_t 	   tauPthreadId; // tid 
	static pthread_mutex_t     tauThreadcountMutex; // to protect counter
	static pthread_mutexattr_t tauThreadcountAttr; // count attribute 
	static int 		   tauThreadCount;     // counter
	static pthread_mutex_t	   tauDBMutex;  // to protect TheFunctionDB
	static pthread_mutexattr_t tauDBAttr;   // DB mutex attribute
	
};
#endif // PTHREADS 

#endif // _PTHREADLAYER_H_

	

/***************************************************************************
 * $RCSfile: PthreadLayer.h,v $   $Author: sameer $
 * $Revision: 1.1 $   $Date: 1998/07/10 20:19:58 $
 * POOMA_VERSION_ID: $Id: PthreadLayer.h,v 1.1 1998/07/10 20:19:58 sameer Exp $
 ***************************************************************************/


