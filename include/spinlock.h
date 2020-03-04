/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdatomic.h>

typedef struct spinlock_s {
    atomic_int val;
} spinlock_t;

static inline void spinlock_init(spinlock_t *p_lock)
{
    atomic_store(&p_lock->val, 0);
}

static inline void spin_lock(spinlock_t *p_lock)
{
    while (atomic_flag_test_and_set(&p_lock->val)) {
        while (atomic_load(&p_lock->val) != 0);
    }
}

static inline void spin_unlock(spinlock_t *p_lock)
{
    atomic_store(&p_lock->val, 0);
}

#endif /* SPINLOCK_H */

