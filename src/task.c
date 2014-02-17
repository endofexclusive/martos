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

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <martos/martos.h>
#include "private.h"
#include "platform_protos.h"

void task_init(
    Task *const task,
    char *const name,
    const Node_Prio prio,
    void (*const init_pc) (void *user_data),
    void *const user_data,
    void *const stack,
    const uint32_t stack_size
)
{
    /* This task can not be running so there is no need to
    protect it. */
    task->state = TASK_INVALID;
    assert(NULL == task_find(name));
    task->node.name = name;
    task->node.prio = prio;
    task->sig_alloc = SIGF_SINGLE;
    task->sig_wait = 0;
    task->sig_recvd = 0;
    task->id_nestcnt = -1;
    taskcontext_init(&task->context, init_pc, user_data, stack, stack_size);
    task->state = TASK_INITIALIZED;
}

void task_schedule(Task *const task)
{
    task_verify(task);
    disable();
    task->state = TASK_READY;
    list_enqueue(&ready, (Node *) task);
    enable();
    reschedule();
}

Task *task_find(char *const name)
{
    Task *task;

    if (NULL == name) {
        return running;
    }
    disable();
    /* Try system lists. */
    task = (Task *) list_find(&ready, name);
    if (NULL == task) {
        task = (Task *) list_find(&waiting, name);
        if (NULL == task) {
            /* Try self. */
            if (0 == strcmp(name, task->node.name)) {
                task = running;
            }
        }
    }
    enable();
    return task;
}

void task_set_prio(Task *const task, const Node_Prio prio)
{
    disable();
    task->node.prio = prio;
    if (task == running ||
      (TASK_READY == task->state &&
      running->node.prio < task->node.prio)) {
        reschedule();
    }
    enable();
}

SignalNumber signal_allocate(SignalNumber signal)
{
    assert(-1 <= signal && signal < SIGNALS_WIDTH);
    Signals target;

    if (-1 != signal) {
        /* The caller has a preference, check it. */
        target = (1 << signal);
        if (target & running->sig_alloc) {
            /* It was already allocated. */
            signal = -1;
        } else {
            /* It is not allocated. */
        }
    } else {
        /* Search for a free signal. */
        signal = SIGNALS_WIDTH - 1;
        do {
            target = (1 << signal);
            if (target & running->sig_alloc) {
                /* It was already allocated: try next. */
            } else {
                /* It is not allocated. */
                break;
            }
            signal--;
        } while (-1 < signal);
    }

    /* Save a disable() enable() pair if no signal was allocated. */
    if (0 < target) {
        /* Mark as allocated. */
        running->sig_alloc |= target;
        /* We were not waiting for the unallocated signal. */
        running->sig_wait &= ~target;
        disable();
        /* We can only receive this signal after it is
        allocated. */
        running->sig_recvd &= ~target;
        enable();
    }

    return signal;
}

void signal_free(const Signals signals)
{
    /* Do not free SIGF_SINGLE! */
    assert((signals & SIGF_SINGLE) == 0);
    /* Do not free unallocated signals. */
    assert((signals & running->sig_alloc) == signals);
    running->sig_alloc &= ~signals;
}

void signal_send(Task *const task, const Signals signals)
{
    disable();
    task->sig_recvd |= signals;
    if (TASK_WAITING == task->state
        && (signals & task->sig_wait)) {
        /* We have set signals which the task was waiting
        for. Take out of waiting list.  */
        list_unlink((Node *) task);
        /* Move signalled task to ready list. */
        task->state = TASK_READY;
        list_enqueue(&ready, (Node *) task);
        if (running->node.prio < task->node.prio) {
            /* Signalled task has higher priority:
            reschedule. */
            reschedule();
        } else {
            /* task has lower priority: leave it.
               task has same  priority: let running have its
               time elapse.
             */
        }
    }
    enable();
}

Signals signal_wait(const Signals signals)
{
    Signals rcvd;
    int16_t nestcnt;

    disable();
    running->sig_wait = signals;
    while (!(signals & running->sig_recvd)) {
        running->state = TASK_WAITING;
        list_add_tail(&waiting, (Node *) running);
        /* Block, we must be switched out! */
        /* We are in disabled state. Temporarily force a break
           of it so that switch can be carried out. */

        nestcnt = id_nestcnt;
        id_nestcnt = 0;
        enable();
        /* Now it's time to break any disable() state made
        before call to task_wait(). The fake id_nestcnt will
        be saved in task->id_nestcnt by reschedule. We have
        the real value locally backed up in nestcnt. */
        reschedule();
        disable();
        id_nestcnt = nestcnt;
    }
    rcvd = signals & running->sig_recvd;
    running->sig_recvd &= ~rcvd;
    enable();
    return rcvd;
}

PRIVATE void task_verify(Task *const task)
{
    disable();
    taskcontext_verify(&task->context);
    assert(SIGF_SINGLE & task->sig_alloc);
    /* It is the task private id_nestcnt that is verified so
    there is no need to compensate for the disable() above. */
    assert(-1 <= task->id_nestcnt);
    assert(
      TASK_INITIALIZED == task->state ||
      TASK_RUNNING == task->state ||
      TASK_READY == task->state ||
      TASK_WAITING == task->state
    );
    enable();
}

