#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// A xv6-riscv syscall can take up to six arguments.
#define max_args 6

// Print a help message.
void print_help(int argc, char **argv) {
  fprintf(2, "%s <options: pid or S/R/X/Z>%s\n",
             argv[0], argc > 7 ? ": too many args" : "");
}

int main(int argc, char **argv) {
  // Print a help message.
  if(argc > 7) { print_help(argc, argv); exit(1); }

  // Argument vector
  int args[max_args];
  memset(args, 0, max_args * sizeof(int));

  /* Assignment 2: System Call and Process
     Convert the char inputs of argv[] to integers in args[].
     In this skeleton code, args[] is initialized to zeros,
     so technically no arguments are passed to the pstate() syscall. */
  
  // Convert the character input arguments (state or process ID options) to integers.
  for(int i = 1; i < argc; i++) {   // argv[0] is always "ps".
    // If argv[i] consists only of non-digit characters, then atoi(argv[i]) = 0.
    if(!atoi(argv[i])) {
      if(!strcmp(argv[i], "S")) { args[i-1] = -1; }         // SLEEPING
      else if(!strcmp(argv[i], "R")) { args[i-1] = -2; }    // RUNNABLE
      else if(!strcmp(argv[i], "X")) { args[i-1] = -3; }    // RUNNING
      else if(!strcmp(argv[i], "Z")) { args[i-1] = -4; }    // ZOMBIE
      else { print_help(argc, argv); exit(1); }             // Invalid characters
    }
    else if(atoi(argv[i]) > 0) {
      // Check if argv[i] contains non-digit characters.
      char *a = argv[i];
      int flag = 0;
      for(; *a != 0; a++) {
        if(*a < '0' || *a > '9') { flag = 1; break; }       // If argv[i] contains non-digit characters, flag = 1.
      }

      if(!flag) { args[i-1] = atoi(argv[i]); }              // Valid process ID
      else { print_help(argc, argv); exit(1); }             // Contains invalid characters
    }
    else {
      print_help(argc, argv); exit(1);
    }
  }

  // Call the pstate() syscall.
  int ret = pstate(args[0], args[1], args[2], args[3], args[4], args[5]);
  if(ret) { fprintf(2, "pstate failed\n"); exit(1); }

  exit(0);
}
