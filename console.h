//-----------------------------------------------------------------------------
// File: console.h
//
// Text console
//-----------------------------------------------------------------------------

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdarg.h>
#include <stdio.h>
#include "types.h"
#include "str.h"

#ifdef _DEBUG
#define DEBUG_CONSOLE
#endif

class display_list_t;

// Always ensure that CONSOLE_BUFFER_SIZE > CONSOLE_LINES
// Always ensure that CONSOLE_BUFFER_SIZE > CONSOLE_MAX_LINE_LENGTH
#define CONSOLE_LINES				1024			// Most lines to scrollback
#define CONSOLE_BUFFER_SIZE			65536			// Most scrollback text available
#define CONSOLE_MAX_LINE_LENGTH		256				// Longest complete line of text

#define CONSOLE_COMMAND_LENGTH		64				// Longest typeable console command
#define CONSOLE_COMMAND_COUNT		32				// Number of commands in console scrollback
#define CONSOLE_SCROLL_TIME			2.0f			// Number of seconds before scrolling

extern const char* const DIVIDER;	// A line of '=' chars

class console_t {
	// Command console, and general logging mechanism, the console works like a
	// restartable block to write to. Whenever writing more text would result
	// in going off the end of the text buffer then writing restarts at the
	// beginning of the buffer overwriting as it goes
public:
	~console_t();

	bool show_warnings;
	bool show_developer;

	// Print this message to console always
	void print(const char* msg);
	void printf(const char* format, ...);

	// Print only if warnings enabled
	void warn(const char* msg);
	void warnf(const char* format, ...);

	// Print only if developer messages enabled
	void dev(const char* msg);
	void devf(const char* format, ...);

	// Print only if in debug mode (these are inlined below)
	void debug(const char* msg);
	void debugf(const char* format, ...);

	const char* get_line(int line);

	void show() { showing = true; }
	void hide() { showing = false; }
	
	void toggle_visible() { showing = !showing; }

	bool is_showing() const { return showing; }

	void set_shader(hshader_t s) { shader = s; }
	void tesselate(display_list_t& dl);

	static console_t& get_instance();

	float get_visibility() const { return visibility; };

	void scroll_up();
	void scroll_down();
	void cursor_left();
	void cursor_right();
	void cursor_up();
	void cursor_down();
	void char_typed(char c);
	void delete_left();		// Delete to the left of the caret
	void delete_right();	// Delete to the right of the caret
	void toggle_insert();	// Toggle insert/overwrite

private:
	typedef str_t<CONSOLE_COMMAND_LENGTH> command_t;

	console_t();

	void vprintf(const char* format, va_list val);
	void update_lines();

	FILE* fp;
	hshader_t shader;

	char text[CONSOLE_BUFFER_SIZE];
	char* lines[CONSOLE_LINES];

	bool showing;	// Is the console currently showing

	float visibility;	// 1.0f fully visible, 0.0f fully hidden
	int pos;		// insertion point in the buffer
	int headline;	// Line to write more text at
	int tailline;	// Oldest line that hasnt been overwritten yet

	float times[3];	// Times when the last 3 lines of text were shown

	command_t commands[CONSOLE_COMMAND_COUNT];
	int current_command;	// index into command buffer for the command being typed
	int command_scroll_pos;	// used when scrolling through the command buffer
	int console_scroll_pos;	// used when scrolling up through console output
	int caret_pos;			// caret position
	bool insert;			// insert or overwrite
};

inline void 
console_t::debug(const char* msg)
	// Print a message if in debug mode
{
#ifdef DEBUG_CONSOLE
	print(msg);
#endif
}

inline void
console_t::debugf(const char* format, ...)
	// Print a formatted message if in debug mode
{
#ifdef DEBUG_CONSOLE
	va_list argptr;
	va_start(argptr, format);
	vprintf(format, argptr);
	va_end(argptr);
#endif
}

#define console (console_t::get_instance())

#endif
