/*
 * Process Management Functions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef _PIDS_H_
#define _PIDS_H_

#include <sys/types.h>

/*
 * find_pid_by_name - Find all PIDs with the given process name
 * @procName: Process name to search for
 * @return: Dynamically allocated array of PIDs terminated by 0, or NULL
 *
 * Caller must free the returned array.
 */
pid_t* find_pid_by_name(const char *procName);

/*
 * pids - Check if a process is running
 * @appname: Process name
 * @return: 1 if running, 0 if not
 */
int pids(char *appname);

/*
 * pids_main - Print all PIDs for a process (for debugging)
 * @appname: Process name
 * @return: Number of PIDs found
 */
int pids_main(char *appname);

#endif /* _PIDS_H_ */
