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

#include <assert.h>
#include <stddef.h>
#include <martos/martos.h>
#include <test_common.h>
#include <stm32f4xx.h>
#include <stm32f4xx_tim.h>
#include <stm32f4xx_gpio.h>

Task t2;
uint8_t t2_stack[TEST_STACK_SIZE];

void t2_f(void *user_data)
{
    while(1) {
        GPIO_ToggleBits(GPIOD, GPIO_Pin_13);
        timer_delay(299);
        GPIO_ToggleBits(GPIOD, GPIO_Pin_13);
        timer_delay(297);
    }
}

void test_task_f(void *user_data)
{
    task_init(
        &t2,
        "t2",
        -1,
        (void (*const)(void *)) t2_f,
        NULL,
        &t2_stack,
        TEST_STACK_SIZE
    );
    task_schedule(&t2);

    while(1) {
        GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
        timer_delay(1167);
        GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
        timer_delay(1231);
    }
}

