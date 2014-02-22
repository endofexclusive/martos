/*
Copyright (c) 2014, Martin Åberg All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. The names of the copyright holder(s) may not be used to endorse or
   promote products derived from this software without specific prior
   written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MARTOS_H
#define MARTOS_H

#include <stdint.h>
#include <stdbool.h>
#include <platform.h>

/** \file */

typedef int16_t Node_Prio;
static const Node_Prio NODE_PRIO_MIN = INT16_MIN;
static const Node_Prio NODE_PRIO_MAX = INT16_MAX;
static const Node_Prio TASK_PRIO_MIN = INT16_MIN;
static const Node_Prio TASK_PRIO_MAX = INT16_MAX - 1;
static const Node_Prio TASK_PRIO_EXCLUSIVE = INT16_MAX;

typedef int_fast8_t SignalNumber;
typedef uint16_t Signals;
static const Signals SIGF_SINGLE = 0x0001;
static const int SIGNALS_WIDTH = 16;
typedef int_fast16_t NestCnt;


typedef struct Node_ {
    struct Node_ *next;
    struct Node_ *prev;
    /* name may be NULL, but then it can't be found by
    list_find() and similar functions. */
    char *name;
    Node_Prio prio;
} Node;


typedef struct MinNode_ {
    struct MinNode_ *next;
    struct MinNode_ *prev;
} MinNode;


/**
\brief Datatype for a list, double-ended queue and priority
queue.

List is implemented with a doubly linked list.
*/

typedef struct {
    MinNode head;
    MinNode tail;
} List;


/**
\brief Unlink node from list.

The modified list is implicit.

\param node The node to unlink.
*/
void list_unlink(Node *const node);


/**
\brief Prepare a list for list operations.

This function must be applied to the list before any other
list operation can take place.

\param list The list to initialize.
*/
void list_init(List *const list);


/**
\brief Test if list is empty.

\param list The list to check for emptiness.
\return true if list is empty, false otherwise.
*/
bool list_is_empty(List *const list);


/**
\brief Add node to end of list.

\param list The list to add the node to.
\param node The node to add.
*/
void list_add_tail(List *const list, Node *const node);


/**
\brief Remove head node of list.

\param list The list for which the head will be removed.
\return The removed node, or NULL if list was empty.
*/
Node *list_rem_head(List *const list);


/**
\brief Get head of list.

The list is not modified.

\param list The list to get head of.
\return Head of list.
*/
Node *list_get_head(List *const list);


/**
\brief Insert a node into a list.

The node is inserted in list after a given node.

\param list The list to insert into.
\param node The node to insert.
\param prev Insert node after this node. If NULL, then node
is inserted at head of list.
*/
void list_insert(List *const list, Node *const node, Node *const prev);


/**
\brief Enqueue node in list.

The node is inserted in list before the first node with a
priority lower than parameter node.

\param list The list to insert into.
\param node The node to insert.
*/
void list_enqueue(List *const list, Node *const node);


/**
\brief Find list node by name.

The first node matching name is returned. More nodes can be
searched by successively calling the function on the found
nodes.

\param start A list or node to start the search from.
\param name The name to match.
\return The first node with matching name, or NULL if none
was found.
*/
Node *list_find(List *const start, char *const name);


/**
\brief Disable interrupts.

Disabling of interrupts nest. Each disable() must be paired
with an enable().
*/
void disable(void);


/**
\brief Enable interrupts.

Each enable() must be preceded by a disable().
*/
void enable(void);


typedef enum {
    TASK_INVALID,
    TASK_INITIALIZED,
    TASK_RUNNING,
    TASK_READY,
    TASK_WAITING
} Task_State;

typedef struct {
    Node node;
    TaskContext context;
    Signals sig_alloc;
    /* sig_wait is valid only if state = TS_WAIT. */
    Signals sig_wait;
    Signals sig_recvd;
    NestCnt id_nestcnt;
    Task_State state;
} Task;


/**
\brief Prepare a task for scheduling.

Private Task fields are set. The task is not scheduled for
execution: use task_schedule() to do so.

\param task The task to initialize.
\param name String identifier of task. It may be NULL.
\param prio Pre-emptive scheduling priority of task. A larger
number gives higher scheduling priority. The lowest permitted
value is TASK_PRIO_MIN and the largest is TASK_PRIO_MAX.
\param init_pc Execution entry point of task. The routine must
never return.
\param user_data Optional parameter to execution entry point.
\param stack Pointer to start of the tasks stack area.
\param stack_size Size of tasks stack.
*/
void task_init(
    Task *const task,
    char *const name,
    const Node_Prio prio,
    void (*const init_pc) (void *user_data),
    void *const user_data,
    void *const stack,
    const uint32_t stack_size
);


/**
\brief Schedule a task for execution.

Add the task to the scheduler so that it may run.

\param task The task to schedule.
*/
void task_schedule(Task *const task);


/**
\brief Find task by name or find self.

System task queues are searched for a task with given name.

\param name The task name to match, or NULL to find the
calling task.
\return Pointer to the task with corresponding name, or NULL
if there was no match.
*/
Task *task_find(char *const name);


/**
\brief Set scheduling priority of a task.

\param task The task to set priority of.
\param prio New priority of task.
*/
void task_set_prio(Task *const task, const Node_Prio prio);


/**
\brief Allocate signal bit.

\param signal The signal bit, >=0, to allocate, or -1 for any
bit. The following must hold: -1 <= signals < SIGNALS_WIDTH.

\return Allocated signal bit in the range 0..SIGNALS_WIDTH -1,
or -1 if no signal bit could be allocated.
*/
SignalNumber signal_allocate(SignalNumber signal);


/**
\brief Free signal bits.
*/
void signal_free(const Signals signals);


/**
\brief Send bit signals to a task.

This function is callable from interrupt context. */
void signal_send(Task *const task, const Signals signals);


/**
\brief Wait for bit signals.
*/
Signals signal_wait(const Signals signals);


typedef struct {
    Task *owner;
    /* This list contains SemaphoreRequests. */
    List req_queue;
    int16_t nestcnt;
} Semaphore;

typedef struct {
    MinNode node;
    Task *waiter;
    Signals signal;
} SemaphoreRequest;

void sem_free(Semaphore *const sem);
void sem_obtain(Semaphore *const sem);
/**
\brief Add a signal request to a semaphore queue.

The purpose of this function is to make it possible to do
a signal_wait() where a semaphore is one of many signal
sources. This allows for semaphores with time-out, blocking
on one of many semaphores etc.
*/
bool sem_add_request(Semaphore *const sem, SemaphoreRequest *const req);
void sem_release(Semaphore *const sem);


/**
\brief User entry point to system.

The kernel is running multitasking when this function is called
so all system calls can be used. This is the place to schedule
user tasks. It is safe to return from this function.
*/
void user_init(void);


typedef enum {
    MSGPORT_SIGNAL,
    MSGPORT_IGNORE
} MsgPort_Action;

typedef struct {
    List message_list;
    Task *task;
    Signals signal;
    MsgPort_Action action;
} MsgPort;


typedef struct {
    Node node;
    MsgPort *reply_port;
} Message;


/**
\brief Initialize a message port before use.
*/
void msgport_init(MsgPort *const port);


Message *msgport_wait(MsgPort *const port);


Message *msgport_get(MsgPort *const port);


void msgport_send(MsgPort *const port, Message *const message);


void msgport_reply(Message *const message);


typedef enum {
    TIMER_NONE,
    TIMER_DELAY,
    TIMER_ALARM
} Timer_Operation;

typedef enum {
    TIMER_INVALID,
    TIMER_INITIALIZED,
    TIMER_ADDED,
    TIMER_DONE,
    TIMER_ABORTED
} Timer_Status;

typedef struct {
    Node node;
    Task *task;
    /** The task will be sent this signal by the timer
    service. */
    Signals signal;
    /** The number of timer ticks to delay when op =
    TIMER_DELAY. */
    Ticks delay;
    /** The timer tick at which task will be signalled signal
    when op = TIMER_ALARM. */
    Ticks tick;
    Timer_Operation op;
    Timer_Status status;
} Timer;


/**
\brief Wait for a specified number of timer ticks.

\param ticks The number of ticks to wait for.
*/
void timer_delay(Ticks ticks);


/**
\brief Get the current timer tick.

\return system tick.
*/
Ticks timer_get_clock(void);


/** Allocate resources for Timer and prepare it.
*/
void timer_allocate(Timer *timer);


void timer_free(Timer *timer);


/**
\brief Add timer request to timer queue.

After the request has been added it can be waited for by
issuing wait() on the timer->signal.

These following conditions must be fulfilled to successfully
add the request:

- If operation = TIMER_DELAY then timer->delay must be set.

- If operation = TIMER_ALARM then timer->tick must be set.

\param timer The Timer to add.
*/
void timer_add(Timer *timer);


/**
\brief Abort a timer request.

\param timer The previoiusly added request to abort.
*/
void timer_abort(Timer *timer);

#endif

