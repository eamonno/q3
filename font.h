//-----------------------------------------------------------------------------
// File: font.h
//
// Display fonts, for writing to the screen
//-----------------------------------------------------------------------------

#ifndef FONT_H
#define FONT_H

#include "displaylist.h"
#include "maths.h"
#include "ui.h"

class font_t {
	// A single display font for writing chars to the screen, assumes the font
	// has 256 chars in 16 rows of 16 chars, ascii ordered
public:
	// Set the shader to draw the text with
	void set_shader(hshader_t s) { shader = s; }
	// Set the color of printed text
	void set_color(color_t col) { color = col; }
	// Set the number of characters per screen
	void set_resolution(float x, float y) { cwidth = UI_WIDTH / x; cheight = UI_HEIGHT / y; }
	// Append the nescessary faces to render the required text
	void write_text(display_list_t& dl, float xpos, float ypos, const char* text) const;
	// Get the current text color
	color_t get_color() const { return color; }
	// Get the current text resolution
	vec2_t get_resolution() const { return vec2_t(UI_WIDTH / cwidth, UI_HEIGHT / cheight); }

	static font_t& get_instance();

private:
	font_t() : color(color_t::white), shader(0) { set_resolution(100.0f, 40.0f); }

	hshader_t shader;
	color_t color;
	float cwidth;
	float cheight;
};

#define font (font_t::get_instance())

#endif