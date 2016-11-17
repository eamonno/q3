//-----------------------------------------------------------------------------
// File: console.cpp
//
// Text console
//-----------------------------------------------------------------------------

#include "console.h"
#include "util.h"
#include "displaylist.h"
#include "timer.h"
#include "font.h"
#include "exec.h"

#include <memory>

#include "mem.h"
#define new mem_new

cvar_float_t	console_max_visibility("console_max_visibility", 1.0f, CVF_NONE, 0.0f, 1.0f);
cvar_float_t	console_caret_flash_time("console_caret_flash_time", 0.5f);
cvar_str_t		console_log_file("console_log_file", "console.log", CVF_CONST);

console_t&
console_t::get_instance()
{
	static std::auto_ptr<console_t> instance(new console_t());
	return *instance;
}

const char* const DIVIDER = "===============================================================================\n";

static inline int next_line(int line) 
	// Get index of the next line pointer wrapping at CONSOLE_LINES
{
	return (++line == CONSOLE_LINES ? 0 : line); 
}

static inline int prev_line(int line)
	// Get index of the next line pointer wrapping to (CONSOLE_LINES - 1) from 0
{
	return (--line == -1 ? CONSOLE_LINES - 1 : line);
}

console_t::console_t()
	// Create the console, zero fill text and lines, Tailline is set to the
	// end of the buffer where it can happily sit until the first wraparound
	: headline(0), tailline(CONSOLE_LINES - 1), show_warnings(true), 
	  show_developer(true), visibility(0.0f), showing(true), shader(0), pos(0),
	  command_scroll_pos(0), caret_pos(0), insert(true), console_scroll_pos(0)
{
	u_zeromem(text, CONSOLE_BUFFER_SIZE * sizeof(char));

	lines[headline] = text;
	for (int i = 1; i < CONSOLE_LINES; ++i)
		lines[i] = text + CONSOLE_BUFFER_SIZE - 1;

	if ((fp = fopen(*console_log_file, "w")) == NULL)
		printf("Failed to open %s - console logging disabled", console_log_file->str());

	times[0] = times[1] = times[2] = -CONSOLE_SCROLL_TIME;
}

void
console_t::scroll_up()
{
	++console_scroll_pos;
}

void
console_t::scroll_down()
{
	if (console_scroll_pos > 0)
		--console_scroll_pos;
}

void
console_t::cursor_left()
{
	if (caret_pos > 0)
		--caret_pos;
}

void
console_t::cursor_right()
{
	if (caret_pos < commands[0].length())
		++caret_pos;
}

void
console_t::cursor_up()
{
	if (command_scroll_pos < CONSOLE_COMMAND_COUNT - 1) {
		++command_scroll_pos;
		commands[0] = commands[command_scroll_pos];
		caret_pos = commands[0].length();
	}
}

void
console_t::cursor_down()
{
	if (command_scroll_pos > 0) {
		--command_scroll_pos;
		if (command_scroll_pos > 0) {
			commands[0] = commands[command_scroll_pos];
			caret_pos = commands[0].length();
		} else {
			commands[0][0] = '\0';
			caret_pos = 0;
		}
	}
}

void
console_t::char_typed(char c)
{
	if (c == '\t')
		c = ' ';
	if (c == '\n' || c == '\r') {
		if (commands[0].length()) {
			for (int i = CONSOLE_COMMAND_COUNT - 2; i >= 0; --i)
				commands[i + 1] = commands[i];
			commands[0][0] = '\0';
			caret_pos = 0;
			command_scroll_pos = 0;
			console.printf(":> %s\n", commands[1].str());
			exec.exec_script(commands[1], commands[1].length());
		} else {
			console.print(":>\n");
		}
	} else {
		if (insert) {
			commands[0].insert(c, caret_pos);
		} else {
			if (commands[0][caret_pos] == '\0')
				commands[0][caret_pos + 1] = '\0';
			commands[0][caret_pos] = c;
		}
		caret_pos = u_min(caret_pos + 1, commands[0].length());
	}
}

void
console_t::delete_left()
{
	if (caret_pos >= 1) {
		commands[0].remove(caret_pos - 1);
		--caret_pos;
	}
}

void
console_t::delete_right()
{
	commands[0].remove(caret_pos);
}

void
console_t::toggle_insert()
{
	insert = !insert;
}

console_t::~console_t()
	// Destructor, close the logfile if its open
{
	if (fp != NULL) {
//		fputc(EOF, fp);
		fclose(fp);
	}
}

void
console_t::print(const char* msg) 
	// Print text to the console, at least CONSOLE_MAX_LINE_LENGTH bytes can
	// be pasted at any time, if more than that is pasted then it may get
	// truncated
{
	int count = u_strncpy(text + pos, msg, CONSOLE_BUFFER_SIZE - pos);
	fwrite(text + pos, sizeof(char), count, fp);
	pos += count;
	update_lines();
};

void
console_t::printf(const char* format, ...)
	// Write formatted text to the console, it is guranteed that at least
	// CONSOLE_MAX_LINE_LENGTH chars can be written without worry in any
	// single printf, any beyond that may be truncated
{
	va_list argptr;
	va_start(argptr, format);
//	int count = u_vsnprintf(text + pos, CONSOLE_BUFFER_SIZE - pos, format, argptr);
	vprintf(format, argptr);
	va_end(argptr);
//	fwrite(text + pos, sizeof(char), count, fp);
//	pos += count;
//	update_lines();
}

void
console_t::vprintf(const char* format, va_list val)
	// Write formatted text to the console, it is guranteed that at least
	// CONSOLE_MAX_LINE_LENGTH chars can be written without worry in any
	// single printf, any beyond that may be truncated
{
	int count = u_vsnprintf(text + pos, CONSOLE_BUFFER_SIZE - pos, format, val);
	fwrite(text + pos, sizeof(char), count, fp);
	pos += count;
	update_lines();
}

void
console_t::warn(const char* msg)
	// Print a message if warnings are enabled
{
	if (show_warnings) 
		print(msg);
}

void
console_t::warnf(const char* format, ...)
	// Print a formatted message if warnings are enabled
{
	if (show_warnings) {
		va_list argptr;
		va_start(argptr, format);
		vprintf(format, argptr);
		va_end(argptr);
	}
}

void
console_t::dev(const char* msg)
	// Print a message if developer messages are enabled
{
	if (show_developer)
		print(msg);
}

void
console_t::devf(const char* format, ...)
	// Print a formatted message if developer messages are enabled
{
	if (show_developer) {
		va_list argptr;
		va_start(argptr, format);
		vprintf(format, argptr);
		va_end(argptr);
	}
}

void
console_t::update_lines()
	// Update the line pointers, Each line pointer points to a null terminated
	// string with no newlines. It is guranteed that after calling this method
	// at least CONSOLE_MAX_LINE_LENGTH bytes can be appended at text[pos] without
	// running to the end of the text buffer
{
	for (char* ptr = lines[headline]; *ptr != '\0'; ptr++) {
		if (*ptr == '\n') {
			headline = next_line(headline);
			if (headline == tailline)
				tailline = next_line(tailline);
			lines[headline] = ptr + 1;
			*ptr = '\0';
			times[2] = times[1];
			times[1] = times[0];
			times[0] = timer.time(TID_APP);
			if (console_scroll_pos != 0)
				++console_scroll_pos;
		}
	}
	
	// If reaching the end of the buffer go back to the start, copy whatever
	// text exists at the start of the current line to the start of the buffer
	if (ptr + CONSOLE_MAX_LINE_LENGTH > text + CONSOLE_BUFFER_SIZE) {
		pos = u_strncpy(text, lines[headline], (ptr - text) + CONSOLE_BUFFER_SIZE);
		lines[headline] = text;
		// Now back up tailline to have it just after headline
		while (lines[tailline] > lines[prev_line(tailline)])
			tailline = prev_line(tailline);
	}
	// Move tailline so that any overwritten text is no longer available
	while (lines[tailline] < lines[headline])
		tailline = next_line(tailline);
#ifdef _DEBUG
	fflush(fp);
#endif
}

const char*
console_t::get_line(int line)
	// Return a specific line of text, if line is positive then the lines will
	// be returned in order, if line is negative then the line that many lines
	// from the end will be returned, line -1 being the last line and so on
{
	int total_lines = headline + CONSOLE_LINES - tailline;
	int the_line;

	if (line >= 0) {
		if (line > total_lines)
			return "NO SUCH LINE YET";
		the_line = tailline + line + 1;
		if (the_line >= CONSOLE_LINES)
			the_line -= CONSOLE_LINES;
	} else {
		if (-line == total_lines)
			return DIVIDER;
		else if (-line > total_lines)
			return "";
		the_line = headline + line;
		if (the_line < 0)
			the_line += CONSOLE_LINES;
	}
	return lines[the_line] == NULL ? "console_t::get_line() error" : lines[the_line];
}

void
console_t::tesselate(display_list_t& dl) 
	// Add all the faces nescessary to render the console to the display list
{
	// Update position if nescessary
	if (showing) {
		visibility += timer.elapsed(TID_APP);
		if (visibility >= *console_max_visibility)
			visibility = *console_max_visibility;
	} else {
		visibility -= timer.elapsed(TID_APP);
		if (visibility <= 0.0f)
			visibility = 0.0f;
	}
	if (visibility != 0.0f) {
		// Add the background
		face_t* face = dl.get_face(shader, 0, 4, 6);
		if (face == 0) {
			console.print("Unable to allocate face for console background\n");
			return;
		}
		face->verts[0].pos.set(0.0f, UI_HEIGHT, -1.0f);
		face->verts[0].tc0.set(0.0f, 0.0f);
		face->verts[0].diffuse = color_t::identity;
		face->verts[1].pos.set(UI_WIDTH, UI_HEIGHT, -1.0f);
		face->verts[1].tc0.set(1.0f, 0.0f);
		face->verts[1].diffuse = color_t::identity;
		face->verts[2].pos.set(0.0f, UI_HEIGHT - UI_HEIGHT * visibility, -1.0f);
		face->verts[2].tc0.set(0.0f, visibility);
		face->verts[2].diffuse = color_t::identity;
		face->verts[3].pos.set(UI_WIDTH, UI_HEIGHT - UI_HEIGHT * visibility, -1.0f);
		face->verts[3].tc0.set(1.0f, visibility);
		face->verts[3].diffuse = color_t::identity;
		face->inds[0] = 0;
		face->inds[1] = 1;
		face->inds[2] = 2;
		face->inds[3] = 2;
		face->inds[4] = 1;
		face->inds[5] = 3;
	}

	// Now draw the visible lines of text
	int minlines = 0;
	if (times[2] + CONSOLE_SCROLL_TIME >= timer.time(TID_APP))
		minlines = 3;
	else if (times[1] + CONSOLE_SCROLL_TIME >= timer.time(TID_APP))
		minlines = 2;
	else if (times[0] + CONSOLE_SCROLL_TIME >= timer.time(TID_APP))
		minlines = 1;

	float text_y = u_min(60.0f * (1.0f - visibility), 60.0f - m_itof(minlines)) + 1.0f;

	font.set_color(color_t::white);
	if (text_y <= 61.0f) {
		if (showing) {
			font.write_text(dl, 0.5f, text_y, ":> ");
			font.write_text(dl, 3.5f, text_y, commands[0]);
			if (*console_caret_flash_time == 0.0f || m_ftoi(timer.time(TID_APP) / *console_caret_flash_time) & 0x1)
				font.write_text(dl, 3.5f + m_itof(caret_pos), text_y, insert ? "_" : "\b");
			text_y += 1.0f;
		}

		if (console_scroll_pos != 0) {
			char buffer[81];
			int count = u_snprintf(buffer, 80, "--- %d --------------------------------------------------------------------------", console_scroll_pos);
			font.write_text(dl, 0.5f, text_y, buffer);
			text_y += 1.0f;
		}
		int line = ~console_scroll_pos;
		while (text_y <= 61.0f) {
			font.write_text(dl, 0.5f, text_y, get_line(line--));
			text_y += 1.0f;
		}
	}
}
