/*
 * FreeModbus Libary: LPC54xxx Port
 * Copyright (C) 2018 Wolf Walter <wolfwalterjever@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: porttimer.c $
 */

/* ----------------------- FreeRTOS includes --------------------------------*/
#include "FreeRTOS.h"
#include "task.h"

/* ----------------------- LPC54xxx includes --------------------------------*/
#include "board.h"
#include "fsl_ctimer.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_CTIMER CTIMER3
#define MB_TIMER_IRQ CTIMER3_IRQn
#define MB_TIMER_IRQ_PRIO 2
#define MB_CTIMER_MAT0_OUT kCTIMER_Match_0 /* Match output 0 */
#define BUS_CLK_FREQ CLOCK_GetFreq(kCLOCK_AsyncApbClk)
#define MB_TIMER_TICKS          ( 20000UL )

/* ----------------------- prototypes ---------------------------------------*/
void ctimer_match0_callback(uint32_t flags);

/* ----------------------- static variables ---------------------------------*/
/* Array of function pointers for callback for each channel */
static ctimer_callback_t ctimer_callback_table[] = {
    ctimer_match0_callback, NULL, NULL, NULL, NULL, NULL, NULL, NULL};


/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortTimersInit( USHORT usTim1Timerout50us )
{
	ctimer_match_config_t matchConfig0;
	ctimer_config_t config;
	/* Enable the asynchronous bridge */
	SYSCON->ASYNCAPBCTRL = 1;

	/* Use 12 MHz clock for some of the Ctimers */
	CLOCK_AttachClk(kFRO12M_to_ASYNC_APB);

	CTIMER_GetDefaultConfig(&config);

	CTIMER_Init(MB_CTIMER, &config);

    matchConfig0.enableCounterReset = false;
    matchConfig0.enableCounterStop = true;
    matchConfig0.matchValue = BUS_CLK_FREQ / MB_TIMER_TICKS * usTim1Timerout50us;
    matchConfig0.outControl = kCTIMER_Output_NoAction;
    matchConfig0.outPinInitState = false;
    matchConfig0.enableInterrupt = true;

    CTIMER_RegisterCallBack(MB_CTIMER,  &ctimer_callback_table[0], kCTIMER_MultipleCallback);
    CTIMER_SetupMatch(MB_CTIMER, MB_CTIMER_MAT0_OUT, &matchConfig0);

    NVIC_SetPriority(MB_TIMER_IRQ, MB_TIMER_IRQ_PRIO);

    return TRUE;
}

void ctimer_match0_callback(uint32_t flags)
{
	BaseType_t reschedule;

	reschedule = pxMBPortCBTimerExpired(  );

	portYIELD_FROM_ISR(reschedule);
}


inline void vMBPortTimersEnable(  )
{
	CTIMER_Reset(MB_CTIMER);
	CTIMER_StartTimer(MB_CTIMER);
}

inline void vMBPortTimersDisable(  )
{
	CTIMER_StopTimer(MB_CTIMER);
}

