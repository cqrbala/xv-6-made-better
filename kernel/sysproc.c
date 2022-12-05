#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "stdlib.h"

extern struct proc proc[NPROC];

uint64
sys_waitx(void)
{
  uint64 addr, addr1, addr2;
  uint wtime, rtime;
  argaddr(0, &addr);
  argaddr(1, &addr1); // user virtual memory
  argaddr(2, &addr2);
  int ret = waitx(addr, &wtime, &rtime);
  struct proc *p = myproc();
  if (copyout(p->pagetable, addr1, (char *)&wtime, sizeof(int)) < 0)
    return -1;
  if (copyout(p->pagetable, addr2, (char *)&rtime, sizeof(int)) < 0)
    return -1;
  return ret;
}

uint64 sys_set_priority(void)
{
  int new_priority;
  int process_pid;
  int return_val = 0;

  argint(0, &new_priority);
  argint(1, &process_pid);

  struct proc *p;
  int change_flag = 0;
  int found_flag = 0;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);

    if (p->pid == process_pid)
    {
      found_flag = 1;
      if (p->static_priority > new_priority)
      {
        change_flag = 1;
      }
      return_val = p->static_priority;
      p->static_priority = new_priority;
      p->niceness = 5;
      p->sleeping_ticks = 0;
      p->sleep_time = 0;
      p->wait_time = 0;
      p->running_ticks = 0;
    }

    release(&p->lock);
  }

  if (found_flag == 0)
  {
    return -1;
  }

  if (change_flag == 1)
  {
    yield();
  }

  return return_val;
}

uint64 sys_settickets(void)
{
  int n;
  argint(0, &n);
  myproc()->tickets = n;
  return 0;
}

uint64 sys_trace(void)
{
  if (myproc()->trace_flag == 0)
  {
    myproc()->trace_flag = 1;
    argint(0, &myproc()->trace_mask);
    // printf("entered here\n");
  }
  return 0;
}

uint64 sys_sigalarm(void)
{
  // printf("arrived here\n");
  struct proc *currprocess;
  currprocess = myproc();
  if (currprocess->alarm_flag == 0)
  {
    int n;
    uint64 handlerfunc;

    argint(0, &n);
    argaddr(1, &handlerfunc);
    currprocess->alarm_interval = n;
    currprocess->handler = handlerfunc;
    currprocess->alarm_flag = 1;
  }
  return 0;
}

uint64 sys_sigreturn(void)
{
  memmove(myproc()->trapframe, myproc()->trapframe_2, sizeof(*(myproc()->trapframe)));
  myproc()->alarm_flag = 1;
  return myproc()->trapframe->a0;
}

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
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
  if (growproc(n) < 0)
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
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
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
