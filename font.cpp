//-----------------------------------------------------------------------------
// File: font.cpp
//
// Implementation of the font class
//-----------------------------------------------------------------------------

#include "font.h"
#include "util.h"
#include "maths.h"
#include "console.h"
#include <memory>

#include "mem.h"
#define new mem_new

/*
#define EO_READ_ONLY	0x1
#define EO_TRANSIENT	0x2
#define EO_PARENT		0x4

class base_exec_object_t {
	// Exec object, these are added to exec to make them available through the console
public:
	virtual const char* get_name();
	virtual const char* get_type();
	virtual const char* get_field(char* name);
	virtual const char* set_field(char* name);

template <class T>
class exec_object {
	exec_object
	// An object which can be added to the exec for access
	int get_field_count
};
*/

font_t&
font_t::get_instance()
{
	static std::auto_ptr<font_t> instance(new font_t());
	return *instance;
}

static float tab_size = 8.0f;

void 
font_t::write_text(display_list_t& dl, float xpos, float ypos, const char* text) const
	// Add a face to the display list to render the given text on a ui sized screen
	// Any chars that would be off the bounds of a ui sized screen are discarded, this
	// may mean that dl comes back unchanged return and newline chars are treated just
	// like any other chars, spaces and tabs will not be displayed but an appropriately
	// sized gap will be left, If shader is not set then no text will be added to the 
	// display list
{
	int len = u_strlen(text);

	if (len == 0 || shader == 0 || ypos - cheight > UI_HEIGHT || ypos < 0.0f)
		return;

	face_t* face = dl.get_face(shader, 0, len * 4, len * 6);
	if (face == 0) {
		console.print("Unable to allocate face for font\n");
		return;
	}
	vertex_t* vert = face->verts;
	index_t* ind = face->inds;

	float vleft = xpos * cwidth;
	float vright;
	float vtop = ypos * cheight;
	float vbottom = vtop - cheight;

	int nchars = 0;

	for (int i = 0; i < len; i++) {
		if (vleft > UI_WIDTH) {
			break;
		} else if (text[i] == ' ') {
			vleft += cwidth;
			continue;
		} else if (text[i] == '\t') {
			vleft = m_floor((((vleft - xpos) / cwidth) + tab_size) / tab_size) * cwidth * tab_size + xpos;
			continue;
		}
		vright = vleft + cwidth;
		// Work out the texture coordinates
		float tleft = m_itof(text[i] & 0xf) * 0.0625f;
		float tright = tleft + 0.0625f;
		float ttop = m_itof(text[i] / 16) * 0.0625f;
		float tbottom = ttop + 0.0625f;
		// Fill in the four vertices for this char
		vert[0].pos.set(vleft, vtop, 0.0f);
		vert[0].tc0.set(tleft, ttop);
		vert[0].diffuse = color;
		vert[1].pos.set(vright, vtop, 0.0f);
		vert[1].tc0.set(tright, ttop);
		vert[1].diffuse = color;
		vert[2].pos.set(vleft, vbottom, 0.0f);
		vert[2].tc0.set(tleft, tbottom);
		vert[2].diffuse = color;
		vert[3].pos.set(vright, vbottom, 0.0f);
		vert[3].tc0.set(tright, tbottom);
		vert[3].diffuse = color;
		vert += 4;
		// Fill in the siz indices for this char
		ind[0] = 4 * nchars;
		ind[1] = 4 * nchars + 1;
		ind[2] = 4 * nchars + 2;
		ind[3] = 4 * nchars + 2;
		ind[4] = 4 * nchars + 1;
		ind[5] = 4 * nchars + 3;
		ind += 6;
		nchars++;
		vleft = vright;
	}
	if (nchars < len) {
		// Return unused buffer space
		dl.shrink_verts((len - nchars) * 4);
		dl.shrink_inds((len - nchars) * 6);
	}
}
