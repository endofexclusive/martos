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

    .syntax unified
    .cpu cortex-m4
    .fpu softvfp
    .thumb

    .global Reset_Handler

    .section .text.Reset_Handler, "ax", %progbits
    .type Reset_Handler, %function

Reset_Handler:  
    /* Copy data segment initializers from flash to SRAM. */
    ldr     r0, =_sdata
    ldr     r1, =_sidata
    ldr     r2, =_data_size
    bl      memcpy

    /* Zero fill the bss segment. */
    ldr     r0, =_sbss
    mov     r1, #0
    ldr     r2, =_bss_size
    bl      memset

    /* CMSIS stuff for setting System clock etc. */
    bl      SystemInit
    /* Set up data structures and init_task. PSP is initialized.
    */
    bl  martos_init

    /* Use process stack pointer and remain in privileged mode.
    User uses PSP. */
    mov     r0, #0x2
    msr     control, r0
    /* "Instruction Synchronization Barrier" flushes the CPU
    pipeline. */
    isb

    /* init_task->frame is in sps. It was set in martos_init. */
    /* Software pop */
    ldm sp!, {r4-r11}
    /* Some of the hardware pop. */
    pop {r0-r3, r12, lr}
    pop {r0} @pc
    pop {r0} @xpsr
    /* Probably not needed for the first task. */
    /* msr apsr,r0 */

    /*  Stack is unwound, so sp is OK. r0 and pc are
    left. Install first tasks startup function parameter. r0 :=
    init_task->frame.r0 */
    ldr	r0, [sp, #-4*8]
    /* Install first tasks entry point.
    pc := init_task->frame.pc */
    ldr	pc, [sp, #-4*2]
    b .

    .size Reset_Handler, .-Reset_Handler

