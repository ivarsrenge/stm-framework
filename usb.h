/*
 * usb.h
 *
 *  Created on: Jun 14, 2025
 *      Author: Ivars Renge
 *
  *      Usb Driver for console
 *
 *      1. Implement CDC on ioc file
 *      2. Define USING_USB
 *      3. Add usbReceiveBuffer as first line in CDC_Receive_FS in usbd_cdc_if.c
 *      #ifdef USING_USB
  	  	  	  usbReceiveBuffer(Buf, Len);
		#endif
 *
 */

#ifndef SYS_USB_H_
#define SYS_USB_H_

#include "core.h"

// define USING_USB in your header file
#ifdef USING_USB
	#include "usb_device.h"
	#include "usbd_core.h"
	#include "usbd_desc.h"
	#include "usbd_cdc.h"
	#include "usbd_cdc_if.h"

	// error codes
	#define USB_OVERFLOW 1 // ring buffer too small

	#define	USB_PROCESSOR_SPEED ST_MS

	#define USB_CMD_BUFFER_SIZE 128
	#define USB_RING_BUFFER_SIZE 128


	extern volatile uint16_t ring_head;
	extern volatile uint16_t ring_tail;
	extern char usb_ring_buf[USB_RING_BUFFER_SIZE];

	extern char cmd[USB_CMD_BUFFER_SIZE];
	extern volatile uint8_t cmdLoaded;

	void usbInit(uint32_t);
	void usbProcessor(uint32_t param);
	void usbReceiveBuffer(uint8_t* Buf, uint32_t *Len);
	__attribute__((weak))  uint8_t onCommand(uint32_t);
	__attribute__((weak))  void onUsbError(uint32_t);



	int usbCanWrite();  // is usb inicialised?
	void resetUSBPort(); // reset to make it available on instant.




#endif

#endif /* SYS_USB_H_ */
