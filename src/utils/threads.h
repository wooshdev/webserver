/**
 * Copyright (C) 2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#ifndef UTILS_THREADS_H
#define UTILS_THREADS_H

/**
 * Description:
 *   This function will yield the current thread. As pthread_yield is 
 *   non-standard, this function tries to call a platform-specific 
 *   function instead.
 */
void threads_yield_thread(void);

#endif /* UTILS_THREADS_H */
