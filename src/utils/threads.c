#include "threads.h"

#ifdef __linux__
	#include <sched.h>
#else
	#error "Your platform isn't supported for cross-platform threads."
#endif

void threads_yield_thread(void) {
	#ifdef __linux__
		sched_yield();
	#endif
	/* Maybe use "pthread_yield"? */
}