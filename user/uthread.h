// Thread and mutex
typedef int thread_t;
typedef struct _thread_mutex_t_ { int locked; } thread_mutex_t;

// Thread interfaces
int thread_create(thread_t *t, void*(*func)(void*), void *arg);
int thread_join(thread_t t, void **ret);
void thread_exit(void *ret) __attribute__((noreturn));

// Mutex interfaces
void thread_mutex_init(thread_mutex_t *mtx);
void thread_mutex_destroy(thread_mutex_t *mtx);
void thread_mutex_lock(thread_mutex_t *mtx);
void thread_mutex_unlock(thread_mutex_t *mtx);
