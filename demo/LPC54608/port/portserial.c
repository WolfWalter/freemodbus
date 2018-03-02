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
 * File: $Id: portserial.c $
 */

/* ----------------------- FreeRTOS kernel includes. ------------------------*/
#include "FreeRTOS.h"

/* ----------------------- LPC54xxx includes. -------------------------------*/
#include "board.h"
#include "fsl_usart.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "port.h"
#include "mb.h"
#include "mbport.h"

/* ----------------------- Definitions --------------------------------------*/
#define MB_USART USART0
#define MB_USART_IRQHandler FLEXCOMM0_IRQHandler
#define MB_USART_IRQn FLEXCOMM0_IRQn
#define MB_USART_PRIO 2
#define MB_USART_CLK_FREQ CLOCK_GetFreq(kCLOCK_Flexcomm0)
#define MB_USART_IRQHandler FLEXCOMM0_IRQHandler

/* ----------------------- Start implementation -----------------------------*/
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
	if( xRxEnable )	USART_EnableInterrupts(MB_USART, kUSART_RxLevelInterruptEnable | kUSART_RxErrorInterruptEnable);
	else USART_DisableInterrupts(MB_USART, kUSART_RxLevelInterruptEnable | kUSART_RxErrorInterruptEnable);
	if( xTxEnable ) USART_EnableInterrupts(MB_USART, kUSART_TxLevelInterruptEnable | kUSART_TxErrorInterruptEnable);
	else USART_DisableInterrupts(MB_USART, kUSART_TxLevelInterruptEnable | kUSART_TxErrorInterruptEnable);
}

BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
	BOOL xResult = TRUE;

	usart_config_t config;
	USART_GetDefaultConfig(&config);
	config.baudRate_Bps = ulBaudRate;
    config.enableTx = true;
    config.enableRx = true;

    switch ( eParity )
	{
	case MB_PAR_EVEN:
		config.parityMode = kUSART_ParityEven;
		break;
	case MB_PAR_ODD:
		config.parityMode = kUSART_ParityOdd;
		break;
	case MB_PAR_NONE:
		config.parityMode = kUSART_ParityDisabled;
		break;
	}

    switch ( ucDataBits )
	{
	case 7:
		config.bitCountPerChar = kUSART_7BitsPerChar;
		break;
	case 8:
		config.bitCountPerChar = kUSART_8BitsPerChar;
		break;
	default:
		xResult = FALSE;
	}

    if( xResult != FALSE ){
    	USART_Init(MB_USART, &config, MB_USART_CLK_FREQ);

    	vMBPortSerialEnable( TRUE, FALSE );

    	NVIC_SetPriority(FLEXCOMM0_IRQn, MB_USART_PRIO);

		/* Enable RX interrupt. */
		EnableIRQ(MB_USART_IRQn);
    }

    return xResult;
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
	USART_WriteByte(MB_USART, ucByte);
    return TRUE;
}

BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
	*pucByte = USART_ReadByte(MB_USART);
    return TRUE;
}

void MB_USART_IRQHandler( void )
{
	BaseType_t reschedule = pdFALSE;
	uint32_t enabledStatus = USART_GetEnabledInterrupts(MB_USART);
	uint32_t statusFlags = USART_GetStatusFlags(MB_USART);

	if (enabledStatus & kUSART_TxLevelInterruptEnable &&  (kUSART_TxFifoEmptyFlag | kUSART_TxError) & statusFlags){
		reschedule |= pxMBFrameCBTransmitterEmpty(  );
	}
    if (enabledStatus & kUSART_RxLevelInterruptEnable && (kUSART_RxFifoNotEmptyFlag | kUSART_RxError) & statusFlags){
    	reschedule |= pxMBFrameCBByteReceived(  );
	}

	portYIELD_FROM_ISR(reschedule);

    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
//	#if defined __CORTEX_M && (__CORTEX_M == 4U)
//		__DSB();
//	#endif
}
