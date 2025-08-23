/*
 * usb.c
 *
 *  Created on: Jun 14, 2025
 *      Author: Ivars Renge
 *
 *
 */

#include "usb.h"

#ifdef USING_USB


extern USBD_HandleTypeDef hUsbDeviceFS;
int usbProcessorSpeed = USB_PROCESSOR_SPEED;

volatile uint16_t ring_head;
volatile uint16_t ring_tail;
char usb_ring_buf[USB_RING_BUFFER_SIZE];

char cmd[USB_CMD_BUFFER_SIZE];
volatile uint8_t cmdLoaded;

// init usb CDC
void usbInit(uint32_t msg) {
	resetUSBPort();
	MX_USB_DEVICE_Init();
	tTask* proc = repeat("USB_PROC",usbProcessorSpeed, &usbProcessor);
	proc->timeout = 1000 * ST_SS * 30;
	proc->realtime_fail = ST_SEC;

	printf("usb loaded\n");
}

// add as first line in CDC_Receive_FS in usbd_cdc_if.c
void usbReceiveBuffer(uint8_t* Buf, uint32_t *Len) {
    for (uint32_t i = 0; i < *Len; i++) {
        uint16_t next_head = (ring_head + 1) % USB_RING_BUFFER_SIZE;

        // Check for overflow
        if (next_head != ring_tail) {
            usb_ring_buf[ring_head] = Buf[i];
            ring_head = next_head;
        } else onUsbError(USB_OVERFLOW);
    }
}

void usbProcessor(uint32_t param) {
    static char local_cmd_buf[USB_CMD_BUFFER_SIZE];
    static uint16_t cmd_index = 0;
    char name[TASK_NAME_LENGTH+1];

    while (ring_tail != ring_head) {
        char c = usb_ring_buf[ring_tail];
        ring_tail = (ring_tail + 1) % USB_RING_BUFFER_SIZE;

        if (c == '\r' || c == '\n') {
            if (cmd_index > 0) {
                local_cmd_buf[cmd_index] = '\0';
                strncpy(cmd, local_cmd_buf, USB_CMD_BUFFER_SIZE);
                strncpy(name, cmd, TASK_NAME_LENGTH);
                exec(name, &onCommand);
                cmd_index = 0;
            }
        } else if (cmd_index < USB_CMD_BUFFER_SIZE - 1) {
            local_cmd_buf[cmd_index++] = c;
        } else {
            cmd_index = 0;  // overflow reset
        }
        // allow realtime
        kernel_process(param); // provide depth to avoid too many unfinished tasks
    }
}

__attribute__((weak))  uint8_t onCommand(uint32_t param) {
	printf("\x1b[33musb> %s\x1b[0m\n", cmd);
}

__attribute__((weak))  void onUsbError(uint32_t flag) {
	printf("\x1b[31mUsb Error %i\x1b[0m\n", flag);
}

// run from main.c after GPIO init
extern USBD_HandleTypeDef hUsbDeviceFS;
void resetUSBPort() {


#if defined(STM32F3)

  // 1. Enable GPIOA clock (should already be enabled)
  __HAL_RCC_GPIOA_CLK_ENABLE();

  // 2. Configure PA12 (USB D+) as output
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // 3. Drive D+ low to simulate USB disconnect
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);

  // 4. Wait ~10ms
  HAL_Delay(10);

  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF14_USB;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#else
  /* 1) stop USB device (soft-disconnect) */
	USBD_Stop(&hUsbDeviceFS);

	/* 2) wait for host to detect detach */
	HAL_Delay(10);

	/* 3) start USB device again (soft-connect) */
	USBD_Start(&hUsbDeviceFS);
#endif

}

// usb is available, can write to usb
int usbCanWrite() {
	return (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED);
}



uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);

// implementation of console write for printf(...). Not included in console for debug purposes, even if you do not need a console, printf works.
int _write(int file, char *data, int len)
{
    if (!usbCanWrite()) {
        return 0;  // USB not ready
    }

	while (CDC_Transmit_FS((uint8_t*)data, len) == USBD_BUSY) {
		kernel_process(1); // TODO! depth required
	}
    return len;
}


#endif
