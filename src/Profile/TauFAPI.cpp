/****************************************************************************
**			TAU Portable Profiling Package			   **
**			http://www.acl.lanl.gov/tau		           **
*****************************************************************************
**    Copyright 1997  						   	   **
**    Department of Computer and Information Science, University of Oregon **
**    Advanced Computing Laboratory, Los Alamos National Laboratory        **
****************************************************************************/
/***************************************************************************
**	File 		: TauFAPI.cpp					  **
**	Description 	: TAU Profiling Package wrapper for F77/F90	  **
**	Author		: Sameer Shende					  **
**	Contact		: sameer@cs.uoregon.edu sameer@acl.lanl.gov 	  **
**	Flags		: Compile with				          **
**			  -DPROFILING_ON to enable profiling (ESSENTIAL)  **
**			  -DPROFILE_STATS for Std. Deviation of Excl Time **
**			  -DSGI_HW_COUNTERS for using SGI counters 	  **
**			  -DPROFILE_CALLS  for trace of each invocation   **
**                        -DSGI_TIMERS  for SGI fast nanosecs timer       **
**			  -DTULIP_TIMERS for non-sgi Platform	 	  **
**			  -DPOOMA_STDSTL for using STD STL in POOMA src   **
**			  -DPOOMA_TFLOP for Intel Teraflop at SNL/NM 	  **
**			  -DPOOMA_KAI for KCC compiler 			  **
**			  -DDEBUG_PROF  for internal debugging messages   **
**                        -DPROFILE_CALLSTACK to enable callstack traces  **
**	Documentation	: See http://www.acl.lanl.gov/tau	          **
***************************************************************************/

/* Fortran Wrapper layer for TAU Portable Profiling */
#include <stdio.h>
#include <ctype.h>
#include "Profile/ProfileGroups.h"

/* 
#define DEBUG_PROF
*/

extern "C" {
void * tau_get_profiler(char *, char *, TauGroup_t);
void tau_start_timer(void *);
void tau_stop_timer(void *);
void tau_exit(char *);
void tau_init(int, char **);
void tau_set_node(int);
void tau_set_context(int);
void tau_register_thread(void);
void tau_trace_sendmsg(int type, int destination, int length);
void tau_trace_recvmsg(int type, int source, int length);
void * tau_get_userevent(char *name);
void tau_userevent(void *ue, double data);
void tau_report_statistics(void);
void tau_report_thread_statistics(void);

/*****************************************************************************
* The following routines are called by the Fortran program and they in turn
* invoke the corresponding C routines. 
*****************************************************************************/
void tau_profile_timer_(void **ptr, char *fname, int *flen)
{
#ifdef DEBUG_PROF
  printf("Inside tau_profile_timer_ fname=%s\n", fname);
#endif /* DEBUG_PROF */
  if (*ptr == 0) 
  {  // remove garbage characters from the end of name
    for(int i=0; i<1024; i++)
    {
      if (!isprint(fname[i]))
      { 
        fname[i] = '\0';
        break;
      }
    }
    *ptr = tau_get_profiler(fname, " ", TAU_DEFAULT);
  }

#ifdef DEBUG_PROF 
  printf("get_profiler returns %x\n", *ptr);
#endif /* DEBUG_PROF */

  return;
}


void tau_profile_start_(void **profiler)
{ 
#ifdef DEBUG_PROF
  printf("start_timer gets %x\n", *profiler);
#endif /* DEBUG_PROF */

  tau_start_timer(*profiler);
  return;
}

void tau_profile_stop_(void **profiler)
{
  tau_stop_timer(*profiler);
  return;
}

void tau_profile_exit_(char *msg)
{
  tau_exit(msg);
  return;
}

void tau_profile_init_(int *argc, char ***argv)
{
  /* tau_init(*argc, *argv); */
  return;
}

void tau_profile_set_node_(int *node)
{
  tau_set_node(*node);
  return;
} 

void tau_profile_set_context_(int *context)
{
  tau_set_context(*context);
  return;
}

#if (defined (PTHREADS) || defined (TULIPTHREADS))
void tau_register_thread_(void)
{
  tau_register_thread();
  return;
}

#ifdef CRAYKAI
/* Cray F90 specific extensions */
void TAU_REGISTER_THREAD(void)
{
  tau_register_thread();
}
#endif /* CRAYKAI */
#endif /* PTHREADS || TULIPTHREADS */

void tau_trace_sendmsg_(int *type, int *destination, int *length)
{
  tau_trace_sendmsg(*type, *destination, *length);
}

void tau_trace_recvmsg_(int *type, int *source, int *length)
{
  tau_trace_recvmsg(*type, *source, *length);
}

void tau_register_event_(void **ptr, char *event_name, int *flen)
{

  if (*ptr == 0) 
  {  // remove garbage characters from the end of name
    for(int i=0; i<1024; i++)
    {
      if (!isprint(event_name[i]))
      { 
        event_name[i] = '\0';
        break;
      }
    }
#ifdef DEBUG_PROF
    printf("tau_get_userevent() \n");
#endif /* DEBUG_PROF */
    *ptr = tau_get_userevent(event_name);
  }
  return;

}

void tau_event_(void **ptr, double *data)
{
  tau_userevent(*ptr, *data);
}

void tau_report_statistics_(void)
{
  tau_report_statistics();
}

void tau_report_thread_statistics_(void)
{
  tau_report_thread_statistics();
}

/* Cray F90 specific extensions */
#ifdef CRAYKAI
void _main();
void TAU_PROFILE_TIMER(void **ptr, char *fname, int *flen)
{
//#ifdef DEBUG_PROF
//  printf("flen = %d\n", *flen);
//#endif /* DEBUG_PROF */
  

  if (*ptr == 0) 
  {  // remove garbage characters from the end of name
    for(int i=0; i<1024; i++)
    {
      if (!isprint(fname[i]))
      { 
        fname[i] = '\0';
        break;
      }
    }
#ifdef DEBUG_PROF
    printf("tau_get_profiler() \n");
#endif /* DEBUG_PROF */
    *ptr = tau_get_profiler(fname, " ", TAU_DEFAULT);
  }

#ifdef DEBUG_PROF 
  printf("get_profiler returns %x\n", *ptr);
#endif /* DEBUG_PROF */

  return;
}

void TAU_PROFILE_START(void **profiler)
{
  tau_profile_start_(profiler);
}

void TAU_PROFILE_STOP(void **profiler)
{
  tau_profile_stop_(profiler);
}

void TAU_PROFILE_EXIT(char *msg)
{
  printf("TAU_PROFILE_EXIT");
  tau_profile_exit_(msg);
}

void TAU_PROFILE_INIT()
{
  _main();
  // tau_profile_init_(argc, argv);
}

void TAU_PROFILE_SET_NODE(int *node)
{
  tau_profile_set_node_(node);
}

void TAU_PROFILE_SET_CONTEXT(int *context)
{
  tau_profile_set_context_(context);
}

void TAU_TRACE_SENDMSG(int *type, int *destination, int *length)
{
  tau_trace_sendmsg(*type, *destination, *length);
}

void TAU_TRACE_RECVMSG(int *type, int *source, int *length)
{
  tau_trace_recvmsg(*type, *source, *length);
}

void TAU_REGISTER_EVENT(void **ptr, char *event_name, int *flen)
{

  if (*ptr == 0) 
  {  // remove garbage characters from the end of name
    for(int i=0; i<1024; i++)
    {
      if (!isprint(event_name[i]))
      { 
        event_name[i] = '\0';
        break;
      }
    }
#ifdef DEBUG_PROF
    printf("tau_get_userevent() \n");
#endif /* DEBUG_PROF */
    *ptr = tau_get_userevent(event_name);
  }
  return;

}

void TAU_EVENT(void **ptr, double *data)
{
  tau_userevent(*ptr, *data);
}

void TAU_REPORT_STATISTICS(void)
{
  tau_report_statistics();
}

void TAU_REPORT_THREAD_STATISTICS(void)
{
  tau_report_thread_statistics();
}
#endif /* CRAYKAI */

} /* extern "C" */

/***************************************************************************
 * $RCSfile: TauFAPI.cpp,v $   $Author: sameer $
 * $Revision: 1.7 $   $Date: 1999/06/20 17:34:41 $
 * POOMA_VERSION_ID: $Id: TauFAPI.cpp,v 1.7 1999/06/20 17:34:41 sameer Exp $ 
 ***************************************************************************/
