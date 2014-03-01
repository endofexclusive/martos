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
#include <lwip/sys.h>
#include <lwip/mem.h>
#include <lwip/stats.h>

#include <martos/martos.h>
#include <arch/sys_arch.h>

void sys_arch_abort(const char *const str)
{
    LWIP_UNUSED_ARG(str);
    user_halt();
}

void sys_init(void)
{
}

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
    sem_init(sem, count);
    SYS_STATS_INC_USED(sem);
    return ERR_OK;
}

void sys_sem_free(sys_sem_t *sem)
{
    LWIP_UNUSED_ARG(sem);
    SYS_STATS_DEC(sem.used);
}

int sys_sem_valid(sys_sem_t *sem)
{
    LWIP_UNUSED_ARG(sem);
    return 1;
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
    LWIP_UNUSED_ARG(sem);
}

void sys_sem_signal(sys_sem_t *sem)
{
    sem_signal(sem);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    u32_t end_time;
    Ticks start_time = timer_get_clock();
    SemaphoreRequest req;

    req.signal = 1 << signal_allocate(-1);
    assert(0 != req.signal);
    req.waiter = task_find(NULL);
    if (false == sem_add_request(sem, &req)) {
        /* We got the semaphore very early. */
        end_time = timer_get_clock() - start_time;
        signal_free(req.signal);
        return end_time;
    } else {
        /* Have to wait for it. */
    }
 
    Timer timer;
    Signals timer_signal = 0;

    if (0 != timeout) {
        /* Setup timer request. */
        timer_allocate(&timer);
        timer.op = TIMER_DELAY;
        timer.delay = timeout;

        timer_signal = timer.signal;
        timer_add(&timer);
    } else {
        /* Block until arrival: timer_signal is 0. */
    }

    Signals got_signals = signal_wait(timer_signal | req.signal);
    if (0 != (got_signals & req.signal)) {
        /* Got semaphore or got semaphore and a timeout. */
        end_time = timer_get_clock() - start_time;
    } else if(0 != (got_signals & timer_signal)) {
        /* Got only a timeout. */
        end_time = SYS_ARCH_TIMEOUT;
    } else {
        /* We got a signal we didn't ask for! */
        assert(true);
        while(1);
    }
    if (0 != timer_signal) {
        timer_abort(&timer);
        timer_free(&timer);
    }
    signal_free(req.signal);
    return end_time;
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    LWIP_UNUSED_ARG(size);
    /* Manually initialize port. Task and signal fields are
    installed when wait-blocking is performed. */
    list_init(&mbox->message_list);
    mbox->signal = 0;
    mbox->action = MSGPORT_IGNORE;
    mbox->task = NULL;
    SYS_STATS_INC_USED(mbox);
    return ERR_OK;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
    signal_free(mbox->signal);
    SYS_STATS_DEC(mbox.used);
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
    if (MSGPORT_INVALID == mbox->action) {
        return 0;
    } else {
        return 1;
    }
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
    /* sys_mbox_free() is always called before calling this
    function. */
    mbox->action = MSGPORT_INVALID;
}

typedef struct {
    Message message;
    void *ptr;
} sys_msg_t;

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    sys_msg_t *m = mem_malloc(sizeof (sys_msg_t));
    m->ptr = msg;
    msgport_send(mbox, (Message *) m);
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    sys_mbox_post(mbox, msg);
    return ERR_OK;
}

/* msg is the payload, not the sys_msg_t object. */
u32_t sys_arch_mbox_fetch(
    sys_mbox_t *mbox,
    void **msg,
    u32_t timeout
)
{
    sys_msg_t *m;
    u32_t end_time;
    Ticks start_time = timer_get_clock();
    /* Prepare message port for blocking. This should
    semantically be done in sys_mbox_new but that function may
    be called from a different context than sys_arch_mbox_fetch
    so do it here instead. */
    mbox->task = task_find(NULL);
    if (0 == mbox->signal) {
        mbox->signal = 1 << signal_allocate(-1);
        assert(0 != mbox->signal);
    }
    mbox->action = MSGPORT_SIGNAL;
 
    Timer timer;
    Signals timer_signal = 0;

    if (0 != timeout) {
        /* Setup timer request. */
        timer_allocate(&timer);
        timer.op = TIMER_DELAY;
        timer.delay = timeout;

        timer_signal = timer.signal;
        timer_add(&timer);
    } else {
        /* Block until arrival: timer_signal is 0. */
    }

    Signals got_signals = signal_wait(timer_signal | mbox->signal);
    if (0 != (got_signals & mbox->signal)) {
        /* Got a message or got a message and a timeout. */
        m = (sys_msg_t *) msgport_get(mbox);
        assert(NULL != m);
        if (NULL != msg) {
        	/* Give message to user. */
        	*msg = m->ptr;
        } else {
        	/* Drop the message. */
        }
        mem_free(m);
        end_time = timer_get_clock() - start_time;
    } else if(0 != (got_signals & timer_signal)) {
        /* Got only a timeout. */
        end_time = SYS_ARCH_TIMEOUT;
    } else {
        /* We got a signal we didn't ask for! */
        assert(true);
        while(1);
    }
    if (0 != timer_signal) {
        timer_abort(&timer);
        timer_free(&timer);
    }
    return end_time;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	assert(msg != NULL);
    sys_msg_t *m;
    m = (sys_msg_t *) msgport_get(mbox);
    if (NULL == m) {
        /* No message found. */
        return SYS_MBOX_EMPTY;
    } else {
        /* A message was there. */
        *msg = m->ptr;
        mem_free(m);
        return 0;
    }
}

sys_thread_t sys_thread_new(
    const char *name,
    void (*thread) (void *arg),
    void *arg,
    int stacksize,
    int prio
)
{
    Task *task;
    void *stack;

    stack = mem_malloc(stacksize);
    if (NULL != stack) {
        task = mem_malloc(sizeof (Task));
        if (NULL != task) {
            task_init(
                task,
                (char *const) name,
                prio,
                thread,
                arg,
                stack,
                stacksize
            );
            task_schedule(task);
        } else {
            /* Allocation failed for task. */
            assert(true);
            while(1);
        }
    } else {
        /* Allocation failed for stack. */
        assert(true);
        while(1);
    }
    return task;
}

u32_t sys_now(void)
{
    return timer_get_clock();
}

#if 0
sys_prot_t sys_arch_protect(void)
{
    Disable();
    return 0;
}

void sys_arch_unprotect(sys_prot_t pval)
{
    Enable();
}
#endif
