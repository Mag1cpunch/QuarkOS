#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdint.h>

typedef struct {
	volatile int locked;
} spinlock_t;

static inline void spinlock_init(spinlock_t *lock) {
	lock->locked = 0;
}

void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);

#endif // SPINLOCK_H