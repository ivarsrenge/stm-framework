/*
 * buttons.c
 *
 *  Created on: Jun 29, 2025
 *      Author: Ivars Renge
 *      Input button usage library
 */


#include "buttons.h"

#ifdef USING_BUTTON

static button* buttonChainList = NULL;

void buttonInit(uint32_t msg) {
	tTask* proc = repeat("BTN_PROC",BUTTON_RATE, &buttonProcessor);
	proc->timeout = ST_SEC;
	proc->realtime_fail = BUTTON_RATE;

	printf("buttons loaded\n");
}

button* buttonAdd(char* name, GPIO_TypeDef* port, uint8_t pinNumber, uint8_t type) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    button* btn = malloc(sizeof(button));
    if (!btn) return NULL;

    btn->name =  name;
    btn->type = type;
    btn->port = port;
    btn->pinNumber = pinNumber;
    btn->prevState = 0;
    btn->next = buttonChainList;

    btn->eventDown = NULL;
    btn->eventUp = NULL;
    buttonChainList = btn;

    /*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = 1U << pinNumber;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = type;
	HAL_GPIO_Init(port, &GPIO_InitStruct);
	return btn;
}

void buttonProcessor(uint32_t depth) {
	button* current = buttonChainList;
	char val = 0;

	while (current) {
		val = (HAL_GPIO_ReadPin(current->port, 1U << current->pinNumber) ? (current->type == BTN_POSITIVE ? 1 : 0) : (current->type == BTN_POSITIVE ? 0 : 1));
		if (current->prevState != val) {
			if (val) {
				if (current->eventDown != NULL) {
					exec(current->name, current->eventDown);
				}
			} else {
				if (current->eventUp != NULL) {
					exec(current->name, current->eventUp);
				}
			}

			current->prevState = val;
		}

		current = current->next;
	}
}

#endif
