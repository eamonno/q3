//-----------------------------------------------------------------------------
// File: display.h
//
// Display list
//-----------------------------------------------------------------------------

#ifndef DISPLAY_LIST_H
#define DISPLAY_LIST_H

#include "maths.h"

#define MAX_VERTICES	65000
#define MAX_INDICES		65000
#define MAX_FACES		20000

struct vertex_t {
	vec3_t pos;
	vec3_t normal;
	color_t diffuse;
	vec2_t tc0;
	vec2_t tc1;
};

typedef ushort index_t;

struct face_t {
	hshader_t shader;
	htexture_t lightmap;
	vertex_t* verts;
	index_t* inds;
	int num_verts;
	int num_inds;
	int base_vert;
	int base_ind;
};

class display_list_t {
	// A display list, all the visible objects get tesselated into this list
	// each frame, the list is then sent to the renderer to be shown on screen
public:
	display_list_t() : next_vertex(verts), next_index(inds), next_face(faces) {}

	void clear();
	face_t* get_face(hshader_t shader, htexture_t lightmap, int num_verts, int num_inds = 0);

	// Return the beginning of the various buffers
	face_t* face_buffer()		{ return faces; }
	vertex_t* vertex_buffer()	{ return verts; }
	index_t* index_buffer()		{ return inds; }

	// Return the end of the various buffers, the value at this point is NOT
	// a valid whatever. Iteration over all the faces would take the form:
	// for (face_t* f = face_buffer(); f != face_buffer_end(); f++);
	// and so on for the other buffers
	const face_t* face_buffer_end() const { return next_face; }
	const vertex_t* vertex_buffer_end() const { return next_vertex; }
	const index_t* index_buffer_end() const { return next_index; }

	// Free the specified amount of verts or inds from the end of the previous
	// face allocated, the face will be modified by this call. Use if it turns
	// out that less vertices or indices than requested are required, The grow
	// methods can be used if you end up needing more vertices as you go. They
	// both return a pointer to the first of the newly added whatevers
	vertex_t* grow_verts(int count);
	index_t* grow_inds(int count);
	void shrink_verts(int count) { next_vertex -= count; (next_face - 1)->num_verts -= count; }
	void shrink_inds(int count) { next_index -= count; (next_face - 1)->num_inds -= count; }

	int num_faces() const { return next_face - faces; }
	int num_verts() const { return next_vertex - verts; }
	int num_inds()  const { return next_index - inds; }

private:
#pragma pack (push, 16)
	vertex_t verts[MAX_VERTICES];
	index_t inds[MAX_INDICES];
	face_t faces[MAX_FACES];
#pragma pack (pop)

	vertex_t* max_vertex() { return verts + MAX_VERTICES; }
	index_t* max_index() { return inds + MAX_INDICES; }
	face_t* max_face() { return faces + MAX_FACES; }

	vertex_t* next_vertex;
	index_t* next_index;
	face_t* next_face;
};

#endif
