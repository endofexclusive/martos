M - A Real-Time Operating System
================================

This microkernel is designed with the following aims and goals.
* Easy to use and understand
* Powerful inter-process communication facilities
* Straight forward implementation
* Portability

MARTOS has support for signalling between tasks, timers,
semaphores, message queues with blocking and non-blocking
operations.

The kernel is platform independent and easy to port.  Ports
exist for ARM Cortex-M4 (STM32F407/417 microcontroller as
found on the STM32F4DISCOVERY development board).

[lwIP](http://savannah.nongnu.org/projects/lwip/), a TCP/IP-stack, has been ported for use
with MARTOS.

MARTOS is licensed under the 3-clause BSD license. Martin
Åberg, `martin@fripost.org`, is the author.


Feature overview
----------------

### Lists and nodes

Every operating system has to maintain and manipulate several
data structures and lists. The approach taken by MARTOS
for lists, fifos and queues is to use a fixed set of list
manipulation functions.

These list functions are used all over the place in the
kernel, indepedent of the actual list data content. So task
lists, message queues, semaphore queues etc use the same
list operations. This is achieved by having all kernel data
structures include a *list node* structure. In effect this is
inheritance as used in the object oriented programming paradigm.


### Tasks and scheduling

MARTOS scheduler is preemptive and priority based. The following
invariant is always true:

*The running task always has a priority at least as high as
every other task ready for execution.*

If more than one task have the highest priority and are
ready, then thse tasks are given access to the processor in
a Round-robin fashion with equal time slices.


### Signals

The most basic inter-process communication (IPC) primitive is
the signal system.  Each task can block waiting on a specific
task private signal. When another task or interrupt signals
the waiting task, it wakes up the task and makes ready for
execution again. It is implemented by setting, masking and
testing bits in signal fields in the task structure.

All other IPC mechanisms build on top of this and allows for a
task to wait for one of multiple sources in the same blocking
call. An example is to wait for a message arrival with timeout.


### Semaphores

MARTOS supports counting semaphores.


### Messages and queues

A task can own multiple message ports. Each message port
has a FIFO queue where messages can be put by other tasks or
by an interrupt routine. Sending a message is a non-blocking
operation. But, with the proper contract between a sender and a
receiver, sending as a blocking operation can also be achieved.
Tasks can block on one or several of its message ports.

Message content is fully user defined and in fact the message
structure does not contain any payload information at all; it
is up to the communicating tasks to agree on content (pointer,
specific data structure etc).


### Timers

With timers a user task can be delayed for a specific interval
or it can set an alarm so that it is waken up at a specific
system time. There are no limitations on the number of timers
in the system.


### Portability

There are no platform dependent code in the OS implementation.
Instead a portability interface is exported which describes the
data structures and the functions that needs to be implemented
by the integrator (`src/platform_protos.h`).


### No arbitrary number limits

There are no limits on the number of user tasks, messages,
timers etc in the kernel. All such things are built up using
doubly linked lists.

Also time spent in kernel calls is easily bounded when the
number of tasks in the system is known. Actually, the there
are only two mechanisms in the kernel whose time complexities
do depend on variable data. The first is the enquing of tasks
in priority order into the list or ready tasks. The other is
when a task adds a timer request.

Dynamic memory allocation may be used, but is not directly
supported by the OS. The *MARTOS way* of doing it is to create
a server task which calls `malloc()`/`free()` according to
some message passing protocol. An alternative is to protect
memory allocation using a semaphore.

