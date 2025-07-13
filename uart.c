/* uart.c
 *
 *  Created on: Jul 12, 2025
 *      Author: Ivars Renge
 */

#include "uart.h"

#ifdef USING_UART

extern UART_HandleTypeDef  USING_UART;  // e.g. huart1

// ring buffer state
volatile uint16_t ring_head = 0;
volatile uint16_t ring_tail = 0;
char            uart_ring_buf[UART_RING_BUFFER_SIZE];

char             cmd[UART_CMD_BUFFER_SIZE];
volatile uint8_t cmdLoaded = 0;

// simple one-byte rx
static uint8_t uart_rx_byte;

/*
void uartReceiveCallback(UART_HandleTypeDef *huart,uint16_t size) {

}*/


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
    tTask* proc = repeat("UART_PROC", UART_PROCESSOR_SPEED, &uartProcessor);
    proc->timeout = 1000 * ST_SS * 30;
    proc->realtime_fail = ST_SEC;
    printf("uart loaded, msg=0x%08lX\n", msg);

//    USING_UART->RxEventCallback = uartReceiveCallback;
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
void uartProcessor(uint32_t param) {
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
    printf("\x1b[32muart> %s\x1b[0m\n", cmd);
    return 0;
}

__attribute__((weak)) void onUartError(uint32_t flag) {
    printf("\x1b[31mUart Error %u\x1b[0m\n", flag);
}

int uartCanWrite(void) {
    HAL_UART_StateTypeDef st = HAL_UART_GetState(&USING_UART);
    return (st != HAL_UART_STATE_BUSY_TX    // 0x21
         && st != HAL_UART_STATE_BUSY_TX_RX // 0x23
         && st != HAL_UART_STATE_BUSY);     // 0x24
}

// _write() override for printf() et al.  Kicks off an interrupt-driven send
// if the UART is idle.  Returns “len” on success, 0 otherwise.
int _write(int file, char *data, int len) {
    if (!uartCanWrite()) {
        return 0;  // still busy transmitting previous data
    }
    // start TX in interrupt mode; HAL will clear BUSY_TX in its TxCpltCallback
    if (HAL_UART_Transmit_IT(&USING_UART, (uint8_t*)data, len) == HAL_OK) {
        return len;
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Make sure you have this in your IRQ/Callback file so the HAL will
// unlock the UART and set gState = READY when the transfer finishes:
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &USING_UART) {
        // nothing to do here—the HAL core calls __HAL_UNLOCK() which sets
        // huart->gState = HAL_UART_STATE_READY for you.
    }
}

#endif /* USING_UART */
