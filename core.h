/*
 * core.h
 *
 *  Created on: Jun 8, 2025
 *      Author: Ivars Renge
 *
 *	Provides basic core functionality for task handling.
 *
 */

#ifndef SYS_CORE_H_
#define SYS_CORE_H_

	#include <stdint.h>
	#include <main.h>
	#include "stdlib.h"

	// other sys libraries
#ifdef USING_USB
	#include "usb.h"
#else
	#ifdef USING_UART
		#include "uart.h"
	#endif
#endif


#ifdef USING_CONSOLE
	#include "console.h"
#endif

#ifdef USING_FILESYSTEM
	#include "filesystem.h"
#endif

#ifdef USING_BUTTON
	#include "buttons.h"
#endif


	#define TASK_TIMEOUT  1000  // how long is allowed the task to execute - MICROSECONDS, 1ms by default
	#define TASK_REALTIME_FAIL ST_MS * 3 // how long is allowed to shift from realtime
	#define TASKER_ULTIMATE_LIMIT 128 // max task count
	#define TASKER_ULTIMATE_DEPTH 8 // recursion depth
	#define TASK_NAME_LENGTH 8

	// ========== types =====================
	typedef struct tTask tTask; // alias



	struct tTask {
		char name[TASK_NAME_LENGTH + 1];
		unsigned int timeout;
		unsigned int realtime_fail;
		unsigned int cycleLength;
		unsigned int runAt;
		unsigned char state;
		unsigned char error_flag;
		uint32_t counter;
		uint64_t duration;
		void (*callback)(uint32_t);
		 tTask *next;
	};

	// quickly translate timing in user friendly names
	// x10
	enum {
	    ST_MS = 1 ,
	    ST_SS = 10  ,
	    ST_S10 = 100  ,
	    ST_SEC = 1000  ,
	    ST_MIN = 60000
	};

	enum {
		    TT_ONCE = 1,
		    TT_REPEAT = 2,
		    TT_PRIORITY = 4
		};

	enum {
		TE_TIMEOUT = 1,
		TE_REALTIME = 2,
		TE_ULTIMATE_LIMIT = 4,
		TE_ULTIMATE_DEPTH = 8
	};

	enum {
		TS_READY = 0,
		TS_RUNNING = 1,
	};

	// =============  user API ======================

	// Schedule a task. Returns a pointer to the created task for later modification.
	// `repeat` = 1 for periodic task, 0 for one-shot.
	// sheduling one by one, it doesn`t remove duplicates
	// repeat will assure cyclic repeating.

	void kernel_process(int); // park in long functions to allow simultanious tasks
	void osDelay(uint32_t time); // can be used on driver inicialization to avoid halting...

	tTask* taskSchedule(char* name, int after, int type, void (*callback)(uint32_t));

	// remove all tasks by handler function, returns count
	uint32_t taskRemove(void (*callback)(uint32_t));

	// check if task exists, returns number of instances of handler
	uint32_t taskExists(void (*callback)(uint32_t));

	// add message and params to task
	void taskSetData(tTask* task, uint32_t msg);


	// ================ macros ===================
	// create immediate task to be run on the main thread, assuring once
	#define exec(n, b) taskSchedule(n, 0,TT_ONCE,b)
	#define execPriority(n, b) taskSchedule(n, 0,TT_ONCE | TT_PRIORITY,b)

	// create non-repeatable task
	#define after(n,a,b) taskSchedule(n,a,0,b)
	#define afterOnce(n,a,b) taskSchedule(n,a,TT_ONCE,b)

	// create repeatable task
	#define repeat(n,a,b) taskSchedule(n,a,TT_REPEAT | TT_ONCE,b)
    #define repeatPriority(n,a,b) taskSchedule(n,a,TT_PRIORIT | TT_REPEAT | TT_ONCE,b)



	// ===============  kernel functions ===================
	// must be included in main.c after hal init
	void onBoot();


	// microsecond timers for task timing
	void usTimerInit(void);
	uint32_t usTimerRead(void);

	// can declare on tasks.c file, it will be called before periphery is loaded
	__attribute__((weak))  void onBeforeLoad(uint32_t);
	// can declare on tasks.c file, it will be called once the scheduler is ready
	__attribute__((weak))  void onLoad(uint32_t);


	// can declare on tasks.c file, it will be called once the scheduler executes task longer than taskTimeout.
	// can declare on tasks.c file, it will be called once the scheduler fails to execute the task on time
	__attribute__((weak))  void onTaskError(tTask*, uint32_t, uint32_t);






#endif /* SYS_CORE_H_ */
