//-----------------------------------------------------------------------------
// File: input.cpp
//
// User input managment
//-----------------------------------------------------------------------------

#include "input.h"
#include <memory>
#include "console.h"
#include "mem.h"

#define new mem_new

using std::auto_ptr;

#undef keyboard
#undef mouse

keyboard_t&
keyboard_t::get_instance()
{
	static auto_ptr<keyboard_t> keyboard(new keyboard_t());
	return *keyboard;
}

keyboard_t::keyboard_t()
	// Constructor, assume no keys are pressed
{
	for (int i = 0; i < NUM_KEYS; ++i)
		keys[i] = false;
}

void
keyboard_t::key_down(int key)
	// Called whenever a key is pressed
{
	keys[key] = true;
	if (console.is_showing()) {
		if (key == VK_UP)
			console.cursor_up();
		else if (key == VK_DOWN)
			console.cursor_down();
		else if (key == VK_LEFT)
			console.cursor_left();
		else if (key == VK_RIGHT)
			console.cursor_right();
		else if (key == VK_PRIOR)
			console.scroll_up();
		else if (key == VK_NEXT)
			console.scroll_down();
		else if (key == VK_BACK)
			console.delete_left();
		else if (key == VK_DELETE)
			console.delete_right();
		else if (key == VK_INSERT)
			console.toggle_insert();
	}
}

void
keyboard_t::key_up(int key)
	// Called whenever a key is released
{
	keys[key] = false;
}

void
keyboard_t::char_typed(int c)
	// Called whenever a char is typed
{
	if (console.is_showing())
		if (c != '\b')
			console.char_typed(c);
}



mouse_t&
mouse_t::get_instance()
{
	static auto_ptr<mouse_t> mouse(new mouse_t());
	return *mouse;
}

mouse_t::mouse_t() : movement(vec2_t::origin)
	// Constructor, assume no buttons are pressed
{
	for (int i = 0; i < NUM_BUTTONS; ++i)
		buttons[i] = false;
}

void
mouse_t::button_down(button_t button)
	// Called whenever a mouse button is pressed
{
	buttons[button] = true;
}

void
mouse_t::button_up(button_t button)
	// Called whenever a mouse button is released
{
	buttons[button] = false;
}

void
mouse_t::move(int x, int y)
	// Called whenever the mouse moves, distances are in pixels, positive
	// x means moveing left to right, positive y means moving upwards
{
	movement += vec2_t(m_itof(x), m_itof(y));
}
