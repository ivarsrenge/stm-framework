/* uart.c
 *
 *  Created on: Jul 12, 2025
 *      Author: Ivars Renge
 */

#include "uart.h"

#ifdef USING_UART


// ring buffer state
volatile uint16_t ring_head = 0;
volatile uint16_t ring_tail = 0;
char            uart_ring_buf[UART_RING_BUFFER_SIZE];

char             cmd[UART_CMD_BUFFER_SIZE];
volatile uint8_t cmdLoaded = 0;
char uartLoaded = 0;

// simple one-byte rx
static uint8_t uart_rx_byte;


extern UART_HandleTypeDef  USING_UART;  // e.g. huart1
extern DMA_HandleTypeDef   USING_UART_DMA;    // e.g. hdma_usart1_tx

// transmit buffer
#define TX_DMA_BUFFER_SIZE   1024
static uint16_t tx_dma_len = 0;
static uint8_t tx_buf[TX_DMA_BUFFER_SIZE];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;

// Kick off DMA from buffer tail to head
#ifndef USING_UART_DMA
void uartTxProcessor(uint32_t param) {
	char queue[1];
	  // empty if head == tail
	    if (tx_head == tx_tail) {
	        return;
	    }

	    __disable_irq();               // protect concurrent access
	    queue[0] = tx_buf[tx_tail];
	    HAL_UART_Transmit(&USING_UART, queue, 1, 100);
	    tx_tail = (tx_tail + 1) % TX_DMA_BUFFER_SIZE;
	    __enable_irq();
	    kernel_process(1);
	    return;
}
#endif

void uartStartTx(void) {
#ifdef USING_UART_DMA
    // 1) only start if DMA is idle and there's data
    if (HAL_DMA_GetState(&USING_UART_DMA) != HAL_DMA_STATE_READY) return;
    if (USING_UART.gState != HAL_UART_STATE_READY) return;

    if (tx_head == tx_tail) return;

    // 2) calculate contiguous chunk
    uint16_t len = (tx_head > tx_tail) ? (tx_head - tx_tail) : (TX_DMA_BUFFER_SIZE - tx_tail);

    // 3) remember length and kick off DMA
    tx_dma_len = len;
//    HAL_UART_StateTypeDef prev = huart3.gState;
    //HAL_StatusTypeDef  ret  =
    if (!HAL_UART_Transmit_DMA(&USING_UART, &tx_buf[tx_tail], len) == HAL_OK) {
    	printf("DMA Failed\n");
    }


#endif
}

// DMA transmit complete callback
#ifdef USING_UART_DMA

void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart) { //HAL_UART_TxCpltCallback
    if (huart != &USING_UART) return;

    // 4) advance tail by exactly what we sent
    tx_tail = (tx_tail + tx_dma_len) % TX_DMA_BUFFER_SIZE;

    // 5) start next chunk, if any
    uartStartTx();
}
#endif

// in stm32xx_it.c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == &USING_UART) {
    uartReceiveBuffer(&uart_rx_byte, 1);
    HAL_UART_Receive_IT(&USING_UART, &uart_rx_byte, 1);
  }
}



// init UART “console”
void uartInit(uint32_t msg) {
    // prime the RX interrupt (should already be set once in MX_..._Init)
    HAL_UART_Receive_IT(&USING_UART, &uart_rx_byte, 1);
    // spawn the processor task

    tTask* proc = repeat("UART_R_PR", UART_RXPROC_SPEED, &uartRxProcessor);
    proc->timeout = 1000 * ST_SS * 30;
    proc->runAt+= ST_S10; // start after S10
    proc->realtime_fail = ST_SEC * 10;

#ifndef USING_UART_DMA
    proc = repeat("UART_T_PR", UART_TXPROC_SPEED, &uartTxProcessor);
    proc->timeout = 1000 * ST_SS;
    proc->realtime_fail = ST_SEC;
    uartLoaded = 1;
    printf("uart loaded\n");
#else
    //USING_UART_DMA.XferHalfCpltCallback = &HAL_UART_TxCpltCallback;
    //USING_UART_DMA.XferCpltCallback = &HAL_UART_TxCpltCallback;
    uartLoaded = 1;
    printf("uart DMA loaded\n");
#endif
}

// push incoming data into ring buffer
void uartReceiveBuffer(uint8_t* data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        uint16_t next = (ring_head + 1) % UART_RING_BUFFER_SIZE;
        if (next != ring_tail) {
            uart_ring_buf[ring_head] = data[i];
            ring_head = next;
        } else {
            onUartError(UART_OVERFLOW);
        }
    }
}

// parse CR/LF-terminated commands, exec them
void uartRxProcessor(uint32_t param) {
    static char local[UART_CMD_BUFFER_SIZE];
    static uint16_t idx = 0;
    char name[TASK_NAME_LENGTH+1];

    while (ring_tail != ring_head) {
        char c = uart_ring_buf[ring_tail];
        ring_tail = (ring_tail + 1) % UART_RING_BUFFER_SIZE;

        if (c == '\r' || c == '\n') {
            if (idx > 0) {
                local[idx] = '\0';
                strncpy(cmd, local, UART_CMD_BUFFER_SIZE);
                strncpy(name, cmd, TASK_NAME_LENGTH);
                exec(name, &onCommand);
                idx = 0;
            }
        } else if (idx < UART_CMD_BUFFER_SIZE - 1) {
            local[idx++] = c;
        } else {
            // overflow, reset
            idx = 0;
        }
        kernel_process(param);
    }
}

__attribute__((weak)) uint8_t onCommand(uint32_t param) {
	setTextColor(YELLOW);
    printf("uart> %s\n", cmd);
	setTextColor(DEFAULT_COLOR);
    return 0;
}

__attribute__((weak)) void onUartError(uint32_t flag) {
	setTextColor(RED);
    printf("Uart Error %u\n", flag);
	setTextColor(DEFAULT_COLOR);
}


int _write(int file, char *data, int len) {
	uint16_t i;
	__disable_irq(); // protect head/tail
	for (i = 0; i < len; i++) {
	  uint16_t next = (tx_head + 1) % TX_DMA_BUFFER_SIZE;
	  if (next == tx_tail) {
		  __enable_irq();
		  uartStartTx();
		  return UART_OVERFLOW;
	  }
	  tx_buf[tx_head] = data[i];
	  tx_head = next;
	}
	__enable_irq();

	if (uartLoaded) uartStartTx();
	return len;
}




#endif /* USING_UART */
