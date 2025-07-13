/*
 * console.h
 *
 *  Created on: Jun 14, 2025
 *      Author: User
 */

#ifndef SYS_CONSOLE_H_
#define SYS_CONSOLE_H_

#include "core.h"

// define USING_CONSOLE in your header file
#ifdef USING_CONSOLE

typedef int (*consoleCmdHandler)(char* args);

enum ConsoleColor {
    BLACK = 0,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
    DEFAULT_COLOR = 9
};

typedef struct consoleCmd {
    const char* name;
    consoleCmdHandler handler;
    struct ConsoleCmd* next;
} consoleCmd;

typedef struct consoleCmd consoleCmd;

void consoleInit(uint32_t);
void consoleRegister(char* name, consoleCmdHandler handler);

void gotoxy(int x, int y);
void cls(void);
void setBackgroundColor(enum ConsoleColor color);
void drawFrame(int x, int y, int w, int h);
void fillFrame(int x, int y, int w, int h);
// Resets terminal: clears formatting and moves cursor to bottom of window
void resetTerminal(void);


void consoleHelp(char* args);
void consoleAbout(char* args);
void consoleTasks(char* args);
void consoleTasksReset(char* args);
void consoleOn(char* args);
void consoleOff(char* args);


#endif

#endif /* SYS_CONSOLE_H_ */
