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

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stm32f4xx.h>
#include <stm32f4xx_conf.h>
#include <martos/martos.h>
#include <private.h>
#include <platform_protos.h>
#include <default_config.h>

static void dead_end(void)
{
}

static const uint32_t EPSR_T = 1 << 24;

PRIVATE void taskcontext_init(
    TaskContext *const context,
    void (*const init_pc) (void *const user_data),
    void *const user_data,
    void *const stack,
    const uint32_t stack_size
)
{
    /* Stack alignment check. */
    assert(0 == ((uintptr_t) stack) % 4);
    assert(0 == stack_size % 4);

    uint32_t *tos =
      (uint32_t *) ((uint8_t *) stack + stack_size);

    StackFrame *frame =
      (StackFrame *) ((uint8_t *) tos - sizeof (StackFrame));

    context->tos = tos;
    context->bos = stack;

    /* Clear the stack frame. */
    while (--tos != (uint32_t *) frame) {
        *tos = 0;
    }

    /* Set some registers for first run. */
    frame->r0 = (uint32_t) user_data;
    frame->lr = (uint32_t) dead_end;
    frame->pc = init_pc;
    frame->xpsr = EPSR_T;

    context->frame = frame;
    taskcontext_verify(context);
}

PRIVATE void taskcontext_verify(TaskContext *const context)
{
    assert((uintptr_t) context->bos <=
      (uintptr_t) context->frame);
    assert(((uintptr_t) context->frame + sizeof (StackFrame)) <=
      (uintptr_t) context->tos);
}

PRIVATE void reschedule(void)
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/* Override standard library abort(). */
void abort(void)
{
    while(1) {
        ;
    }
}

/* Used by newlib assert. See assert.h. The function must not
return. */
void __assert_func(
    const char *file,
    int line,
    const char *function,
    const char *expression
)
{
    /* Note that we do not use disable() here! So id_nestcnt
    is preserved. */
    __disable_irq();
    abort();
    while(1) {
        ;
    }
}

void disable(void)
{
    __disable_irq();
    id_nestcnt++;
}

void enable(void)
{
    assert(-1 <= id_nestcnt);
    id_nestcnt--;
    if (id_nestcnt < 0) {
        __enable_irq();
    }
}

void led_init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

    GPIO_InitTypeDef gpioStructure;
    gpioStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
    gpioStructure.GPIO_Mode = GPIO_Mode_OUT;
    gpioStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &gpioStructure);

    GPIO_WriteBit(GPIOD, GPIO_Pin_12 | GPIO_Pin_13, Bit_RESET);
}

StackFrame *platform_pre(void)
{
    TaskContext *context;

    context = martos_pre();

    NVIC_SetPriority(PendSV_IRQn, 0xFF);
    SysTick_Config(SysTick->CALIB & SysTick_CALIB_TENMS_Msk);
    /* FIXME: NVIC_SetPriority is called in SysTick_Config...*/
    /* High priority? */
    NVIC_SetPriority(SysTick_IRQn, 0);
    led_init();

    __set_PSP((uint32_t) context->frame);
    return context->frame;
}

static void SysTick_Handler(void)
{
    elapsed--;

    if (0 == elapsed) {
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
    /* PendSV may NOT be triggered immediately if you single
       step out of this function. This is because the debugger
       may have masked out the PendSV interrupt. */
}

void *PendSV_Handler_user(StackFrame *old_frame)
{
    /* Check task which is switched out. */
    task_verify(running);
    /* This is the only place where we may change the running pointer. */

    /* Shared structures must be protected! */
    __disable_irq();

    running->context.frame = old_frame;
    switch (running->state) {
    case TASK_RUNNING:
        /* Maybe select new task to run. Check prio then Elapsed */
        running->state = TASK_READY;
        list_enqueue(&ready, (Node *) running);
        /* If no swap was needed, we can just return here, but
        fall through instead. If the task ended up at the head
        of running list then it will be removed again below. */
    case TASK_READY:
    case TASK_WAITING:
        /* This task is swapped out and does not have state TASK_RUNNING. */
        running->id_nestcnt = id_nestcnt;
        /* It is now switched out. */
        /* Ready for a new one. */
        running = (Task *) list_rem_head(&ready);
        running->state = TASK_RUNNING;
        id_nestcnt = running->id_nestcnt;
        elapsed = QUANTUM;
        break;
    default:
        /* Shall never happen! */
        while (1) {
            ;
        }
    }

    if (id_nestcnt < 0) {
        __enable_irq();
    } else {
        /* We are already protected. */
    }

    /* Check task which is switched in. */
    task_verify(running);
    return running->context.frame;
}

static volatile Ticks timer_now;

PRIVATE void timer_init_platform(void)
{
    timer_now = 0;

    /* Set up hardware to generate timer interrupts. */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitTypeDef timerInitStructure;
    timerInitStructure.TIM_Prescaler = 28 - 1; // 1MHz timebase
    timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    timerInitStructure.TIM_Period = 1000-1;
    timerInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    timerInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &timerInitStructure);
    TIM_Cmd(TIM2, ENABLE);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef nvicStructure;
    nvicStructure.NVIC_IRQChannel = TIM2_IRQn;
    nvicStructure.NVIC_IRQChannelPreemptionPriority = 0;
    nvicStructure.NVIC_IRQChannelSubPriority = 1;
    nvicStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvicStructure);

    //TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
}

Ticks timer_get_clock(void)
{
    return timer_now;
}

static void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        timer_now++;
        timer_poll();
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}

static void Default_Handler(void)
{
    for(;;);
}

extern uint32_t _tos;

void Reset_Handler(void);
void PendSV_Handler(void);

PRIVATE void (*const vector_table[])(void)
  __attribute__ ((section(".isr_vector"))) = {
    (void (*)(void)) &_tos,
    /* Reset */
    Reset_Handler,
    /* NMI */
    Default_Handler,
    /* HardFault */
    Default_Handler,
    /* MemManage */
    Default_Handler,
    /* BusFault */
    Default_Handler,
    /* UsageFault */
    Default_Handler,
    /* <RESERVED> */
    0, 0, 0, 0,
    /* SVC */
    Default_Handler,
    /* DebugMon (<RESERVED> ?) */
    Default_Handler,
    /* <RESERVED> */
    0,
    /* PendSV */
    PendSV_Handler,
    /* SysTick */
    SysTick_Handler,
/* IRQ */
 
/* WWDG_IRQHandler */
    Default_Handler,
/* PVD_IRQHandler */
    Default_Handler,
/* TAMP_STAMP_IRQHandler */
    Default_Handler,
/* RTC_WKUP_IRQHandler */
    Default_Handler,
/* FLASH_IRQHandler */
    Default_Handler,
/* RCC_IRQHandler */
    Default_Handler,
/* EXTI0_IRQHandler */
    Default_Handler,
/* EXTI1_IRQHandler */
    Default_Handler,
/* EXTI2_IRQHandler */
    Default_Handler,
/* EXTI3_IRQHandler */
    Default_Handler,
/* EXTI4_IRQHandler */
    Default_Handler,
/* DMA1_Stream0_IRQHandler */
    Default_Handler,
/* DMA1_Stream1_IRQHandler */
    Default_Handler,
/* DMA1_Stream2_IRQHandler */
    Default_Handler,
/* DMA1_Stream3_IRQHandler */
    Default_Handler,
/* DMA1_Stream4_IRQHandler */
    Default_Handler,
/* DMA1_Stream5_IRQHandler */
    Default_Handler,
/* DMA1_Stream6_IRQHandler */
    Default_Handler,
/* ADC_IRQHandler */
    Default_Handler,
/* CAN1_TX_IRQHandler */
    Default_Handler,
/* CAN1_RX0_IRQHandler */
    Default_Handler,
/* CAN1_RX1_IRQHandler */
    Default_Handler,
/* CAN1_SCE_IRQHandler */
    Default_Handler,
/* EXTI9_5_IRQHandler */
    Default_Handler,
/* TIM1_BRK_TIM9_IRQHandler */
    Default_Handler,
/* TIM1_UP_TIM10_IRQHandler */
    Default_Handler,
/* TIM1_TRG_COM_TIM11_IRQHandler */
    Default_Handler,
/* TIM1_CC_IRQHandler */
    Default_Handler,
/* TIM2_IRQHandler */
    TIM2_IRQHandler
};

