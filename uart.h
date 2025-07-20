/* uart.h
 *
 *  Created on: Jul 12, 2025
 *      Author: Ivars Renge
 *
 *  UART Driver for console-style commands
 *
 *  Usage:
 *    1. In your main header or before including this, do:
 *         #define USING_UART huart1*
 *    2. Include this file:
 *         #include "uart.h"
 *    3. In your MX_USARTx_UART_Init (or right after), start RX-IT:
 *         HAL_UART_Receive_IT(&USING_UART, &uart_rx_byte, 1);
 *    4. In your stm32xx_it.c, in HAL_UART_RxCpltCallback:
 *         #ifdef USING_UART
 *             uartReceiveBuffer(&uart_rx_byte, 1);
 *             HAL_UART_Receive_IT(&USING_UART, &uart_rx_byte, 1);
 *         #endif
 *
 *    20.07.2025
 *    USING_UART_DMA hdma_usart2_tx will allow faster output
 *    In this case - make sure __HAL_RCC_DMA1_CLK_ENABLE(); is called before UART initialization, spend a hell lot of effort to figure this out
 */

#ifndef SYS_UART_H_
#define SYS_UART_H_

#include "core.h"

#ifdef USING_UART

//#include "main.h"      // for extern UART_HandleTypeDef huart1, etc.

#define UART_OVERFLOW           1    // ring-buffer overflow
#define UART_RXPROC_SPEED       ST_MS
#define UART_TXPROC_SPEED    	ST_MS
#define UART_CMD_BUFFER_SIZE    128
#define UART_RING_BUFFER_SIZE   128
#define TX_DMA_BUFFER_SIZE   1024


#define USE_HAL_UART_REGISTER_CALLBACKS 1

extern volatile uint16_t   ring_head;
extern volatile uint16_t   ring_tail;
extern char                uart_ring_buf[UART_RING_BUFFER_SIZE];

extern char                cmd[UART_CMD_BUFFER_SIZE];
extern volatile uint8_t    cmdLoaded;

// call once in main() after MX_USARTx_UART_Init()
void    uartInit(uint32_t msg);

// spawnable task that parses CR/LF-terminated commands
void    uartRxProcessor(uint32_t param);
void    uartTxProcessor(uint32_t param);

// called from IRQ callback to push incoming bytes
void    uartReceiveBuffer(uint8_t* data, uint16_t len);

__attribute__((weak)) uint8_t onCommand(uint32_t param);
__attribute__((weak)) void    onUartError(uint32_t flag);

// can we send? (non-blocking check)
int     uartCanWrite(void);

#endif /* USING_UART */

#endif /* SYS_UART_H_ */
