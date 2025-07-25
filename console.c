/*
 * console.c
 *
 *  Created on: Jun 14, 2025
 *      Author: Ivars Renge
 *      Console implementation on printf base
 */

#include "console.h"

#ifdef USING_CONSOLE


#ifdef USING_RICH_CONSOLE

	static consoleApp *appList = NULL;

	void monitorAppDraw() {
		printf("--- < App > ---");
	}

	void consoleGui(char* name) {
		//consoleAppRegister("monitor", monitorAppDraw,  5,  5, 50, 20);
		consoleAppRegister("Test", monitorAppDraw, 5, 5, 50, 20, WHITE, BLUE);
	}


	static void drawAppWindow(const consoleApp *a) {
	    // 1) fill background
	    setBackgroundColor(a->bg);
	   // for (int row = 0; row < a->h; ++row) {
	    //    gotoxy(a->x, a->y + row);
	    //    for (int col = 0; col < a->w; ++col) putchar(' ');
	    //}

	    // 2) draw frame (borders) in fg
	    setBackgroundColor(a->bg);
	    setTextColor(a->fg);
	    drawFrame(a->x, a->y, a->w, a->h);

	    // 3) call the app’s content
	    gotoxy(a->x + 1, a->y + 1);
	    a->onDraw();

	    // 4) reset to default colors for next draws
	    setTextColor(DEFAULT_COLOR);
	    setBackgroundColor(DEFAULT_COLOR);
	    printf("\n");
	}


	void  consoleAppDrawAll(uint32_t par) {
		consoleApp *a = appList;

		if (!a) return;

		cls();

		while (a) {
			if (a->visible) {
				drawAppWindow(a);
			}
			a = a->next;
		}


	}


	void 	 consoleAppRegister(char *name, void (*onDraw)(int x, int y), uint8_t x, uint8_t y, uint8_t w, uint8_t h, enum ConsoleColor fg, enum ConsoleColor bg) {
	    consoleApp* app = malloc(sizeof(consoleApp));
	    if (!app) return;

	    *app = (consoleApp){ .name = name,
	                           .onDraw = onDraw,
	                           .x = x, .y = y,
	                           .w = w, .h = h,
	                           .fg = fg, .bg = bg,
	                           .visible = 1,
	                           .next = appList };

	    appList = app;

	}


	// Moves the cursor to column x, row y (1-based coordinates)
	void gotoxy(int x, int y) {
	    printf("\x1B[%d;%dH", y, x);
	}

	// Clears the screen and moves cursor to home (1,1)
	void cls(void) {
	    printf("\x1B[2J\x1B[H");
	}

	// Draws a rectangular frame at (x,y) with width w and height h
	// Uses '+' for corners, '-' for horizontal edges, '|' for vertical edges
	void drawFrame(int x, int y, int w, int h) {
	    if (w < 2 || h < 2) return; // need at least space for corners

	    // Top edge
	    gotoxy(x, y);
	    putchar('+');
	    for (int i = 0; i < w - 2; ++i) putchar('-');
	    putchar('+');

	    // Sides
	    for (int row = 1; row < h - 1; ++row) {
	        gotoxy(x, y + row);
	        putchar('|');
	        gotoxy(x + w - 1, y + row);
	        putchar('|');
	    }

	    // Bottom edge
	    gotoxy(x, y + h - 1);
	    putchar('+');
	    for (int i = 0; i < w - 2; ++i) putchar('-');
	    putchar('+');
	}

	// Fills a rectangular area at (x,y) with width w and height h
	void fillFrame(int x, int y, int w, int h) {
	    if (w < 1 || h < 1) return;
	    for (int row = 0; row < h; ++row) {
	        gotoxy(x, y + row);
	        for (int col = 0; col < w; ++col) {
	            putchar(' ');
	        }
	    }
	}

	// Resets terminal: clears formatting and moves cursor to bottom of window
	void resetTerminal(void) {
	    // Reset colors and attributes
	    printf("\x1B[0m");
	    // Ensure cursor is visible
	    printf("\x1B[?25h");
	    // Move cursor far down (clamped to bottom)
	    printf("\x1B[999;1H");

	#ifdef USING_USB
	    fflush(stdout);
	#endif
	}


#endif



// Sets the text (foreground) color; pass one of ConsoleColor
void setTextColor(enum ConsoleColor color) {
    // DEFAULT_COLOR maps to CSI 39
#ifdef USING_RICH_CONSOLE
    int code = (color == DEFAULT_COLOR) ? 39 : (30 + color);
    printf("\x1B[%dm", code);
#endif
}

// Sets the background color; pass one of ConsoleColor
void setBackgroundColor(enum ConsoleColor color) {
#ifdef USING_RICH_CONSOLE
	// DEFAULT_COLOR maps to CSI 49
    int code = (color == DEFAULT_COLOR) ? 49 : (40 + color);
    printf("\x1B[%dm", code);
#endif
}




static consoleCmd* consoleCmdList = NULL;

void consoleInit(uint32_t msg) {
	consoleRegister("about", &consoleAbout);
	consoleRegister("gui", &consoleGui);
	consoleRegister("help", &consoleHelp);
	consoleRegister("taskreset", &consoleTasksReset);
	consoleRegister("tasks", &consoleTasks);
	consoleRegister("on", &consoleOn);
	consoleRegister("off", &consoleOff);


	consoleAbout(NULL);

	printf("console loaded\n");

#ifdef USING_RICH_CONSOLE
	repeat("GUI", ST_SEC, &consoleAppDrawAll);
#endif
}



void consoleRegister(char* name, consoleCmdHandler handler) {
    consoleCmd* cmd = malloc(sizeof(consoleCmd));
    if (!cmd) return;

    cmd->name = name;
    cmd->handler = handler;
    cmd->next = consoleCmdList;
    consoleCmdList = cmd;
}

__attribute__((weak))  uint8_t onCustomCommand(char* command) {
	// to handle this, place on user file
	setTextColor(RED);
	printf("console error> Unknown command %s\n", command);
	setTextColor(DEFAULT_COLOR);
}


uint8_t onCommand(uint32_t param) {
	char* command = cmd;
	char* space = strchr(command, ' ');
	char* args = "";

	if (space) {
		*space = '\0';
		args = space + 1;
	}

	consoleCmd* current = consoleCmdList;
	while (current) {
		if (strcmp(current->name, command) == 0) {
			return current->handler(args);
		}
		current = current->next;
	}

	return onCustomCommand(command);
}


// specific commands

void consoleAbout(char* args) {
    uint32_t t = uwTick / ST_SEC;

#ifdef USING_RICH_CONSOLE
	setTextColor(WHITE);
	setBackgroundColor(BLUE);
	fillFrame(5, 3, 40, 10);
	drawFrame(5, 3, 40, 10);
	gotoxy(15, 3);
    printf("--- < About > ---\n");
    gotoxy(7, 6);
    printf("Firmware: %s (%s)\n", FIRMWARE_VERSION, __TIMESTAMP__);
    gotoxy(7, 7);
#ifdef DEBUG
    printf("DEBUG ");
#endif
    printf("Build: %s %s\n", __DATE__, __TIME__);
    gotoxy(7, 8);
    printf("Alive %ud %02u:%02u:%02u\n",
           (unsigned int)t / 86400,         // days
		   (unsigned int)(t / 3600) % 24,   // hours
		   (unsigned int)(t / 60) % 60,     // minutes
		   (unsigned int)t % 60);           // seconds
    gotoxy(7, 9);
    printf("Type 'help' for commands.\n\x1b[0m");
	resetTerminal();
#else
    printf("About\n");
    printf("Firmware: %s\n", FIRMWARE_VERSION);
    printf("Build: %s %s\n", __DATE__, __TIME__);
    printf("Warning: USING_RICH_CONSOLE not defined\n");
#ifdef DEBUG
    printf("Debug build\n");
#else
    printf("Release build\n");
#endif

#endif

}

/*
void consoleHelp(char* args) {
	consoleCmd* current = consoleCmdList;
	printf("Registered console commands:\n");
	int cnt = 0;
	while (current) {
		printf(" -%s (%p)\n", current->name, (void*)current->handler); //
		current = current->next;
		cnt++;
		kernel_process(1);
	}
	printf("Total (%i)\n", cnt);
	return;
}*/


void consoleHelp(char* args) {
    consoleCmd* current = consoleCmdList;
    size_t filter_len = (args && args[0]) ? strlen(args) : 0;
    int cnt = 0;

    if (filter_len) {
        printf("Filtered \"%s\":\n", args);
    } else {
        printf("Available:\n");
    }

    while (current) {
        if (!filter_len || strncmp(current->name, args, filter_len) == 0) {
            printf(" - %s (%p)\n", current->name, (void*)current->handler);
            cnt++;
        }
        current = current->next;
        kernel_process(1);
    }

    if (filter_len) {
        printf("Matching: %d\n", cnt);
    } else {
        printf("Total: %d\n", cnt);
    }
}

#endif




int getPortPinFromArgs(char* args, GPIO_TypeDef** port, uint16_t* pin) {
    char portChar;
    int pinNum;

    if (sscanf(args, "P%c%d", &portChar, &pinNum) != 2) {
        return 0;  // parsing failed
    }

    switch (portChar) {
        case 'A': *port = GPIOA; break;
        case 'B': *port = GPIOB; break;
        case 'C': *port = GPIOC; break;
        case 'D': *port = GPIOD; break;
        case 'E': *port = GPIOE; break;
        default: return 0;  // invalid port
    }

    if (pinNum < 0 || pinNum > 15) {
        return 0;  // invalid pin
    }

    if (pinNum == 0) {
		*pin = GPIO_PIN_All;      // special: all pins
	} else {
		*pin = (1u << pinNum);    // single-pin mask
	}
    return 1;
}


void consoleOn(char* args) {
    GPIO_TypeDef* port;
    uint16_t mask;
    if (!getPortPinFromArgs(args, &port, &mask)) {
        printf("Usage: on PE0  (0 → all pins)\n");
        return;
    }

    HAL_GPIO_WritePin(port, mask, GPIO_PIN_SET);
    if (mask == GPIO_PIN_All) {
        printf("P%c ALL ON\n", args[1]);
    } else {
        printf("P%c%d ON\n", args[1], __builtin_ctz(mask));
    }
}

void consoleOff(char* args) {
    GPIO_TypeDef* port;
    uint16_t mask;
    if (!getPortPinFromArgs(args, &port, &mask)) {
        printf("Usage: off PE0  (0 → all pins)\n");
        return;
    }

    HAL_GPIO_WritePin(port, mask, GPIO_PIN_RESET);
    if (mask == GPIO_PIN_All) {
        printf("P%c ALL OFF\n", args[1]);
    } else {
        printf("P%c%d OFF\n", args[1], __builtin_ctz(mask));
    }
}
