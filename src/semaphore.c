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
#include <assert.h>
#include <martos/martos.h>
#include "private.h"

/* FIXME: All functions in this file need sanity checks
(asserts). */

static void sem_verify(Semaphore *const sem)
{
    assert(false);
}

void sem_allocate(Semaphore *const sem)
{
    sem->owner = NULL;
    sem->nestcnt = 0;
    list_init(&sem->req_queue);
}

void sem_obtain(Semaphore *const sem)
{
    SemaphoreRequest req;
    req.signal = SIGF_SINGLE;
    req.waiter = running;
    if (true == sem_add_request(sem, &req)) {
        signal_wait(SIGF_SINGLE);
    } else {
        /* We have got it. */
    }
}

bool sem_add_request(Semaphore *const sem, SemaphoreRequest *const req)
{
    sem_verify(sem);
    /* We could temporarily set a very high priority instead
    of disable(). */
    disable();
    if (NULL != sem->owner && (sem->owner != running)) {
        /* Someone has the semaphore, and it is not us. Add
        our request but do not wait. */
        list_add_tail(&sem->req_queue, (Node *) req);
        enable();
        return false;
    } else {
        /* The semaphore is free. Take it. */ 
        sem->owner = running;
        sem->nestcnt++;
        enable();
        return true;
    }
}

void sem_release(Semaphore *const sem)
{
    SemaphoreRequest *req;

    sem_verify(sem);
    disable();
    assert(sem->owner == running);
    sem->nestcnt--;
    if (0 == sem->nestcnt) {
        /* Last nesting stage, check pending requests for the
        semaphore. */
        req = (SemaphoreRequest *) list_rem_head(&sem->req_queue);
        if (NULL == req) {
            /* No one wants it. */
        } else {
            /* Give new owner a nest count. */
            sem->nestcnt = 1;
            signal_send(req->waiter, req->signal);
        }
    }
    enable();
}

