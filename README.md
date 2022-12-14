# README for Assignment 4

# System calls -

## Trace-

Adding a new system call called trace and accompanying user program called strace

Runs the command passed to it and records all the system calls that the command calls/uses during its execution. We also provide a mask which is a bit mask of the system calls to trace. 

Steps for implementation:

1. Add the trace_flag and trace_mask variables in kernel/proc.h to struct proc, we initialize these variables to 0 in kernel/proc.c in the allocproc function.
    
    We also make a copy of the trace_mask and trace_flag from the parent process to the child process in the fork function in kernel/proc.c.
    
2. Create a function sys_trace in kernel/sysproc.c, in which we check if the trace_flag is 0 and if it is, change it to 1. We also set the value of mask passed to the function as the mask for the process.

```c
uint64 sys_trace(void)
{
  if (myproc()->trace_flag == 0)
  {
    myproc()->trace_flag = 1;
    argint(0, &myproc()->trace_mask);
  }
  return 0;
}
```

1. We then modify the syscall function in kernel/syscall.c, we check whether the trace_flag for a process is set to 1, and if it is we use the trace_mask to track the required system calls.
    
    We use the syscallnames and the syscall_argcount arrays.
    
    ```c
    void syscall(void)
    {
      int num;
      struct proc *p = myproc();
    
      num = p->trapframe->a7;
      if (num > 0 && num < NELEM(syscalls) && syscalls[num])
      {
        int return_val_flag = 0;
        // Use num to lookup the system call function for num, call it,
        // and store its return value in p->trapframe->a0
        if (p->trace_flag == 1)
        {
          if (p->trace_mask >> num)
          {
            if (num != 2)
            {
              return_val_flag = 1;
              int arg_itr = syscall_argcount[num];
              if (arg_itr > 0)
              {
                printf("%d: syscall %s ( ", p->pid, syscallnames[num]);
    
                for (int i = 0; i < arg_itr; i++)
                {
                  int n;
                  argint(i, &n);
                  printf("%d ", n);
                }
              }
              else
              {
                printf("%d: syscall %s ( no arguments ", p->pid, syscallnames[num]);
              }
            }
          }
        }
        p->trapframe->a0 = syscalls[num]();
        if (return_val_flag == 1)
        {
          printf(") -> %d\n", p->trapframe->a0);
        }
      }
      else
      {
        printf("%d %s: unknown sys call %d\n",
               p->pid, p->name, num);
        p->trapframe->a0 = -1;
      }
    }
    ```
    
2. Then we create a file user/strace.c where we first error handle for the number of arguments and the value of the mask.
    
    Then we fork and execute the trace system call
    
    ```c
    int main(int argc, char *argv[])
    {
        if (argc <= 2)
        {
            printf("strace: wrong arguments\n");
            exit(1);
        }
    
        char *mask = argv[1];
    
        for (int i = 0; i < strlen(mask); i++)
        {
            if (mask[i] >= '0' && mask[i] <= '9')
            {
                // alright;
            }
            else
            {
                printf("strace: Invalid integer mask argument\n");
                exit(1);
            }
        }
    
        int integer_mask = atoi(mask);
    
        int pid = fork();
        if (pid == -1)
        {
            printf("strace: Error forking a child to run command\n");
            exit(1);
        }
    
        if (pid == 0)
        {
            trace(integer_mask);
    
            exec(argv[2], argv + 2);
            printf("strace: exec %s failed\n", argv[2]);
            exit(1);
        }
        wait(0);
        exit(0);
    }
    ```
    

## Sigalarm and sigreturn

We add a system call called sigalarm(interval, handler), on calling alarm(n, handler_function), after every n ticks of the CPU, the handler_function is called, after it returns, the application resumes where it had stopped

We also add a system call called sigreturn() to reset the process state to before the handler was called, the sigreturn() function is called at the end of the handle_function before it returns to allow the application to resume where it had stopped.

Steps for implementation:

1. Add the alarm_flag, alarm_interval, handler, ticks_passed variables to the kernel/proc.h to struct proc and initialize them to 0 in kernel/proc.c in the allocproc function.
2. We define functions sys_sogalarm and sys_sigreturn in kernel/sysproc.c.
    
    In sys_sigalarm, we check if the alarm_flag is 0 and if it is, we change it to 1. We also set the alarm_interval and handler for the current process to the arguments passed for the alarm.
    
    We also copy the trapframe of the process into a variable called trapframe_2. (We use this in the sys_sigreturn function to ensure the process can resume where it had stopped).
    
    ```c
    uint64 sys_sigalarm(void)
    {
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
    ```
    
    In sys_sigreturn, we copy back trapframe_2 of the process to trapframe which ensures that the process can resume at the point it was before we interrupted it to execute the handler function.
    
    We also reset the alarm_flag to 1 (which we set to 0 after the number of ticks passed is equal to the argument given as the interval).
    

```c
uint64 sys_sigreturn(void)
{
  memmove(myproc()->trapframe, myproc()->trapframe_2, sizeof(*(myproc()->trapframe)));
  myproc()->alarm_flag = 1;
  return myproc()->trapframe->a0;
}
```

1. In kernel/trap.c in the usertrap function we check if the alarm flag is 1, if it is we increment the ticks passed since the process began execution, and once the ticks passed is equal to the the interval that was passed as an argument to the system call we reset the ticks_passed to 0, the alarm_flag to 0 and we copy the trapframe of the current process to a backup and set the program counter to the alarm handler function to begin its execution.
    
    
    ```c
    if (p->alarm_flag == 1)
        {
          p->ticks_passed++;
          if (p->ticks_passed == p->alarm_interval)
          {
            p->ticks_passed = 0;
            p->alarm_flag = 0;
            memmove(p->trapframe_2, p->trapframe, sizeof(*(p->trapframe)));
            p->trapframe->epc = p->handler;
          }
        }
    ```
    

# Copy-on-write fork

Normally when a child process is forked, it requires access to the same memory the parent had. So the entire memory of the parent is copied and the child process accesses this memory.

In Copy-on-write fork we modify the code the implement the following functionality: 

Instead of copying the memory of the parent, we allow the child process to access the same memory as the parent and we also keep track of he nummber of processes accessing that page in memory. In addition to that, we also make the pages read-only, so if any process tries to alter them a page-fault exception is raised and a copy of the page is made and that copy is altered.

Steps for implementation:

1. We add the PTE_COW flag to the kernel/riscv.h file and we define a function that converts between page pointer and page index in an array we keep.
    
    ```c
    #define PTE_COW (1L << 8) // is a cow page
    ```
    
    ```c
    #define Pa2Ref_Idx(pa) ((PGROUNDDOWN(pa) - KERNBASE) / PGSIZE)
    ```
    
2. In the kernel/kalloc.c file we add an array of page indexes where each element tells us how many processes are accessing a page in memory.
    
    
    ```c
    struct
    {
      struct spinlock lock;
      struct run *freelist;
      char page_ref[(PHYSTOP - KERNBASE) / PGSIZE];
    } kmem;
    ```
    
    In the kinit function, we set the value of each element in the array to 0. 
    
    ```c
    void kinit()
    {
      initlock(&kmem.lock, "kmem");
      memset(kmem.page_ref, 0, sizeof(kmem.page_ref));
      freerange(end, (void *)PHYSTOP);
    }
    ```
    
    In the kalloc and freerange function when we assign a specific page to a process, we change the value for that page index to 1.
    
    ```c
    void freerange(void *pa_start, void *pa_end)
    {
      char *p;
      p = (char *)PGROUNDUP((uint64)pa_start);
      for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
      {
        kmem.page_ref[Pa2Ref_Idx((uint64)p)] = 1;
        kfree(p);
      }
    }
    ```
    
    ```c
    void * kalloc(void)
    {
      struct run *r;
    
      acquire(&kmem.lock);
    
      r = kmem.freelist;
    
      if (r)
      {
        kmem.freelist = r->next;
        kmem.page_ref[Pa2Ref_Idx((uint64)r)] = 1;
      }
    
      release(&kmem.lock);
    
      if (r)
        memset((char *)r, 5, PGSIZE); // fill with junk
      return (void *)r;
    }
    ```
    
    We define a function called increment_pageref which increments the page index in the array if more processes point to the page in memory.
    
    ```c
    void increment_pageref(uint64 pa)
    {
      if ((char *)pa < end || pa >= PHYSTOP)
      {
        panic("Increment Reference Count");
      }
    
      acquire(&kmem.lock);
    
      kmem.page_ref[Pa2Ref_Idx(pa)]++;
    
      release(&kmem.lock);
    }
    ```
    
    In the kfree function, we decrement the pointer for that page index, and free the page memory only if the number of processes accesing that page is 0.
    
    ```c
    void kfree(void *pa)
    {
      struct run *r;
    
      if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree");
    
      // Fill with junk to catch dangling refs.
      // memset(pa, 1, PGSIZE);
    
      r = (struct run *)pa;
      acquire(&kmem.lock);
      kmem.page_ref[Pa2Ref_Idx((uint64)pa)]--;
    
      if (!kmem.page_ref[Pa2Ref_Idx((uint64)pa)])
      {
        memset(pa, 1, PGSIZE);
        r->next = kmem.freelist;
        kmem.freelist = r;
      }
      release(&kmem.lock);
      return;
    }
    ```
    
3. In the kernel/trap.c file, in the usertrap function when a process tries to modify a cow page (which is made read-only) a page fault exception occurs and the page is copied.
    
    
    ```c
    else if (r_scause() == 15 && r_stval() < p->sz && cow_handler(p->pagetable, r_stval()) > 0)
      {
      }
    ```
    
4. In the kernel/vm.c file, we modify the uvmcopy function, instead of copyng the memory of the parent process and then giving it to the child, we let the child access the same memory as the parent, mark the pages it is accessing as cow pages and increment the number of processes accessing that page (the page index in the array).

```c
int uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  pte_t *pte;
  uint64 pa, i;
  uint flags;

  for (i = 0; i < sz; i += PGSIZE)
  {
    if ((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");
    if ((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present");
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);

    if (flags & PTE_W)
    {
      flags = (flags & (~PTE_W)) | PTE_COW;
      *pte = PA2PTE(pa) | flags;
    }

    increment_pageref(pa);

    if (mappages(new, i, PGSIZE, pa, flags) != 0)
    {
      goto err;
    }
  }
  return 0;

err:
  uvmunmap(new, 0, i / PGSIZE, 1);
  return -1;
}
```

We also modify the copyout function by calling the cow_handler function (custom written function to check for cow pages) and checking its return value (and based on the return value deciding whether to copy the memory to the specified address in the page table). 

```c
int copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
  uint64 n, va0, pa0;

  while (len > 0)
  {
    va0 = PGROUNDDOWN(dstva);

    if (cow_handler(pagetable, dstva) == -1)
    {
      return -1;
    }

    pa0 = walkaddr(pagetable, va0);
    if (pa0 == 0)
      return -1;
    n = PGSIZE - (dstva - va0);
    if (n > len)
      n = len;
    memmove((void *)(pa0 + (dstva - va0)), src, n);

    len -= n;
    src += n;
    dstva = va0 + PGSIZE;
  }
  return 0;
}
```

We write a function called cow_handler which checks whether a specific page in the page table is a cow page or not.

```c
int cow_handler(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  uint64 vpage, pa, flags;
  char *mem;

  if (va >= MAXVA)
  {
    return -1;
  }

  vpage = PGROUNDDOWN(va);

  pte = walk(pagetable, vpage, 0);
  if (pte == 0)
  {
    return -1;
  }
  if ((*pte & PTE_COW) == 0)
  {
    return 0;
  }
  if ((*pte & PTE_V) == 0)
  {
    return -1;
  }

  if ((mem = kalloc()) == 0)
  {
    return -1;
  }

  pa = PTE2PA(*pte);
  flags = PTE_FLAGS(*pte);
  flags = (flags | PTE_W) & (~PTE_COW);

  memmove(mem, (char *)pa, PGSIZE);
  *pte = PA2PTE(mem) | flags;
  kfree((void *)pa);

  return 1;
}
```

# Schedulers

Four schedulers, with flags: **FCFS**, **LBS**, **PBS**, and **MLFQ**, have been added to xv6 which has the default scheduler as Round Robin and others invoked with the appropriate flag during compile time.

## 1. **FCFS (First Come First Serve)**

To invoke this scheduler, run the following command:

```c
make qemu MODE=FCFS
```

Due to FCFS being non preemptive in nature, we stop calling the scheduler during timer interrupt by removing **yield( )** in **trap.c**

To keep track of which process ???comes??? first, we add to struct proc: creation_time which is set to **ticks** in allocproc( ).

In the scheduler function - scheduler( ) in proc.c - we iterate through runnable processes and find the one with the least creation time indicating the process that arrived first. 

[ Code snippet for the above]

```c
    int flag = 0;
    struct proc *first_created = 0;

    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);

      if (p->state == RUNNABLE)
      {
        if (first_created == 0)
        {
          first_created = p;
          flag = 1;
        }
        else
        {
          if (first_created->creation_time > p->creation_time)
          {
            first_created = p;
          }
        }
      }

      release(&p->lock);
    }
```

If a process had been selected (checked with the value of the flag), we acquire its lock again, change its state to RUNNING and switch context to the process???s memory.

Once it???s executed, we reset the cpu struct to get it ready for the next process to be scheduled.

[ Following code snipped shows the logic implemented ]

```c
if (flag == 1)
    {
      acquire(&first_created->lock);
      if (first_created->state == RUNNABLE)
      {
        first_created->state = RUNNING;
        c->proc = first_created;
        swtch(&c->context, &first_created->context);

        c->proc = 0;
      }
      release(&first_created->lock);
    }
```

## 2. **Lottery Based Scheduler**

To invoke this scheduler, run the following command:

```c
make qemu MODE=LBS
```

Every process has a certain number of tickets(tickets in struct proc) whose default value is set to 1 in allocproc( ) and to set it to any value desired by the process, the following system call is invoked.

### int settickets (int number)

```c
uint64 sys_settickets(void)
{
  int n;
  argint(0, &n);
  myproc()->tickets = n;
  return 0;
}
```

The probability of choosing a process to run is proportional to the tickets it owns. So for this scheduler, first iterate through the RUNNABLE processes and get the total number of tickets.

```c
    int total_tickets = 0;
    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == RUNNABLE)
      {
        total_tickets += (p->tickets);
      }
      release(&p->lock);
    }
```

Now, we pick a number at random in between [0 , total number of tickets] using a random integer generator. This value is scaled down to the range by taking ( % total number of tickets ).

The following random function is used whose seed is the **ticks value** at the time of invocation.

```c
int rand(uint64 next) // RAND_MAX assumed to be 32767
{
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % 32768;
}

// called and scaled in the scheduler function in the following way:

uint random_num = rand(ticks);
random_num = random_num % total_tickets;
```

To accomodate the probabilty factor in choosing a process, we tend to cumulate the ticket value of each RUNNABLE process and as soon as the total value becomes greater or equal to the random number picked, we schedule that process and break out of the iteration to prevent the next process from being scheduled.

In this way, processes with less tickets tend to have lesser probability of being run and the ones with the higher number of tickets, is more probable to be chosen to run.

```c
// code implementation

    total_tickets = 0;
    int flag = 0;

    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == RUNNABLE)
      {
        total_tickets += (p->tickets);
        if (total_tickets >= random_num)
        {
          flag = 1;
          p->state = RUNNING;
          c->proc = p;
          swtch(&c->context, &p->context);

          c->proc = 0;
        }
      }
      release(&p->lock);
      if (flag == 1)
      {
        break;
      }
    }
```

## 3. **PBS ( Priority Based Scheduler )**

To invoke this scheduler, run the following command:

```c
make qemu MODE=PBS
```

Due to the non preemptive nature of this scheduler, the function yield( ) is removed when timer interrupts occur.

In this case, every process now has a static priority value and niceness added to the proc structure with default values during process allocation as 60 and 5 respectively.

Since we require - ticks spent sleeping and ticks spent running - to calculate the niceness of a process after being scheduled, we add those entries too. To facilitate finding the former value, when a process calls sleep, we mark the time at that point when it goes to sleep in the **sleep( )**. And when it is woken up in **wakeup( )** , we mark the time and calculate the ticks spent sleeping as the difference between the two. It is at that point we change the niceness value using the given formula in the case that it hasn???t been scheduled yet ( sleeping time = 0 & running time = 0).

```c
// sleep()
  p->state = SLEEPING;
  p->sleep_time = ticks; // ticks noted

// in wakeup function, right after changing state to RUNNABLE
// the following change is done

p->state = RUNNABLE;
#ifdef PBS
        p->wakeup_time = ticks;
        p->sleeping_ticks = p->wakeup_time - p->sleep_time;
        if ((p->sleeping_ticks != 0) && (p->running_ticks != 0))
        {
          p->niceness = (int)((p->sleeping_ticks / (p->sleeping_ticks + p->running_ticks)) * 10);
        }
#endif
```

The ticks spent running is calculated by incrementing the count for every running process whenever a timer interrupt occurs:

```c
// trap.c - usertrap()

#ifdef PBS
    struct proc *itr;
    for (itr = proc; itr < &proc[NPROC]; itr++)
    {
      acquire(&itr->lock);
      if (itr->state == RUNNING)
      {
        itr->running_ticks++;
      }
      if ((itr->sleeping_ticks != 0) && (itr->running_ticks != 0))
      {
        itr->niceness = (int)((itr->sleeping_ticks / (itr->sleeping_ticks + itr->running_ticks)) * 10);
      }
      release(&itr->lock);
    }
#endif
```

Another thing to note: A process can change it???s static priority using the following system call:

### int set_priority(int new_priority, int pid)

```c
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
```

The implementation of it just iterates through the process and when it finds the process with the matching pid, the static priority is changed and rescheduling occurs by calling yield( ) if the priority has increased. Resetting of all the new entries added occurs too.

In case the process was found, the old priority is returned and if the process was not found, -1 is returned indicating an incorrect pid value.

Now in the actual scheduler function, we iterate through the RUNNABLE processes while calculating the dynamic priority of a process (using the formula given) on the go, and choose a process with highest priority (lowest dp value). 

In the case that 2 or more processes have the same priority, we choose the one which has been scheduled the lesser number of times.

??? This is done by adding a new entry to the struct proc and incrementing the counter every time the process is scheduled.

In case there is a tie in this value, we check the creation time (present in struct proc) and choose the one with a lesser creation time.

Below is the code implementation: 

```c
    struct proc *selected_proc = 0;
    int dp;
    int flag = 0;
    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == RUNNABLE)
      {
        if (selected_proc == 0)
        {
          flag = 1;
          selected_proc = p;
          int min_result = ((p->static_priority - p->niceness + 5) < 100) ? p->static_priority - p->niceness + 5 : 100;
          dp = (0 > min_result) ? 0 : min_result;
        }
        else
        {
          int min_result = ((p->static_priority - p->niceness + 5) < 100) ? p->static_priority - p->niceness + 5 : 100;
          int proc_dp = (0 > min_result) ? 0 : min_result;

          if (proc_dp < dp)
          {
            selected_proc = p;
            dp = proc_dp;
          }
          else if (proc_dp == dp)
          {
            if (p->number_of_times_scheduled < selected_proc->number_of_times_scheduled)
            {
              selected_proc = p;
              dp = proc_dp;
            }
            else if (p->number_of_times_scheduled == selected_proc->number_of_times_scheduled)
            {
              if (p->creation_time < selected_proc->creation_time)
              {
                selected_proc = p;
                dp = proc_dp;
              }
            }
          }
        }
      }
      release(&p->lock);
    }
```

If a process has been selected then it is scheduled similar to how it was in FCFS or LBS.

```c
if (flag == 1)
    {
      acquire(&first_created->lock);
      if (first_created->state == RUNNABLE)
      {
        first_created->state = RUNNING;
        c->proc = first_created;
        swtch(&c->context, &first_created->context);

        c->proc = 0;
      }
      release(&first_created->lock);
    }
```

## 4. **Multi-Level Feedback Queue ( MLFQ )**

To invoke this scheduler, run the following command:

```c
make qemu MODE=MLFQ
```

To implement this, the following modifications were made to struct proc:

```c
  int queue;
  int ticks_completed;
  int wait_time;
  int queue_index;
```

to keep track of which queue the process is in, the ticks completed in that specific queue(running time), the time it spends waiting in the queue and the queue index of that proc.

During process allocation, they are intialized to 0 except queue_index which is set to **ticks**.

??? We do not maintain an actual queue structure and hence not a real queue_index. Instead we use the notion of a process being first in a queue as the one having the least ticks stored in its queue_index.

Now in trap.c, we maintain the following array:

### int time_slice[5] = {1, 2, 4, 8, 16};

which indicates the total ticks that each process is allowed to have in each queue. 

```c
#ifdef MLFQ

    struct proc *itr;
    int flag = 0;
    for (itr = proc; itr < &proc[NPROC]; itr++)
    {
      acquire(&itr->lock);
      if (itr->state == RUNNING)
      {
        itr->ticks_completed++;
        if (itr->ticks_completed == time_slice[itr->queue])
        {
          flag = 1;
          if (itr->queue != 4)
          {
            itr->queue++;
            itr->ticks_completed = 0;
            itr->queue_index = ticks;
            itr->wait_time = 0;
          }
          else if (itr->queue == 4)
          {
            itr->ticks_completed = 0;
            itr->queue_index = ticks;
            itr->wait_time = 0;
          }
        }
      }
      else if (itr->state == RUNNABLE)
      {
        if (myproc() != itr)
        {
          if (itr->queue < myproc()->queue)
          {
            flag = 1;
          }
        }
        itr->wait_time++;
      }
      release(&itr->lock);
    }

    if (flag == 1)
    {
      yield();
    }

#endif
```

So when a timer interrupt occurs, we iterate through the processes and

1) If we find a RUNNING process, we increment the ticks it has completed. This is followed up by checking if the process has completed the time slice allocated to it in that queue and if yes: we increment the queue(only when it isn???t in the last queue), reset the queue_index to ticks so that it gets ???added??? to the tail of the next queue, reset the other parameters and also set the rescheduling flag.

2) If we find a RUNNABLE process, we increment the counter for waiting time and check if the process has higher priority than the one currently running( which would lead to rescheduling if true so the flag is set accordingly).

**yield( )** is called if the rescheduling flag has been set.

```c
for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == RUNNABLE)
      {
        if (p->wait_time > 1000)
        {
          if (p->queue != 0)
          {
            p->queue--;
            p->ticks_completed = 0;
            p->queue_index = ticks;
            p->wait_time = 0;
          }
        }
      }
      release(&p->lock);
    }
```

Now in the scheduler function, we first iterate through the processes to check if any process has waited for more than a 1000 ticks upon which it is pushed to the immediate higher priority queue while resetting it???s properties( queue_index is again set to ticks to add it to the tail of the new queue).

```c
    int queue_num = 10;

    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == RUNNABLE)
      {
        if (p->queue < queue_num)
        {
          queue_num = p->queue;
        }
      }
      release(&p->lock);
    }
```

Next, in the above code, we iterate through the processes to find the non-empty highest priority queue so that we can run it.

In the following code, we iterate through the processes which are present in the highest priority queue that we just found, and select the one at the head of the queue(least queue_index).

```c
    struct proc *selected_process = 0;
    int flag2 = 0;
    if (queue_num != 10)
    {
      for (p = proc; p < &proc[NPROC]; p++)
      {
        acquire(&p->lock);
        if (p->state == RUNNABLE)
        {
          if (p->queue == queue_num)
          {
            if (selected_process == 0)
            {
              flag2 = 1;
              selected_process = p;
            }
            else
            {
              if (p->queue_index < selected_process->queue_index)
              {
                selected_process = p;
              }
            }
          }
        }
        release(&p->lock);
      }
    }
```

Once we have selected the process, we reset the waiting time to 0 as now it is ready to be scheduled similar to how it???s done in the previous modes of scheduling like:

```c
if (flag2 == 1)
    {
      acquire(&selected_process->lock);
      if (selected_process->state == RUNNABLE)
      {
        // selected_process->last_run = ticks;
        selected_process->wait_time = 0;
        selected_process->state = RUNNING;
        c->proc = selected_process;
        swtch(&c->context, &selected_process->context);

        c->proc = 0;
      }
      release(&selected_process->lock);
    }
```

Important : There is a possibility of a process relinquishing the cpu to wait for IO and sleeps meanwhile. When it is woken up to be scheduled again, it should be added to the tail of the queue that it was in before going to sleep. This is account for by reseting the queue_index to ticks in the **wakeup( )** function.

```c
#ifdef MLFQ
        p->queue_index = ticks;
#endif
```

# TESTING SCHEDULERS

To test the schedulers, we check the average run time and wait time of a process using a userprogram.

## DEFAULT - Round Robin

Due to preemption in RR at every tick, the effect of the constant context switches and waiting to be rescheduled again, displays itself in the wait time.

```c
Average Run time: 11 ticks
Average Wait time: 111 ticks
```

## FCFS - First Come First Serve

Due to the absence of preemption, the overhead caused by context switches is non existent and can be seen in the low average wait time and a higher average run time. But if we had more cpu bound processes, then the average wait time would increase as the cpu would be hogged, leaving other processes in the queue to wait longer.

```c
Average Run time: 13 ticks
Average Wait time: 105 ticks
```

## LBS - Lottery Based Scheduling

LBS scheduler is preemptive in nature hence the higher wait time and the less number of tickets allocated for each process leads to a low probability of it being scheduled and thus the low average run time.

```c
Average Run time: 7 ticks
Average Wait time: 113 ticks
```

## PBS - Priority Based Scheduling

PBS scheduler gets the best of both worlds where it is non preemptive in nature as well as the processes call the set_priority system call to increase it???s priority, leading to more cpu time for each process and reducing the time it has to wait in the queue.

```c
Average Run time: 13 ticks
Average Wait time: 105 ticks
```

## MLFQ - Multi Level Feedback Queue

MLFQ scheduler is ideal when there is a variety of processes to run. Due to the similar nature of the processes, the benefits aren???t widely seen but compared to the other schedulers, it still does perform well.

```c
Average Run time: 13 ticks
Average Wait time: 108 ticks
```
