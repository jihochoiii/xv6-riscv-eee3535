#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/uthread.h"

#define max_threads 64
#define num_threads 32
#define array_size  16384

int *array  = 0;
int *tid    = 0;
int *matrix = 0;
int *file   = 0;
thread_t *thread = 0;
thread_mutex_t mutex;


// Panic on critical errors
void panic(const char *str) {
  fprintf(2, "panic: %s\n", str);
  if(matrix) { free(matrix); }
  if(array)  { free(array);  }
  if(tid)    { free(tid);    }
  if(thread) { free(thread); }
  exit(1);
}


// Test #1 thread function
void* test1_func(void *arg) {
  // Nothing to do
  thread_exit(0);
}

// This test keeps calling thread_create() until it runs out of available slots
// in proc[NPROC] in kernel/proc.c.
int test1(int repeat) {
  printf("Test #1: creating as many threads as possible            ");

  int i = 0, count = 0, passed = 0;

  while(repeat--) {
    memset(thread, 0, max_threads * sizeof(thread_t));
  
    for(count = 0, i = 0; !thread_create(&thread[i], &test1_func, 0); count++, i++) ;
    for(i = 0; i < count; i++) {
      if(thread_join(thread[i], 0)) { panic("thread_join failed"); }
    }

    // A good number of threads should have been created.  
    if(!(passed = (count > num_threads))) { break; }
  }
  printf("(%s)\n", passed ? "pass" : "fail");

  return passed;
}


// Test #2 thread function
void* test2_func(void *arg) {
  int tid = *(int*)arg;
  // Set a flag in the array using tid as an index.
  array[tid] = 1;
  thread_exit(0);
}

// This test checks if tid values are correctly passed to thread functions.
// Each thread sets a flag in the array indexed by tid.
int test2(int repeat) {
  printf("Test #2: creating threads and passing arguments          ");

  int i = 0, sum = 0, passed = 0;

  array = (int*)malloc(num_threads * sizeof(int));

  while(repeat--) {
    memset(array, 0, num_threads * sizeof(int));
    memset(thread, 0, max_threads * sizeof(thread_t));
    sum = 0;
  
    for(i = 0; i < num_threads; i++) {
      if(thread_create(&thread[i], &test2_func, &tid[i])) { panic("thread_create failed"); }
    }
    for(i = 0; i < num_threads; i++) {
      if(thread_join(thread[i], 0)) { panic("thread_join failed"); }
      sum += (array[i] == 1);
    }
 
    // The number of flags must be the same as the number of threads.
    if(!(passed = (sum == num_threads))) { break; }
  }
  printf("(%s)\n", passed ? "pass" : "fail");

  free(array); array = 0;

  return passed;
}


// Test #3 thread function
void* test3_func(void *arg) {
  int tid = *(int*)arg;
  // Return a pointer that is tid elements away from the array's base address.
  int *ptr = array + tid;
  thread_exit(ptr);
}

// Each thread returns a pointer that is tid elements away from the base
// address of the array. By setting a flag at the returned pointer, the main
// thread confirms the correctness of reclaiming the return value.
int test3(int repeat) {
  printf("Test #3: reclaiming return values from thread functions  ");

  int i = 0, sum = 0, passed = 0;
  int *ret = 0;

  array = (int*)malloc(num_threads * sizeof(int));

  while(repeat--) {
    memset(array, 0, num_threads * sizeof(int));
    memset(thread, 0, max_threads * sizeof(thread_t));
    sum = 0;
  
    for(i = 0; i < num_threads; i++) {
      if(thread_create(&thread[i], &test3_func, &tid[i])) { panic("thread_create failed"); }
    }
    for(i = 0; i < num_threads; i++) {
      if(thread_join(thread[i], (void**)&ret)) { panic("thread_join failed"); }
      *ret = 1;
      sum += (array[i] == 1);
    }
  
    // The number of flags must be the same as the number of threads.
    if(!(passed = (sum == num_threads))) { break; }
  }
  printf("(%s)\n", passed ? "pass" : "fail");

  free(array); array = 0;

  return passed;
}


// Test #4 thread function 2
void* test4_func2(void *arg) {
  // Record the PID.
  *(int*)arg = getpid();
  thread_exit(0);
}

// Test #4 thread function 1
void* test4_func1(void *arg) {
  int pid = 0, tid = *(int*)arg;
  thread_t t;
  if(thread_create(&t, &test4_func2, &pid)) { panic("thread_create failed"); }
  if(thread_join(t, 0)) { panic("thread_join failed"); }
  // Check if the thread has the same PID.
  array[tid] = (pid == getpid()) ? pid : 0;
  thread_exit(0);
}

// This test recursively generates threads and checks if all threads have the
// same process IDs. Each child thread creates one grandchild thread.
int test4(int repeat) {
  printf("Test #4: recursively creating threads and checking PIDs  ");

  int i = 0, sum = 0, passed = 0;
  // The main thread generates a half number of threads.
  int thread_count = num_threads>>1, pid = getpid();

  array = (int*)malloc(thread_count * sizeof(int));

  while(repeat--) {
    memset(array, 0, thread_count * sizeof(int));
    memset(thread, 0, max_threads * sizeof(thread_t));
    sum = 0;
    
    for(i = 0; i < thread_count; i++) {
      if(thread_create(&thread[i], &test4_func1, &tid[i])) { panic("thread_create failed"); }
    }
    for(i = 0; i < thread_count; i++) {
      if(thread_join(thread[i], 0)) { panic("thread_join failed"); }
      sum += (array[i] == pid);
    }

    // The number of flags must be the same as the number of threads.
    if(!(passed = (sum == thread_count))) { break; }
  }
  printf("(%s)\n", passed ? "pass" : "fail");

  free(array); array  = 0;

  return passed;
}


// Test #5 thread function
void* test5_func(void *arg) {
  // xv6-riscv's malloc() is not thread-safe.
  thread_mutex_lock(&mutex);
  // malloc() by a thread, free() by the parent
  int *ptr = (int*)malloc(array_size);
  thread_mutex_unlock(&mutex);
  thread_exit(ptr);
}

// This test checks if the main and child threads harmoniously handle the
// shared virtual memory space. If child threads mistakenly use own pagetables
// and heap sizes, this test will make the kernel crash.
int test5(int repeat) {
  printf("Test #5: sharing virtual memory space between threads    ");

  int i = 0, passed = 0;
  int *ret = 0;

  while(repeat--) {
    memset(thread, 0, max_threads * sizeof(thread_t));
  
    for(i = 0; i < num_threads; i++) {
      if(thread_create(&thread[i], &test5_func, 0)) { panic("thread_create failed"); }
    }
    for(i = 0; i < num_threads; i++) {
      if(thread_join(thread[i], (void**)&ret)) { panic("thread_join failed"); }
      // xv6-riscv's free() is not thread-safe.
      thread_mutex_lock(&mutex);
      free(ret);
      thread_mutex_unlock(&mutex);
    }
 
    // Returned pointer should not be a null pointer.
    if(!(passed = (ret != 0))) { break; }
  }
  printf("(%s)\n", passed ? "pass" : "fail");

  return passed;
}


// Test #6 thread function
void* test6_func(void *arg) {
  int tid = *(int*)arg;
  // Close the file descriptor using tid as an index.
  close(file[tid]);
  thread_exit(0);
}

// This test checks if the main thread and child threads correctly handle the
// shared file descriptors. The main thread opens many file descriptors, and
// child threads close them. If the main tries to close the file descriptors,
// they should all fail because the threads have close them already.
int test6(int repeat) {
  printf("Test #6: sharing file descriptors between threads        ");

  int sum = 0, passed = 0;

  file = (int*)malloc(num_threads * sizeof(int));

  while(repeat--) {
    memset(file,   0, num_threads * sizeof(int));
    memset(thread, 0, num_threads * sizeof(int));
    sum = 0;

    // Open as many file descriptors as the number of threads.
    for(int i = 0; i < num_threads; i++) { file[i] = dup(1); }

    for(int i = 0; i < num_threads; i++) {
      if(thread_create(&thread[i], &test6_func, &tid[i])) { panic("thread_create failed"); }
    }
    for(int i = 0; i < num_threads; i++) {
      if(thread_join(thread[i], 0)) { panic("thread_join failed"); }
      sum += !close(file[i]);
    }

    // Closing file descriptors should have all failed because they were closed by threads.
    if(!(passed = (sum == 0))) { break; }
  }
  printf("(%s)\n", passed ? "pass" : "fail");

  free(file); file = 0;

  return passed;
}


// Test #7 thread function
void* test7_func(void *arg) {
  int i, tid = *(int*)arg;
  // The critical section is intentionally made lengthy to detect race conditions.
  thread_mutex_lock(&mutex);
  // Add all numbers in a matrix row to an array element.
  for(i = 0; i < array_size; i++) {
    array[i] += matrix[tid * array_size + i];
  }
  thread_mutex_unlock(&mutex);
  thread_exit(0);
}

// This test generates a matrix of array_size(columns) * num_threads(rows).
// Each thread uses tid to select a row in the matrix, and the row is added
// to a shared array that is protected by a mutex.
int test7(int repeat) {
  printf("Test #7: mutex lock on critical sections                 ");

  int i = 0, j = 0, sum = 0, passed = 0;

  matrix = (int*)malloc(array_size * num_threads * sizeof(int));
  array  = (int*)malloc(array_size * sizeof(int));

  while(repeat--) {
    memset(matrix, 0, array_size * num_threads * sizeof(int));
    memset(array,  0, array_size * sizeof(int));
    memset(thread, 0, max_threads * sizeof(thread_t));
  
    // The matrix is filled with random numbers between -9 and 9.
    for(i = 0; i < num_threads; i++) {
      for(j = 0; j < array_size; j++) { matrix[i * array_size + j] = urand() % 19 - 9; }
    }
  
    for(i = 0; i < num_threads; i++) {
      if(thread_create(&thread[i], &test7_func, &tid[i])) { panic("thread_create failed"); }
    }
    for(i = 0; i < num_threads; i++) {
      if(thread_join(thread[i], 0)) { panic("thread_join failed"); }
    }
  
    for(j = 0; j < array_size; j++) {
      sum = 0;
      for(i = 0; i < num_threads; i++) { sum += matrix[i * array_size + j]; }
      if(array[j] != sum) { break; }
    }
 
    // i and j indices should have reached the end of matrix. 
    if(!(passed = ((i == num_threads) && (j == array_size)))) { break; }
  }
  printf("(%s)\n", passed ? "pass" : "fail");

  free(matrix); matrix = 0;
  free(array);  array = 0;

  return passed;
}


int main(int argc, char **argv) {
  // User-specified test#
  int test_vec = 0;
  for(int i = 1; i < argc; i++) {
    for(int j = 0; argv[i][j]; j++) {
      test_vec |= (0x1 << (argv[i][j] - '0'));
    }
  }
  // Test all, if no test# is specified.
  test_vec = (test_vec == 0 ? 0xfe : test_vec);

  int repeat, passed;
  thread_mutex_init(&mutex);

  // Initialize the thread and tid arrays.
  thread = (thread_t*)malloc(max_threads * sizeof(thread_t));
  tid = (int*)malloc(max_threads * sizeof(int));
  for(int i = 0; i < max_threads; i++) { tid[i] = i; }

  // Tests are repeated many times to hit possible race conditions.
  repeat = 200; passed = 1;
  if((test_vec = test_vec >> 1) & 0x1) { passed &= test1(repeat);   }
  if((test_vec = test_vec >> 1) & 0x1) { passed &= test2(repeat);   }
  if((test_vec = test_vec >> 1) & 0x1) { passed &= test3(repeat);   }
  if((test_vec = test_vec >> 1) & 0x1) { passed &= test4(repeat);   }
  if((test_vec = test_vec >> 1) & 0x1) { passed &= test5(repeat);   }
  if((test_vec = test_vec >> 1) & 0x1) { passed &= test6(repeat);   }
  if((test_vec = test_vec >> 1) & 0x1) { passed &= test7(repeat/5); } // Test 5x less.
  printf("%s\n", passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED");

  free(thread); free(tid);
  thread_mutex_destroy(&mutex);

  exit(0);
}
