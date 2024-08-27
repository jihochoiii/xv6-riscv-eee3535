#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define num_ptr     10
#define num_loop    30
#define max_malloc  65536

int main(int argc, char** argv) {
  int* malloc_ptr[num_ptr];     // Array of malloc pointers
  int malloc_sz[num_ptr];       // Array of malloc-created block sizes

  // Initialize the arrays.
  memset(malloc_ptr, 0, num_ptr * sizeof(int*));
  memset(malloc_sz, 0, num_ptr * sizeof(int));

  // Test runs for num_loop times:
  // Case 1: If a memory block does not exist at random index i,
  //         create one with an arbitrary size between 1 and max_malloc.
  // Case 2: If a memory block exists at random index i, free it.
  for(unsigned n = 0; n < num_loop; ) {
    printf("Test #%d: ", ++n);
    unsigned i = urand() % num_ptr;
    if(!malloc_ptr[i]) { // Case 1
      unsigned sz = 0;
      while(!sz) { sz = urand() % max_malloc; }
      printf("malloc(%d bytes)\n", malloc_sz[i] = sz);
      malloc_ptr[i] = malloc(sz);
    }
    else { // Case 2
      printf("free(%d bytes)\n", malloc_sz[i]);
      free(malloc_ptr[i]);
      malloc_ptr[i] = 0;
    }
    // Show the chain of free blocks every 5 tests.
    if(!(n % 5)) { freelist(); }
  }

  // Clean up to avoid memory leaks.
  for(unsigned r = 0; r < num_ptr; r++) {
    if(malloc_ptr[r]) {
      free(malloc_ptr[r]);
    }
  }

  exit(0);
}

