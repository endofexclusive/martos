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

#ifndef PRIVATE_H
#define PRIVATE_H

/* Define MARTOS_NAMESPACE to compile all C files of the kernel
in a single compilation unit. */
#ifdef MARTOS_NAMESPACE
    #define PRIVATE static
#else
    #define PRIVATE
/* Interrupt disable nest count. */
extern NestCnt id_nestcnt;

/* Number of ticks left for task in round-robin. */
extern uint_fast8_t elapsed;

/* The currently running task. */
extern Task *running;

/* All ready tasks must be on the waiting queue. */
extern List ready;

/* All non-ready tasks must be on the waiting queue. */
/* FIXME: What about tasks waiting on semaphores? */
extern List waiting;

#endif

/* Verify that task structure is valid. */
PRIVATE void task_verify(Task *const task);
PRIVATE void timer_init(void);
PRIVATE void timer_poll(void);
PRIVATE TaskContext *martos_pre(void);

#endif
