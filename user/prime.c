#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/sid.h"

int main(int argc, char **argv) {
  if(argc != 2) {
    printf("Usage: %s [Number]\n", argv[0]);
    exit(1);
  }

  int n = atoi(argv[1]);

  // A slow way of finding the Nth prime to
  // make a compute-bound job
  int i, j, k = 0;
  for(i = 2; k < n; i++) {
    for(j = 2; j < i; j++) {
      if((i % j) == 0) { break; }
    }
    if(i == j) { ++k; }
  }

  printf("%dth prime number = %d\n", n, --i);

  exit(0);
}

