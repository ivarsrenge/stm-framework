/*
 * core.c
 *
 *  Created on: Jun 8, 2025
 *      Author: User
 *
 *
 *      Your code shows a functional and relatively compact custom task scheduler for STM32, and you're using solid embedded development practices. However, there's room to **enhance readability, safety, maintainability, and clarity**. Here's a **code quality evaluation** broken down by category:

---
*/


#include "core.h"


static tTask taskQueue = { 0 };
uint32_t taskTimeout = TASK_TIMEOUT;
uint32_t taskRealtimeFail = TASK_REALTIME_FAIL;
uint32_t tasksTotalExecuted = 0;


void onBoot() {
	// core tasks
	int timing = 10;

	//SysTick_Config(1600);//5x speed Temporary !TODO

	usTimerInit();

	taskQueue.next = NULL;
	exec("BOOT",&onBeforeLoad);

	// libs
#ifdef USING_USB
	tTask* usbi = after("USB_INIT",timing+=10, &usbInit); // this can start asap
	usbi->timeout = 1000 * ST_MS * 15;
	timing+=ST_SEC;
#else
	#ifdef USING_UART
		tTask* usbi = after("UART_INIT",timing+=10, &uartInit); // this can start asap
		usbi->timeout = 1000 * ST_MS * 15;
		timing+=ST_SEC;
	#endif
#endif


#ifdef USING_CONSOLE
	tTask* cns = after("CNS_INIT",timing+=10, &consoleInit);
	cns->timeout = 1000 * ST_MS * 50;
#endif

#ifdef USING_BUTTON
	after("BTN_INIT", timing+=10, &buttonInit);
#endif


#ifdef USING_FILESYSTEM
	after("FS_INIT", timing+=10, &fsInit);
#endif

	after("LOAD",timing+=10, &onLoad);
	while (1==1) kernel_process(0);
}

__attribute__((weak))  void onLoad(uint32_t param) {

}

__attribute__((weak))  void onBeforeLoad(uint32_t param) {

}

__attribute__((weak))  void onTaskError(tTask* task, uint32_t msg, uint32_t time) {
	// error repeats once
	if (task!=NULL) {
		if (task->error_flag & msg)
			return;
		else
			task->error_flag|=msg;
	}

	printf("\x1b[31m");
	if (task!=NULL) {
		task->name[TASK_NAME_LENGTH] = '\0';
		printf("Task Error> [%s]:", task->name);
	} else {
		printf("Tasker Error>");
	}

	switch(msg){
		case TE_REALTIME:
			printf(" missed realtime limits by %ims", (int)time);
			break;
		case TE_TIMEOUT:
			printf(" executes too long - %ius", (int)time);
			break;
	}
	printf("\x1b[0m\n");

}


void kernel_process(int depth) {
	uint8_t (*handler)(uint32_t);
	tTask *current = &taskQueue;
	tTask *prev;
	uint32_t beforeT;
	uint32_t runningTaskCount = 0;
	uint8_t result = 0;

	if (depth > TASKER_ULTIMATE_DEPTH) {
		onTaskError(NULL, TE_ULTIMATE_DEPTH, 0);
		return;
	}

	while (current && current->next != NULL) {
		if (++runningTaskCount > TASKER_ULTIMATE_LIMIT) {
			onTaskError(NULL, TE_ULTIMATE_LIMIT, 0);
			return;
		}

		prev = current;
		current = current->next;

		if ((int32_t)(uwTick - current->runAt) >= 0 && current->callback && current->state == TS_READY) {

			handler = current->callback;

			// check realtime offset
			if (uwTick - current->runAt > current->realtime_fail) {
				onTaskError(current, TE_REALTIME, (uwTick - current->runAt) - current->realtime_fail);
			}

			tasksTotalExecuted++;


			if (current->cycleLength) { // repeatative tasks
				beforeT = usTimerRead();
				current->state = TS_RUNNING;
				result = handler(depth+1);
				current->state = TS_READY;
				current->counter++;
				current->duration+=(usTimerRead() - beforeT);

				if (!result)
					if ((int32_t)(usTimerRead() - beforeT) > (int32_t)current->timeout)
						onTaskError(current, TE_TIMEOUT, usTimerRead() - beforeT);

				current->runAt += current->cycleLength;
			} else { // once tasks
				prev->next = current->next;
				beforeT = usTimerRead();
				current->state = TS_RUNNING;
				result = handler(depth+1);
				current->state = TS_READY;

				if (!result)
					if ((int32_t)(usTimerRead() - beforeT) > (int32_t)current->timeout)
						onTaskError(current, TE_TIMEOUT, usTimerRead() - beforeT);
				free(current);
				current = prev->next;
			}

		}
	}
}

// allow to handle some task
void osDelay(uint32_t time) {
	uint32_t haltAt = uwTick + time;
	while ((int32_t)(uwTick - haltAt) < 0) {
		kernel_process(1);
	}
}



// microsecond timer for debugging tasks
/** @brief  Initialize DWT CYCCNT for microsecond timing.
 *   Enables the cycle counter and resets it to zero.
 */
void usTimerInit(void) {
    // Enable trace and debug blocks
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // Reset the cycle counter
    DWT->CYCCNT = 0;
    // Enable the cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}


uint32_t usTimerRead(void) {
    // Number of CPU cycles since init
    uint32_t cycles = DWT->CYCCNT;
    // Convert to microseconds: cycles / (SystemCoreClock / 1_000_000)
    return cycles / (SystemCoreClock / 1000000U);
}



tTask* taskSchedule(char *name, int after, int type, void (*handler)()) {
	if (type & TT_ONCE) taskRemove(handler);
	tTask *current = &taskQueue;


	tTask *task = malloc(sizeof(tTask));
	if (!task) return NULL;

	strncpy(task->name, name, TASK_NAME_LENGTH);
	task->error_flag = 0;
	task->counter = 0;
	task->duration = 0;
	task->timeout = TASK_TIMEOUT;
	task->state = TS_READY;
	task->realtime_fail = TASK_REALTIME_FAIL;
	task->runAt = uwTick + after;
	task->callback = handler;
	task->cycleLength = (type & TT_REPEAT ? after : 0);
	task->next = NULL;


	while (current->next) {
		// if next task runs later, ready to insert, for priority, insert on equal as well

		// if task is not with high priority and other tasks are scheduled at the same tick, add a tick
		if ((!(type & TT_PRIORITY)) && (current->next->runAt == task->runAt)) task->runAt++;

		if (current->next->runAt > task->runAt) {
			task->next = current->next;
			current->next = task;
			return task;
		}
		current = current->next;
	}

	current->next = task;
	return task;
}



uint32_t taskRemove(void (*handler)()) {
	tTask *current = &taskQueue;
	tTask *prev;
	uint32_t count = 0;
	while (current->next != NULL) {
		prev = current;
		current = current->next;

		if (current->callback == handler) {
			prev->next = current->next;
			free(current);
			count++;
			current = prev->next;
		}
	}
	return count;
}

uint32_t taskExists(void (*handler)()) {
	tTask *current = &taskQueue;
	uint32_t count = 0;
	while (current->next != NULL) {
		current = current->next;
		if (current->callback == handler) {
			count++;
		}
	}
	return count;
}

#ifdef USING_CONSOLE
void consoleTasks(char* args) {
	tTask* current = taskQueue.next;
    printf("\n === Debug Tasks ===\n");
	printf("Time now %lu:\n",  uwTick);
	while (current) {
		//current->callback =  cb=%p
		current->name[TASK_NAME_LENGTH] = '\0';

		if (current->state == TS_RUNNING)  printf("<RUNNING> ");

		printf("-[%s] runAt=%lu, wait=%i ", current->name, (unsigned long int)current->runAt, abs(uwTick - current->runAt));
		if (current->error_flag) {
			if (current->error_flag | TE_REALTIME) printf("REALTIME %i ",(int)current->realtime_fail);
			if (current->error_flag | TE_TIMEOUT) printf("FROZE %i ", (int)current->timeout);
		}
		if (current->counter) {
			printf("Cnt: %i ", (int)current->counter);
			printf("Dur (avg): %ius ", (int)(current->duration / (uint64_t)current->counter));
		}
		printf("\n");
		current = current->next;
	}
}
void consoleTasksReset(char* args) {
	tTask* current = taskQueue.next;
    printf("\n === Reset Tasks ===\n");
	while (current) {
		//current->callback =  cb=%p
		current->name[TASK_NAME_LENGTH] = '\0';
		printf("-[%s] ", current->name);
		if (current->error_flag) printf("CL-EF");
		current->error_flag = 0;
		current->counter = 0;
		current->duration = 0;
		printf("\n");
		current = current->next;
	}
}
#endif


