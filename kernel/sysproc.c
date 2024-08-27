#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_pstate(void)
{
  // EEE3535 Operating Systems
  // Assignment 2: System Call and Process

  int call = ticks;   // pstate() call time

  struct proc *p;

  // Retrieve the input arguments of the pstate() syscall.
  // args[0] to args[5] are stored in registers a0 to a5.
  // Read function argument registers from a0 to a5 using argint().
  int args0, args1, args2, args3, args4, args5;
  argint(0, &args0);  // args[0]
  argint(1, &args1);  // args[1]
  argint(2, &args2);  // args[2]
  argint(3, &args3);  // args[3]
  argint(4, &args4);  // args[4]
  argint(5, &args5);  // args[5]

  // Print out a header line.
  printf("PID\tPPID\tState\tRuntime\tName\n");

  // Repeat for all processes in the process list.
  for(p = proc; p < &proc[NPROC]; p++) {
    // elapsed runtime = time difference between the process creation and the pstate() call
    int runtime = call - p->creation;

    // Check whether any of the input arguments match the process ID of p.
    int pid_check = (args0 == p->pid || args1 == p->pid || args2 == p->pid || args3 == p->pid || args4 == p->pid || args5 == p->pid);

    // Get PPID.
    int ppid = 0;
    if(p->parent) { ppid = p->parent->pid; }

    // Print the information of processes.
    // If the command is a simple ps command (i.e. args0 = 0), print the information of all active processes.
    // If the command includes state or process ID options, print the information of all processes that satisfy the conditions.
    // Print runtime in the 1:23.4 (1 minute 23.4 seconds) format. (1 tick = 0.1s, 600 ticks = 1 min)
    switch(p->state) {
      case SLEEPING:
        if(args0 == 0 || pid_check || args0 == -1 || args1 == -1 || args2 == -1 || args3 == -1 || args4 == -1 || args5 == -1) {
          printf("%d\t%d\tS\t%d:%d.%d\t%s\n", p->pid, ppid, runtime / 600, (runtime / 10) % 60, runtime % 10, p->name);
        } break;
      case RUNNABLE:
        if(args0 == 0 || pid_check || args0 == -2 || args1 == -2 || args2 == -2 || args3 == -2 || args4 == -2 || args5 == -2) {
          printf("%d\t%d\tR\t%d:%d.%d\t%s\n", p->pid, ppid, runtime / 600, (runtime / 10) % 60, runtime % 10, p->name);
        } break;
      case RUNNING:
        if(args0 == 0 || pid_check || args0 == -3 || args1 == -3 || args2 == -3 || args3 == -3 || args4 == -3 || args5 == -3) {
          printf("%d\t%d\tX\t%d:%d.%d\t%s\n", p->pid, ppid, runtime / 600, (runtime / 10) % 60, runtime % 10, p->name);
        } break;
      case ZOMBIE:
        if(args0 == 0 || pid_check || args0 == -4 || args1 == -4 || args2 == -4 || args3 == -4 || args4 == -4 || args5 == -4) {
          printf("%d\t%d\tZ\t%d:%d.%d\t%s\n", p->pid, ppid, runtime / 600, (runtime / 10) % 60, runtime % 10, p->name);
        } break;
      default:
        break;
    }
  }

  return 0;
}
