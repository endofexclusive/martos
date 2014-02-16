/*

Copyright (c) 2014, Martin Åberg All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. The names of the copyright holder(s) may not be used to endorse or promote
products derived from this software without specific prior written permission.

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
#include <assert.h>
#include <martos/martos.h>
#include "private.h"
#include "platform_protos.h"

void timer_allocate(Timer *timer)
{
    SignalNumber signum;

    timer->signal_task = running;

    signum = task_allocate_signal(-1);
    assert(-1 != signum);
    timer->signal = 1 << signum;

    timer->operation = TIMER_NONE;
    timer->status = TIMER_INITIALIZED;
}

void timer_free(Timer *timer)
{
    timer->signal_task = NULL;
    /* Deallocate signal. */
    assert(true);
    timer->operation = TIMER_NONE;
    timer->status = TIMER_INVALID;
}

void timer_delay(Ticks ticks)
{
    Timer timer;

    timer_allocate(&timer);
    timer.operation = TIMER_DELAY;
    timer.delay = ticks;
    timer_add(&timer);
    if (TIMER_ADDED == timer.status) {
        task_wait(timer.signal);
    } else {
        /* Already elapsed. */
    }
    timer_free(&timer);
}

void timer_add(Timer *timer)
{
    assert(
      TIMER_DELAY == timer->operation ||
      TIMER_ALARM == timer->operation
    );
    assert(
      TIMER_INITIALIZED == timer->status ||
      TIMER_DONE == timer->status ||
      TIMER_ABORTED == timer->status
    );

    if (TIMER_DELAY == timer->operation) {
        assert(0 != timer->tick);
        timer->tick = timer_clock() + timer->delay;
    } else {
        /* Tick was already set. */
    }

    /* Check if timer->tick has already elapsed. */
    if (timer->tick <= timer_clock()) {
        timer->status = TIMER_ABORTED;
        return;
    }

    /* Insert timer in the timers queue according to
    timer->tick. The timers queue must be sorted. */

    Timer *tnode;
    bool added = false;

    disable();
    tnode = (Timer *) timers.head.next;
    while (NULL != tnode->node.next) {
        /* For each Timer in timers. */
        if (0 < tnode->tick - timer->tick) {
            /* Tick in the new Timer is less than nt. Add the
            new Timer before the found timer. */
            list_insert(&timers, &timer->node, tnode->node.prev);
            added = true;
            break;
        }
        tnode = (Timer *) tnode->node.next;
    }

    if (false == added) {
        /* The timers queue was empty or timer shall be at
        end. */
        list_add_tail(&timers, &timer->node);
    }
    timer->status = TIMER_ADDED;

    if (list_get_head(&timers) == &timer->node) {
        /* Timer ended up in head of the queue so we must
        notify the timer hardware. */
        timer_update();
    }
}

PRIVATE void timer_poll(void)
{
    Ticks now = timer_clock();

    Timer *tnode;

    disable();
    tnode = (Timer *) timers.head.next;
    while (NULL != tnode->node.next) {
        /* For each Timer in timers. */
        if (0 < tnode->tick - now) {
            break;
        }
        /* Tick in tnode is greater than now. Remove and signal. */
        node_unlink(&tnode->node);
        task_signal(tnode->signal_task, tnode->signal);
        /* Next. */
        tnode = (Timer *) tnode->node.next;
    }
    enable();
}

