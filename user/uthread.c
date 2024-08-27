#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/uthread.h"

// thread_create() is a wrapper around tfork().
int thread_create(thread_t *t, void*(*func)(void*), void *arg) {
  int tid;

  // Call the tfork() syscall.
  tid = tfork(func, arg);
  *t = tid;

  return !(tid);
}

// thread_join() is a wrapper around twait().
int thread_join(thread_t t, void **ret) {
  // Call the twait() syscall.
  return !(twait(t, ret));
}

// thread_exit() is a wrapper around texit().
void thread_exit(void *ret) {
  // Call the texit() syscall.
  texit(ret);
}

void thread_mutex_init(thread_mutex_t *mtx) {
  mtx->locked = 0;
}

void thread_mutex_destroy(thread_mutex_t *mtx) {
  // not necessary
}

// Acquire the lock.
// Loop until the lock is acquired.
void thread_mutex_lock(thread_mutex_t *mtx) {
  while(__sync_lock_test_and_set(&mtx->locked, 1) != 0)
    ;
  __sync_synchronize();
}

// Release the lock.
void thread_mutex_unlock(thread_mutex_t *mtx) {
  __sync_synchronize();
  __sync_lock_release(&mtx->locked);
}
