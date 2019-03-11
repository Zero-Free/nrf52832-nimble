/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <rtthread.h>
#include <rthw.h>

#include "board.h"
#include "drv_uart.h"

#include "nrfx.h"
#include "nrfx_rtc.h"
#include "nrfx_clock.h"

#ifdef __CC_ARM
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define NRF_SRAM_BEGIN    (&Image$$RW_IRAM1$$ZI$$Limit)
#elif __ICCARM__
#pragma section="HEAP"
#define NRF_SRAM_BEGIN    (__segment_end("HEAP"))
#else
extern int __bss_end;
#define NRF_SRAM_BEGIN    (&__bss_end)
#endif

#define OSTICK_CLOCK_HZ  ( 32768UL )
#define COUNTER_MAX 0x00ffffff
#define CYC_PER_TICK (OSTICK_CLOCK_HZ / RT_TICK_PER_SECOND)
#define MAX_TICKS ((COUNTER_MAX - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY 32

#define TICK_RATE_HZ  RT_TICK_PER_SECOND

#define NRF_RTC_REG        NRF_RTC1
/* IRQn used by the selected RTC */
#define NRF_RTC_IRQn       RTC1_IRQn

static uint32_t last_count;

static uint32_t counter_sub(uint32_t a, uint32_t b)
{
    return (a - b) & COUNTER_MAX;
}

static void set_comparator(uint32_t cyc)
{
    nrf_rtc_cc_set(NRF_RTC_REG, 0, cyc & COUNTER_MAX);
}

void RTC1_IRQHandler(void)
{
    uint32_t t, dticks, next;

    NRF_RTC_REG->EVENTS_COMPARE[0] = 0;

    t = nrf_rtc_counter_get(NRF_RTC_REG);
    dticks = counter_sub(t, last_count) / CYC_PER_TICK;
    last_count += dticks * CYC_PER_TICK;
    next = last_count + CYC_PER_TICK;

    if ((int32_t)(next - t) < MIN_DELAY)
    {
        next += CYC_PER_TICK;
    }
    set_comparator(next);

    rt_tick_increase();
}

uint32_t _timer_cycle_get_32(void)
{
    // k_spinlock_key_t key = k_spin_lock(&lock);
    uint32_t ret = counter_sub(nrf_rtc_counter_get(NRF_RTC_REG), last_count) + last_count;

    // k_spin_unlock(&lock, key);
    return ret;
}

void os_tick_init(void)
{
    nrf_clock_lf_src_set((nrf_clock_lfclk_t)NRFX_CLOCK_CONFIG_LF_SRC);
    nrfx_clock_lfclk_start();

    nrf_rtc_prescaler_set(NRF_RTC_REG, 0);

    nrf_rtc_cc_set(NRF_RTC_REG, 0, CYC_PER_TICK);
    nrf_rtc_event_enable(NRF_RTC_REG, RTC_EVTENSET_COMPARE0_Msk);
    nrf_rtc_int_enable(NRF_RTC_REG, RTC_INTENSET_COMPARE0_Msk);

    /* Clear the event flag and possible pending interrupt */
    nrf_rtc_event_clear(NRF_RTC_REG, NRF_RTC_EVENT_COMPARE_0);
    NVIC_ClearPendingIRQ(NRF_RTC_IRQn);

    NVIC_SetPriority(NRF_RTC_IRQn, 1);
    NVIC_EnableIRQ(NRF_RTC_IRQn);

    nrf_rtc_task_trigger(NRF_RTC_REG, NRF_RTC_TASK_CLEAR);
    nrf_rtc_task_trigger(NRF_RTC_REG, NRF_RTC_TASK_START);

    set_comparator(nrf_rtc_counter_get(NRF_RTC_REG) + CYC_PER_TICK);
}

void rt_hw_board_init(void)
{
    os_tick_init();

#ifdef RT_USING_HEAP
    rt_system_heap_init((void *)NRF_SRAM_BEGIN, (void *)CHIP_SRAM_END);
#endif

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#ifdef RT_USING_CONSOLE
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif
}

static int reboot(void)
{
    rt_hw_cpu_reset();

    return 0;
}
MSH_CMD_EXPORT_ALIAS(reboot, reboot, "reset system");
