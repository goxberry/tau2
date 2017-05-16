/****************************************************************************
 * **                      TAU Portable Profiling Package                     **
 * **                      http://www.cs.uoregon.edu/research/tau             **
 * *****************************************************************************
 * **    Copyright 1997-2017                                                  **
 * **    Department of Computer and Information Science, University of Oregon **
 * **    Advanced Computing Laboratory, Los Alamos National Laboratory        **
 * ****************************************************************************/
/******************************************************************************
 * **      File            : TauPluginInternals.h                             **
 * **      Description     : Internal API for the TAU Plugin System           **
 * **      Contact         : sramesh@cs.uoregon.edu                           **
 * **      Documentation   : See http://www.cs.uoregon.edu/research/tau       **
 * ***************************************************************************/

#include <Profile/TauPluginTypes.h>

#ifndef _TAU_PLUGIN_INTERNALS_H_
#define _TAU_PLUGIN_INTERNALS_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TAU_PLUGIN_INIT_FUNC "Tau_plugin_init_func"

int Tau_initialize_plugin_system();
int Tau_util_load_and_register_plugins(PluginManager* plugin_manager);
void* Tau_util_load_plugin(const char *name, const char *path, PluginManager* plugin_manager);
void* Tau_util_register_plugin(const char *name, void* handle, PluginManager* plugin_manager);

int Tau_util_cleanup_all_plugins();

PluginManager* Tau_util_get_plugin_manager();
void Tau_util_invoke_callbacks(Tau_plugin_event event, const void * data);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _TAU_PLUGIN_INTERNALS_H_ */
