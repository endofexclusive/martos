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
#include <stdint.h>
#include <assert.h>
#include <martos/martos.h>
#include <lwip/tcpip.h>
#include <lwip/api.h>
#include <stm32f4xx_gpio.h>
#include "udpecho.h"

#define N_STACK_SIZE 4096
Task n1;
Task n2;
uint8_t n1_stack[N_STACK_SIZE];
uint8_t n2_stack[N_STACK_SIZE];

void n1_f(void)
{
    struct netconn *conn;
    struct netbuf *buf;
    struct ip_addr addr;
    char text[] = "Internet of things";
    err_t res;

    conn = netconn_new(NETCONN_UDP);
    assert(NULL != conn);

    IP4_ADDR(&addr, 127,0,0,1);

    res = netconn_connect(conn, &addr, 7);
    assert(ERR_OK == res);

    buf = netbuf_new();
    assert(NULL != buf);

    res = netbuf_ref(buf, text, sizeof(text));
    assert(ERR_OK == res);

    Timer timer;

    timer_allocate(&timer);
    timer.op = TIMER_ALARM;
    timer.tick = timer_get_clock();

    while (1) {
        timer.tick += 250;
        res = netconn_send(conn, buf);
        assert(ERR_OK == res);
        GPIO_ToggleBits(GPIOD, GPIO_Pin_13);

        timer_add(&timer);
        if (TIMER_ADDED == timer.status) {
            signal_wait(timer.signal);
        } else {
            /* Already elapsed. */
        	assert(true);
        }
    }
    timer_free(&timer);
    res = netconn_delete(conn);
    assert(ERR_OK == res);

    netbuf_delete(buf);

}

void n2_f(void)
{
    Timer timer;

    timer_allocate(&timer);
    timer.op = TIMER_ALARM;
    timer.tick = timer_get_clock();

    while (1) {
        timer.tick += 250;
        GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
        timer_add(&timer);
        if (TIMER_ADDED == timer.status) {
            signal_wait(timer.signal);
        } else {
            /* Already elapsed. */
        	assert(true);
        }
    }
    timer_free(&timer);
}

void user_init(void)
{
    struct netif *loif;

    tcpip_init(NULL, NULL);
    loif = netif_find("lo0");
    assert(NULL != loif);
    netif_set_default(loif);
    udpecho_init();

    task_init(
        &n1,
        "n1",
        0,
        (void (*const)(void *)) n1_f,
        NULL,
        &n1_stack,
        N_STACK_SIZE
    );
    task_init(
        &n2,
        "n2",
        -1,
        (void (*const)(void *)) n2_f,
        NULL,
        &n2_stack,
        N_STACK_SIZE
    );
    task_schedule(&n1);
    task_schedule(&n2);
}

