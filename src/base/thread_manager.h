/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#ifndef BASE_THREAD_MANAGER_H
#define BASE_THREAD_MANAGER_H

#include "configuration/config.h"

/**
 * Description:
 *   Setup the thread manager.
 *
 * Parameter:
 *   config_t
 *     The configuration to read its' options from.
 *
 * Return Value:
 *   (boolean) Success Status.
 */
int thread_manager_setup(config_t);

/**
 * Description:
 *   Kill the thread manager's child threads.
 */
void thread_manager_wait_or_kill(void);

/**
 * Description:
 *   Create & add a thread to the thread pool.
 *
 * Parameters:
 *   start_routine
 *     The function to call when the thread starts.
 *   void *
 *     The start_routine function's parameter.
 *
 * Return Value:
 *   -2: An error has occurred while creating the thread.
 *   -1: An allocation error has occurred.
 *    0: The thread pool is full.
 *    1: Success.
 */
int thread_manager_add(void *(*start_routine) (void *), void *);

/**
 * Description:
 *   Signals the thread manager that this thread has been finished. 
 *   This function should only be called on threads created by thread_manager_add.
 */
void thread_manager_finished(void);

#endif /* BASE_THREAD_MANAGER_H */