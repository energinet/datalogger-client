/*
 * Copyright (C) 2014 - 2015 LIAB ApS <info@liab.dk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#include "timeout.h"

static void
timeout_handler(union sigval val) {
	kill(val.sival_int, SIGKILL);
}

/**system_timeout makes it possible to run system(3) style commands with timeouts.
	The timeout parameter is set to the time remaining after execution
  @param cmd The command to execute
  @param timeout A pointer to a nanosecond timeout
  @return The return value is an integer consisting of one or more of the following:
  - An error in the timeout system (unable to fork etc.). This can be tested with
	the macro SYSTEM_TIMEOUT_ERROR() to the return value, or by checking if the
	timeout was set to -1. The different errors
	can be seen in the header file for this function. If the fork failed,
	the command was never executed. In other cases, the command might have begun
	execution, but was killed at the time of the error. It is not possible to test
	information on command exit status after an error in the timeout system.
  - The command was terminated by a signal, see wait(2). In the case of a timeout
	the signal will be SIGKILL, and the timeout parameter will be set to 0.
  - The command exited normally, see wait(2).
  @note It is possible for the timeout to trigger at the same time as the child
	process finishes normally, in which case the timeout parameter will be 0, but
	the exit status of the child is correctly returned. Please test whether the
	child was terminated by a signal in conjunction with testing the timeout parameter.
 */
int
system_timeout(const char *cmd, long long int *timeout) {
	timer_t timer;
	pid_t worker_id;
	int status = 0;
	struct sigevent timerevent;
	struct itimerspec nanotimeout;

	//Start the child process
	worker_id = fork();
	if (worker_id == -1) {
		*timeout = -1;
		return SYSTEM_TIMEOUT_FORK_FAILED;
	}

	if (worker_id == 0) {
		//Replace the child with the command, same style as system(3)
		execl("/bin/sh", "sh", "-c", cmd, (char *) 0);
	} else {

		//Create a timer structure for handling timer expiry
		timerevent.sigev_notify = SIGEV_THREAD;
		timerevent.sigev_value.sival_int = worker_id;
		timerevent.sigev_notify_function = timeout_handler;
		timerevent.sigev_notify_attributes = NULL;

		nanotimeout.it_value.tv_sec = *timeout / 1000000000;
		nanotimeout.it_value.tv_nsec = *timeout % 1000000000;
		nanotimeout.it_interval.tv_sec = 0;
		nanotimeout.it_interval.tv_nsec = 0;

		//Try to create a timer using the monotonic clock - fall back to realtime if necessary 
		if (timer_create(CLOCK_MONOTONIC, &timerevent, &timer) == -1) {
			if (timer_create(CLOCK_REALTIME, &timerevent, &timer) == -1) {
				kill(worker_id, SIGKILL);
				*timeout = -1;
				return SYSTEM_TIMEOUT_TIMER_CREATE_FAILED;
			}
		}

		if (timer_settime(timer, 0, &nanotimeout, NULL) == -1) {
			kill(worker_id, SIGKILL);
			*timeout = -1;
			status = SYSTEM_TIMEOUT_TIMER_SET_FAILED;
		} else {
			//Wait for the child process to exit
			wait(&status);

			//Update the timeout with the remaining time
			timer_gettime(timer, &nanotimeout);
			*timeout = (nanotimeout.it_value.tv_sec * 1000000000);
			*timeout += nanotimeout.it_value.tv_nsec;

		}

	}
	timer_delete(timer);
	return status;
}
