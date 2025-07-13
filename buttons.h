/*
 * buttons.h
 *
 *  Created on: Jun 29, 2025
 *      Author: Ivars Renge
 *
 *      Dependencies
 *      Make sure you include HAL of the controller like stm32f303xc, containing port definitions.
 */

#ifndef SYS_BUTTONS_H_
#define SYS_BUTTONS_H_

#include "core.h"

#ifdef USING_BUTTON

	typedef int (*btnHandler)(uint16_t);


	#define BUTTON_RATE ST_MS

	#define  BTN_NEGATIVE GPIO_PULLUP // 0 - off, 1 on
	#define  BTN_POSITIVE GPIO_PULLDOWN // 1 - off, 0 - on

	typedef struct button {
		char* name;
		uint8_t type;
		uint8_t pinNumber; // 0, 1, 2, 3, 4...
		uint8_t prevState;
		GPIO_TypeDef* port; // PORTA
		struct button* next;
		btnHandler* eventDown;
		btnHandler* eventUp;
	} button;

	void buttonInit(); // initialize framework
	void buttonProcessor(uint32_t); // real Time Task

	button* buttonAdd(char*, GPIO_TypeDef*, uint8_t, uint8_t); // app uses a button buttonAdd("Temp", GPIOA, 0, BTN_POSITIVE);

#endif

#endif /* SYS_BUTTONS_H_ */
