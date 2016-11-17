//-----------------------------------------------------------------------------
// File: input.h
//
// Input handler
//-----------------------------------------------------------------------------

#ifndef INPUT_H
#define INPUT_H

#include "maths.h"
#include "win.h"	// key codes

class keyboard_t {
	// Recieves and handles all keyboard input
public:
	void key_down(int key);
	void key_up(int key);
	void char_typed(int c);

	bool is_key_down(int key) const { return keys[key]; };

	static keyboard_t& get_instance();

private:
	enum { NUM_KEYS = 256 };

	keyboard_t();

	bool keys[NUM_KEYS];
};

class mouse_t {
	// Recieves and handles all mouse input
public:
	enum button_t {
		BUTTON1,
		BUTTON2,
		BUTTON3
	};

	void button_down(button_t button);
	void button_up(button_t button);

	void move(int x, int y);
	const vec2_t& change() { return movement; }

	void reset() { movement = vec2_t::origin; };

	static mouse_t& get_instance();

private:
	vec2_t movement;

	enum { NUM_BUTTONS = 3 };

	mouse_t();

	bool buttons[NUM_BUTTONS];
};

#define keyboard	(keyboard_t::get_instance())
#define mouse		(mouse_t::get_instance())

#endif
