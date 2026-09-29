# Lab 3 - Scheduling and Threading

The purpose of this lab is to introduce you to the concepts of
scheduling and concurrency. This lab consists primarily of three parts:  
- First, you will be extending xv6's scheduler to support multiple new schedulers.
- Second, you will be constructing a kernel-space threading library. 
- Third, you will be implementing a number of synchronization primitives.

This is a *large* lab, larger than the labs you've done so far, so be warned! 

To help you stay organized, we introduced a bonus "checkpoint" opportunity. 
Your submissions will automatically be evaluated on Parts 1 and 2 (tests 1-21).
Each test you pass in that range by the checkpoint deadline will count towards
a checkpoint bonus!

To give a sense of how long it may take to complete each part, we've marked
them with the following labels:
- Easy: 30 minutes to 1 hour
- Moderate: 1 to 5 hours
- Hard: 5 to 9 hours

Note that these are just estimates. Your completion time will vary based on
your grasp of the course material and proficiency with navigating xv6. We
highly recommend that you read chapters 3 to 5 of the xv6 manual.

**Important**: If you submit without completing all the parts, make sure to
implement all syscall stubs mentioned in this lab to prevent `make failure`. We
**cannot** give submissions back due to failed compilations.

> **NOTE:** Throughout this lab, you will have to test the concurrency of your
> system. You will have to run the `xv6-qemu` script with multiple cpus (default
> is 1 cpu) using the `-c <CPUS>` or `--num-cpus=<CPUS>` flags. We recommend
> initial debugging with 1 cpu, to make things easier to parse, then further
> debugging with additional cpus.

```
// Make sure you see the following lines of code
// when running ./xv6-qemu -c 4 for example
cpu1: starting 1
cpu2: starting 2
cpu3: starting 3
cpu0: starting 0
```

## Part 1 (moderate) - Scheduling

In this portion of the lab we will define a basic scheduler API, and build several new
schedulers.

#### Background
Recall the xv6 scheduler is found in `kernel/src/proc.c`, in the `scheduler()` function, and by
default implements a Round Robin (RR) scheduler. After each CPU is setup, all
eventually reach `mpmain()`, where `scheduler()` is called for the first time.
`scheduler()` loops over the process table in order looking for `RUNNABLE`
processes. When a `RUNNABLE` process is found, the kernel switches to that
process. It executes until it finishes or until a timer tick interrupts it and
causes the process to `yield()`.

Remember that the timer is a hardware interrupt. The code for handling this is
found in `kernel/src/trap.c`, around line 105.

```
// Force process to give up CPU on clock tick.
// If interrupts were on while locks held, would need to check nlock.
if (proc && proc->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER)
  yield();
```

`yield()` sets the current process to `RUNNABLE` and then switches directly to the
kernel scheduler.

### The Spec

For this portion of the lab, you will be enabling the user-space to specify
their scheduler policy. You will:

- Enable the user-space to select their scheduling policy (`SCHED_RR`, or
  `SCHED_FIFO`)
- Enable the user-space process to set a priority.
- Implement two new schedulers: Round Robin with Priority and FIFO with Priority


#### Added System Call

To enable the user-space to set their scheduler policy, you will be adding one
system-call to the kernel:

```
int setscheduler(int pid, int policy, int priority);

Arguments:
pid - the pid of the process to change priority (a process may only change the
scheduler of itself or its direct children)
policy - the scheduler policy (SCHED_RR, SCHED_FIFO)
priority - the new priority value (any non-negative int is allowed)

setscheduler should be declared in a program by including "user.h" and
should be defined within the user-space ulib library (ulib_SOURCES
in user/Sources.cmake).
```

A user-space program should also be able to use the macros `SCHED_RR` and
`SCHED_FIFO` by including the file `include/sched.h` (typically through the
pre-processor directive `#include "sched.h"`).

#### Scheduling Algorithms

You will be building two schedulers for this lab, a Round Robin (RR), and a
First In First Out (FIFO) scheduler. We will also be adding a notion of
priority to these schedulers. We will explain the behaviors of these
schedulers without priority first, then add the notion of priority after.

##### Round Robin

Your Round Robin scheduler will logically create a circular buffer of processes
to loop over. It will run each process in the buffer for either one scheduling
quantum (unit of scheduling), or until the process becomes non-runnable. Then,
it will select the next process in the circular buffer and repeat.

For this lab you are to implement a Round Robin scheduler, much like the default
xv6 scheduler (note the default xv6 scheduler is a reasonable RR baseline, and you
may directly use that code, particularly for your non-priority scheduler). Your
scheduler must keep a circular buffer of processes, then run those processes in order,
assigning a scheduling quantum to each one. Once that quantum has expired, the scheduler
should run the next available process.

##### First In First Out

The second scheduler you are to construct is the First In First Out (FIFO) scheduler.
The FIFO scheduler logically keeps a list of processes, then runs them in order. Unlike
the RR scheduler, as long as the process at the head of the FIFO queue can make progress,
it will not be preempted unless a higher priority process comes along (see the **Priorities**
section).

There are two major differences between RR and FIFO.
- In a FIFO scheduler, the current process will not yield to another process of the same priority
until it becomes non-runnable.
- FIFO processes will always run with higher priority than RR processes (if there are any
runnable FIFO processes, they should run before any RR processes).

##### Priorities

Now that we've outlined the basics of FIFO and RR scheduling, we'll specify
how priorities should work.

Each process has both a scheduler policy and priority. When the scheduler selects a process, it
should obey the following rules:

- FIFO processes always run before RR processes.
- Higher priority values correspond to higher logical priority.
- A process will not be scheduled if a higher priority process is runnable. 
- If two processes share priority, then they will run in scheduler order
  (as specified in the scheduler specification).
- When a new process becomes runnable, if it should run before the current
process, your scheduler should immediately preempt the currently running process and
schedule it (with one exception, in "Nit").
- FIFO and RR ordering should be based on when the process is created, **NOT** when it arrives
in the scheduler queue.


##### Nit:
Do not tamper with the APIC `TIMER` or `PERIODIC` as you will be modifying the 
external timer which interrupts the CPU for scheduling decisions.

If another process becomes a better candidate than the currently running process,
the kernel must immediately switch to running that process.

There is one exception to this rule. If there are multiple CPUs active, and an
action on CPU `c1` running process `p1` causes a process to become
runnable that is higher priority than process `p2` currently running on a different
CPU `c2` but not high enough to preempt `p1`, then `c2` need not preempt `p2` until
either `c2` is interrupted, `p2` completes, or `p2` is suspended. (In other words,
you don't have to implement CPU spidey-senses to somehow detect that a higher-priority
process was spawned by a *different* CPU and instantly preempt the current process.)

*tl;dr: CPUs shouldn't affect other CPUs!*

#### Default Behavior

All processes should default to `SCHED_RR` with a priority of 0.

## Part 2 (hard) - Threading

What is a thread, and how do we build it? Like a process, a thread represents
an independent execution context. However, where processes have memory isolation,
a thread shares its address space with all others in its thread group (we'll
define this term soon).

In this part of Lab 3, you will be adding threading support to xv6. Like Linux,
you'll be treating threads as processes, except for the fact that they share
memory, and a few other resources, with their neighboring threads.

### The birth of `clone`

First, you'll need a way to create a new thread (a process that shares address
space with its parent). For this lab, we'll be accomplishing this with our
version of the classic system call `clone()`:

```
int clone(void *stack, int stack_size)

Arguments:
  stack - a pointer to the beginning of a memory region of size stack_size, to
           be used as the new thread's stack
  stack_size - the size of the new thread's stack in bytes

Return:
  As with fork, clone returns twice on success (in the child and the parent).
  The returned values are:
    Parent - the pid of the child.
    Child - 0

  On error, clone returns exactly once, with the value -1 (no child is created).

Behavior:
  On success, clone creates a new child process which shares its address space with the
  current process. Additionally, clone sets up the child's stack to be logically
  equivalent to the parent's stack. Initially, the child's register state should be
  equivalent to that of the parent, with the exception of registers used for 
  the return value from clone (recall eax is the return value of a system call), 
  or holding information about the stack.
```

`clone()` creates a new process, and adds it to the caller's **thread group**.
Processes within a thread group all share the same address space. Thread
groups are created via the `fork` or `exec` system calls. The first
process within a thread group (the `fork`ed or `exec`'d process) is the thread
group's owner. If the owner of a thread group terminates before the other
threads in the group, the behavior for those threads is undefined.

`clone` should additionally follow these rules:

- `clone` should fail cleanly on errors. If `clone` cannot run (for instance, if the stack size
  is not large enough), it should return -1.

- Cloned processes share several resources with their parent, namely:
   - Virtual address space (shared memory)
   - File descriptor table
   - Current working directory

When any thread makes a change to a shared resource (such as writing to memory,
allocating new memory, or changing the directory) that change should be visible
to all threads in that thread group. 

**NOTE:** `clone` sets up its stack to be logically equivalent to its parents; however, it
cannot just `memcpy` the stack. What do you know about stacks that limits you
from doing this (think back to Lab 1's backtrace)? How must `clone` adjust?

### Waiting on specific processes

xv6 has a `wait` system call that waits on any child process. This is useful
when we don't know which children we want to wait on, but can result in
non-deterministic results. As you will see below, we sometimes need to be able to wait on specific processes.

You will add support for this by implementing the `waitpid()` system call:
```
int waitpid(int pid)

Arguments:
  pid - PID of the process/thread to wait on

Return:
  -1 on error, 0 on success

Behavior:
  Wait on the process/thread with process id = pid. If the PID doesn't exist,
  return -1 without waiting.
```

### Nits

- All threads within a thread group share all shared resources.
- If a thread finishes before its children, the behavior of the child threads is
undefined (so don't worry about testing that case).

## Part 3 (moderate) - Beginnings of a user-space threading library

We now have sufficient support from the kernel to start building a user-space
threading library.

You will now implement the following user-space library functions to allow users
to easily create and wait on threads. These functions are defined in
`user/src/threads.c`.
```
Function: thread_create
Arguments:
  start_routine - A function pointer to the routine that the child thread will run
  arg - the argument passed to start_routine
Return:
  -1 on failure, PID of the created thread on success
Description:
  Creates a new child thread. That thread will immediately begin running start_routine,
  as though invoked with start_routine(arg).

Definition:
int thread_create(void *(*start_routine)(void *), void *arg);


Function: thread_wait
Arguments:
  pid - pid of the thread to wait on
Return:
  -1 on failure, PID of the joined thread on success
Description:
Waits for a child thread of process id = pid to finish.

Definition:
int thread_wait(int pid);
```

These functions are declared in `user/include/threads.h`. You will implement
them in `user/include/threads.c`. Be warned, despite this simple interface,
these functions actually have tricky implementations, particularly when
attempting to safely avoid memory leaks.

**Important: Lab 3's thread library has some rather tricky behavior related to
deallocating its stack. We provide you with a small assembly segment which
atomically calls "free" of the stack of currently running thread, then calls
exit safely. You may use this code in your project. (To use
`free_stack_and_exit`, include the header "free_stack_and_exit.h". The source
code for it can be found in `user/asm/free_stack_and_exit.S`)**

### Nits

- Since `thread_wait` takes in a PID, users can pass in the PID of the process
  itself (and not that of a thread created by it). This is fine.

## Part 4 (easy) - User-space spinlocks

Now you will extend your user-space library by implementing spinlocks.

You will implement the following functions:
```
Function: spinlock_init
Arguments:
  s - a pointer to the spinlock to initialize
Return:
  -1 on failure, 0 on success
Description:
Initializes a spinlock.

Definition:
int spinlock_init(struct spinlock *s);


Function: spinlock_acquire
Arguments:
  s - a pointer to the spinlock to acquire
Return:
  -1 on failure, 0 on success
Description:
Acquires a spinlock. If the spinlock is already locked, the calling thread spin
until the lock is available.

Definition:
int spinlock_acquire(struct spinlock *s);


Function: spinlock_release 
Arguments:
  s - a pointer to the spinlock to release
Return:
  -1 on failure, 0 on success
Description:
Releases a spinlock.

Definition:
int spinlock_release(struct spinlock *s);
```

These functions are declared in `user/include/threads.h`. You will implement
them in `user/include/threads.c`. As discussed in class, implementing
synchronization primitives is tricky. In particular, you will need to use
atomic instructions to avoid data races. To that end, we've patched C11 atomics
to the user-space implementation of xv6. You can find the corresponding header
file at `user/include/atomics.h` (which you can subsequently include using
`#include "atomics.h"` in user-space). 

## Part 5 (hard) - User-space mutexes

With spinlocks you can now write multi-threaded code that protects its critical
sections. Spinlocks, however, can be inefficient if the lock is heavily
contended because you waste CPU cycles by having multiple threads contending
for the lock spin in a loop.

Instead of spinning we can put the process to sleep if the lock has been
acquired by some other process and wake it up when released. Locks that exhibit
this behavior are commonly referred to as **mutexes** (mutual exclusion locks).

For this part we will be implementing a user-space mutex. Implementing a mutex
in user-space is tricky due to the **Lost Wakeup Problem**, in which a process is
notified to wakeup right before it goes to sleep, thereby losing the
notification and sleeping indefinitely.

To address this issue, we will be implementing the following system calls:
```
Function: park
Arguments:
  chan - The channel to sleep on
Return:
  -1 on failure, 0 on success
Description:
Puts a process to sleep on channel chan

Definition:
int park(void *chan);


Function: setpark
Arguments:
  chan - The channel to sleep on
Return:
  -1 on failure, 0 on success
Description:
Signals that a process intends to sleep. Does not actually put the process to sleep!

Definition:
int setpark(void *chan);


Function: unpark
Arguments:
  chan - The channel to sleep on
Return:
  -1 on failure, Number of processes woken up on success
Description:
Wake up at most one process sleeping on channel chan.

Definition:
int unpark(void *chan);
```

Once you have these system calls in place, use them to implement the following
functions in `user/src/threads.c`:
```
Function: mutex_init
Arguments:
  m - a pointer to the mutex to initialize
Return:
  -1 on failure, 0 on success
Description:
Initializes a mutex.

Definition:
int mutex_init(struct mutex *m);


Function: mutex_acquire
Arguments:
  m - a pointer to the mutex to acquire
Return:
  -1 on failure, 0 on success
Description:
Acquires a mutex. If the mutex is already locked, the calling thread will sleep until the mutex is available.


Definition:
int mutex_acquire(struct mutex *m);

Function: mutex_release
Arguments:
  m - a pointer to the mutex to release
Return:
  -1 on failure, 0 on success
Description:
Releases a mutex. If there are any threads waiting on the mutex, one of them will be woken up.

Definition:
int mutex_release(struct mutex *m);
```

### Nits

- `setpark(void *chan)` doesn't put the process to sleep. It only informs the
  kernel that the process is _about to go to sleep_ in the near future. You
  will need this to solve the Lost Wakeup Problem. Additionally, there is no
  guarantee that `setpark` is called: it could happen that a thread does not decide
  to use `setpark` and rather just uses `park` and `unpark`. In that case, you must not
  rely on `setpark` being called.
- `unpark(void *chan)` doesn't specify which process to wake up. We leave this
  choice to you.

## Part 6 (easy) - User-space conditional variables

While spinlock and mutex synchronization work well, sometimes we need a
synchronization pattern similar to a producer-consumer queue. Instead of
spinning on a spinlock or yielding the CPU in a mutex, we would like the thread
to sleep until certain condition is met. Condition variables give us this
abstraction.

Implement the following functions in `user/src/threads.c`:
```
Function: cond_init 
Arguments:
  cond - a pointer to the condition variable to initialize
Return:
  -1 on failure, 0 on success
Description:
Initializes a condition variable.

Definition:
int cond_init(struct condvar *cond);


Function: cond_wait 
Arguments:
  cond - a pointer to the condition variable to wait on
  m - a pointer to the mutex to acquire
Return:
  -1 on failure, 0 on success
Description:
Atomically blocks the current thread waiting on the condition variable cond,and releases the mutex m. The waiting thread unblocks only after another thread calls cond_signal. After being woken up the current thread reacquires the mutex
m.


Definition:
int cond_wait(struct condvar *cond, struct mutex *m);


Function: cond_signal
Arguments:
  cond - a pointer to the condition variable to signal
Return:
  -1 on failure, 0 on success
Description:
Unblocks one thread waiting for the condition variable cond.

Definition:
int cond_signal(struct condvar *cond);
```

## General Guidance

Recall, a kernel's responsibility is to provide high-level abstractions to the
user-space. A user should never be able to break the abstractions
provided by the kernel. As such, user input to the kernel (such as system calls) should
not allow the user-space to execute arbitrary code on the user's behalf, modify
arbitrary kernel memory, or crash the kernel. The autograder will try to crash
your kernel by providing unexpected user-space input! You should protect against
bad input that comes from user-space, just as a real kernel must protect against
malicious users.

Also, the kernel persists throughout the lifetime of the machine. As a result,
any OS code should be free of memory leaks and data-races. Your code should
error out correctly when given bad inputs, and shouldn't leak resources (memory,
process table entries, file descriptors, etc), even in the instance of failures.

Lastly, we encourage you to have fun while implementing it. It may seem daunting
at first, but rest assured that you have everything you need to do well in this
lab. Make good use of lectures, Piazza, and office hours; we're here to help!

## Grading

#### Autograder

As usual, you will submit this lab to the autograder. The testcases are shown below:

- Scheduling Tests
  - Test 1-10
- Clone Functionality
  - Tests 11-15
- Clone Limits
  - Tests 16-18
- Clone Error/Security
  - Test 19-21
- Thread Library General Testing
  - Tests 22-25
- Thread Library I/O Blocking
  - Test 26
- Thread Library Error/Security
  - Test 27-30
- Thread Address Space
  - Test 31
- Spinlock
  - Test 32
- Park, Setpark, Unpark
  - Tests 33-35
- Mutex
  - Test 36
- Waitpid
  - Test 37
- Conditional Variables
  - Tests 38-41

**IMPORTANT**: Since this lab has a good deal of concurrency involved, you may
be able to pass some tests without correctly implementing some of these
primitives. We will be looking over your submission when hand-grading, so
please thoroughly test your implementation.

#### Hand Grading
Similar to previous labs, there is a hand graded section of the lab. We will check for
the following:
- Mutual exclusion in newly created kernel data structures
- Attempts to subvert the autograder
- Violations of the student honor code

#### Point Breakdown
- 100 pts - Autograder
- 25 pts - Hand Grading
- 6 pts - Checkpoint

Total lab grade is 125pts, so the maximum attainable score is 131pts/125pts.
The checkpoint will only consist of Parts 1 and 2, which are testcases 1-21, and
will be percentage-based, meaning that you will receive points relative to the percentage 
of passing tests in the range 1-21. All submissions are to be made in the Lab 3 autograder.
