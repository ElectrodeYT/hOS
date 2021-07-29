#ifndef MUTEX_H
#define MUTEX_H

typedef volatile long mutex_t;

static inline void acquire(mutex_t* mutex) {
	while(!__sync_bool_compare_and_swap(mutex, 0, 1)) {
		asm("pause");
	}
}
 
static inline void release(mutex_t* mutex) {
	__sync_synchronize();
	*mutex = 0;
}

#endif