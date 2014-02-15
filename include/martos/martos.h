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
#include <taskcontext.h>

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
    /* type Node_Type; */
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


typedef struct {
    Node node; 
} Semaphore;


/**
\brief Unlink node from list.

The modified list is implicit.

\param node The node to unlink.
*/
void node_unlink(Node *const node);


/**
\brief Prepare a list for list operations.

This function must be applied to the node before any other
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
\brief Call a function on all nodes on a list.

Traversal is from head to tail. The traversal ends when fn
returns false or when the tail node has been visited.

\param start The list to traverse. It can also be a node,
in which case this node will be skipped.
\param fn Function to call on each node. The node parameter
is a handle to the visited node. Parameter
arg to fn is the same as parameter arg to list_apply.
\return Pointer to the first node for which fn returns true,
or the list parameter if fn did never return true.
*/
List *list_apply(
    List *const start,
    bool (*fn)(Node *const node, void *const arg),
    void *const arg
);


/**
\brief Prepare a task for scheduling.

Private Task fields are set. The task is not scheduled for
execution: use task_schedule() to do so.

\param task The task to initialize.
\param name String identifier of task. It may be NULL.
\param prio Pre-emptive scheduling priority of task. A
larger number gives higher scheduling priority. The lowest
permitted value is TASK_PRIO_MIN and the largest is
TASK_PRIO_MAX. TASK_PRIO_EXCLUSIVE is reserved and
not allowed for this function.
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
\brief Unschedule task.

Remove the task from the scheduler so that it will not run
again, until it is added again with task_schedule().
\param task The task to schedule.
*/
void task_unschedule(Task *const task);


/**
\brief Find task by name or find self.

Task queues are searched for a task with given name.

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
SignalNumber task_allocate_signal(SignalNumber signal);


/**
\brief Free a signal bit.
*/
void task_free_signal(const SignalNumber signal);


/**
\brief Send bit signals to a task.

This function is callable from interrupt context. */
void task_signal(Task *const task, const Signals signals);


/**
\brief Wait for bit signals.
*/
Signals task_wait(const Signals signals);


/**
\brief Callback function for initialization of the first task
to be scheduled.

This function is for the purpose of the user to select a
task to run att system start.  Only a call to task_init()
with proper parameters is needed. Other initializations can
be done here aswell.

\note The multitasking kernel is not ready or running when this
function is called so only a small set of kernel operations
can be called. Only the list_ functions are considered safe.

\return Pointer to the first task. Do not run task_schedule()
on the task in init_task_init()!  */
Task *init_task_init(void);

void disable(void);


void enable(void);


void sem_init(Semaphore *const sem, int value);


/**
\brief Signal a semaphore.

value := value + 1
IF value = 0 THEN unblock a client.
*/
void sem_signal(Semaphore *const sem);


/**
\brief Wait on semaphore.

value := value - 1
IF value < 0 THEN block.
*/
void sem_wait(Semaphore *const sem);


typedef enum {
    MSGPORT_SIGNAL,
    MSGPORT_IGNORE
} MsgPort_Flags;

typedef struct {
    Node node;
    MsgPort_Flags flags;
    /* A single signal used for communication on this port. */
    Signals signal;
    Task *signal_task;
    List message_list;
} MsgPort;

typedef struct {
    Node node;
    MsgPort *reply_port;
} Message;

/**
\brief Initialize a message port before use.

*/
bool msgport_init(MsgPort *const port);
Message *msgport_wait(MsgPort *const port);
Message *msgport_get(MsgPort *const port);
void msgport_put(MsgPort *const port, Message *const message);
void msgport_reply(Message *const message);

#endif

