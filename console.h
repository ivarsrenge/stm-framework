/*
 * console.h
 *
 *  Created on: Jun 14, 2025
 *      Author: Ivars Renge
 */

#ifndef SYS_CONSOLE_H_
#define SYS_CONSOLE_H_

#include "core.h"

// define USING_CONSOLE in your header file
// define USING_RICH_CONSOLE to enable colors on console & apps
/*
Concept	Description
Application
	A named UI element that can render itself in a window, handle its own drawing, and be closed.
onDraw Callback
	Per app function that knows how to paint the app’s contents (inside its window frame).
Window Descriptor
	Each app carries its own (x,y,width,height) so it only redraws within its box.
Visibility
	Apps live in a linked list; only those with visible == true get drawn.
Kill Command
	A console command (kill <app>) that marks an app invisible (or removes it from the list).
Rich Only
	The draw machinery only compiles/executes when USING_RICH_CONSOLE is defined.



	If gui is enabled, apps must be filled on console from virtual buffer screen
	Probably should be consoleApp include USING_APP with defined screen size 80x40?
	Array should contain chars, colors could be assigned into apps, to reduce load...



                       +--------<Application>---<x>-+
                       |							|
                       | This Content should be ref |
                       | reshed in like 1/10sec		|
                       +----------------------------+

	+-<Status>--<x>-+
	| Status app    |
	+---------------+

Goal: smooth, flicker‑free windows with per‑cell colors over a simple serial link.

Trade‑off: full redraw is easiest but floods the wire; per‑app dirty flags need careful overlap management.

Double‑buffer approach:

Keep a “last drawn” and “next frame” screen in RAM.

Render every app into the next frame buffer.

Diff the two buffers and only emit the ANSI sequences for changed cells.

Why it wins: uses a modest ~9 KB RAM, sends the minimal data each refresh, avoids flicker, and doesn’t require complex region logic—trading a bit of CPU for stable, clean updates.







*/
#ifdef USING_CONSOLE

typedef int (*consoleCmdHandler)(char* args);


typedef struct consoleCmd {
    const char* name;
    consoleCmdHandler handler;
    struct ConsoleCmd* next;
} consoleCmd;




typedef struct consoleCmd consoleCmd;

void consoleInit(uint32_t);
void consoleRegister(char* name, consoleCmdHandler handler);

// Resets terminal: clears formatting and moves cursor to bottom of window
void resetTerminal(void);


void consoleHelp(char* args);
void consoleAbout(char* args);
void consoleTasks(char* args);
void consoleTasksReset(char* args);
void consoleOn(char* args);
void consoleOff(char* args);




	#ifdef USING_RICH_CONSOLE
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

	typedef struct consoleApp {
		const char           *name;       // unique identifier
		void                (*onDraw)();    // draw callback
		uint8_t               x, y;       // top left corner (based console coords)
		uint8_t               w, h;       // window size
		enum ConsoleColor   		fg;
		enum ConsoleColor   		bg;
		char                  visible;   // show or hide
		struct consoleApp    *next;
	} consoleApp;

	// Register a new application. Must be done before using it.
	void 	 consoleAppRegister(char *name, void (*onDraw)(), uint8_t x, uint8_t y, uint8_t w, uint8_t h, enum ConsoleColor fg, enum ConsoleColor bg);
	void     consoleAppShow(char *name);// Show (open) a registered app by name
	void     consoleAppHide(char *name);// Hide (close) a visible app by name
	void     consoleAppKill(char *name);// Remove an app completely
	void     consoleAppDrawAll(uint32_t);// Draw all visible apps (called from your main loop or console refresh)

	void gotoxy(int x, int y);
	void cls(void);
	void drawFrame(int x, int y, int w, int h);
	void fillFrame(int x, int y, int w, int h);

	#endif



	void setTextColor(enum ConsoleColor color);
	void setBackgroundColor(enum ConsoleColor color);


#endif

#endif /* SYS_CONSOLE_H_ */
