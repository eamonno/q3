//-----------------------------------------------------------------------------
// File: displaylist.cpp
//
// Implementation for the display list
//-----------------------------------------------------------------------------

#include "displaylist.h"
#include "console.h"
#include "util.h"

#include "mem.h"
#define new mem_new

void
display_list_t::clear()
{
	int vused = next_vertex - verts;
	int iused = next_index - inds;
	int fused = next_face - faces;
	next_vertex = verts;
	next_index = inds;
	next_face = faces;
}

vertex_t*
display_list_t::grow_verts(int count)
	// Increase the size of the vertex buffer for the last face allocated
	// and return a pointer to the first of the newly allocated vertices
{
	if (next_vertex + count >= max_vertex())
		return 0;
	vertex_t* v = next_vertex;
	next_vertex += count;
	(next_face - 1)->num_verts += count;
	return v;
}

index_t*
display_list_t::grow_inds(int count)
	// Increase the size of the index buffer for the last face allocated
	// and return a pointer to the first of the newly allocated indices
{
	if (next_index + count >= max_index())
		return 0;
	index_t* i = next_index;
	next_index += count;
	(next_face - 1)->num_inds += count;
	return i;
}

face_t*
display_list_t::get_face(hshader_t shader, htexture_t lightmap, int num_verts, int num_inds) 
{
	if (next_vertex + num_verts >= max_vertex()) {
		console.warn("display_list_t::get_face(): vertex buffer full\n");
		return 0;
	}
	if (next_index + num_inds >= max_index()) {
		console.warn("display_list_t::get_face(): index buffer full\n");
		return 0;
	}
	if (next_face == max_face()) {
		console.warn("display_list_t::get_face(): face buffer full\n");
		return 0;
	}
	next_face->shader = shader;
	next_face->lightmap = lightmap;
	next_face->num_verts = num_verts;
	next_face->num_inds = num_inds;
	next_face->verts = num_verts ? next_vertex : 0;
	next_face->inds = num_inds ? next_index : 0;
	next_face->base_vert = next_vertex - verts;
	next_face->base_ind = next_index - inds;
	next_vertex += num_verts;
	next_index += num_inds;

	return next_face++;
}
