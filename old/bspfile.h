//-----------------------------------------------------------------------------
// File: bspfile.h
//
// A Quake 3 BSP file. Maps directly to the file structure used on disk
//-----------------------------------------------------------------------------

#ifndef BSPFILE_H
#define BSPFILE_H

#include "file.h"
#include "bsp.h"

class bspfile_t {
public:
	bspfile_t(file_t* f);
	~bspfile_t();

	bool is_valid();
	
	int num_textures();		// Get the number of textures in the bsp
	int num_planes();		// Get the number of planes in the bsp
	int num_nodes();		// Get the number of nodes in the bsp
	int num_leaves();		// Get the number of leaves in the bsp
	int num_leaffaces();	// Get the number of leaf faces in the bsp
	int num_leafbrushes();	// Get the number of leaf brushes in the bsp
	int num_lightmaps();	// Get the number of lightmaps in the bsp
	int num_models();		// Get the number of models in the world
	int num_brushes();		// Get the number of brushes in the world
	int num_brushsides();	// Get the number of brushsides
	int num_vertices();		// Get the number of vertices
	int num_meshverts();	// Get the number of meshverts
	int num_faces();		// Get the number of faces

	void get_textures(bsp_t::texture_t* dest);	// Copy the textures to dest
	void get_planes(plane_t* dest);				// Copy the planes to dest
	void get_nodes(bsp_t::node_t* dest);		// Copy the nodes to dest
	void get_leaves(bsp_t::leaf_t* dest);		// Copy the leaves to dest
	void get_leaffaces(bsp_t::leafface_t* dest);// Copy the leaf faces to dest
	void get_leafbrushes(bsp_t::leafbrush_t* dest);	// Copy the leaf brushes to dest
	void get_lightmaps(bsp_t::lightmap_t* dest);// Copy the lightmaps to dest
	void get_models(bsp_t::model_t* dest);		// Copy the models to dest
	void get_brushes(bsp_t::brush_t* dest);		// Copy the brushes to dest
	void get_brushsides(bsp_t::brushside_t* dest);	// Copy the brushsides to dest
	void get_vertices(vertex_t* dest);			// Copy the vertices to dest
	void get_meshverts(bsp_t::meshvert_t* dest);// Copy the meshverts to dest
	void get_faces(bsp_t::face_t* dest);		// Copy the faces to dest
	void get_visdata(bsp_t::visdata_t& data);	// Copy the visdata to dest

private:
	struct bspheader_t;

	file_t* file;
	bspheader_t* header;
};

#endif
