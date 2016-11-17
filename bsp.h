//-----------------------------------------------------------------------------
// File: bsp.h
//
// Binary space partition tree
//-----------------------------------------------------------------------------

#ifndef BSP_H
#define BSP_H

#include "displaylist.h"
#include "maths.h"
#include "str.h"

class bsp_t {
	// Binary space partition tree
public:
	typedef int leafface_t;
	typedef int leafbrush_t;
	typedef index_t meshvert_t;

	struct lightmap_t {
	public:
		enum { WIDTH = 128 };
		enum { HEIGHT = 128 };
		enum { CHANNELS = 3 };
		
		lightmap_t() : handle(0) {}

		ubyte data[HEIGHT * WIDTH * CHANNELS];
		htexture_t handle;
	};

	class texture_t {
	public:
		texture_t() : shader(0) {}

		str_t<64> name;		// Shader name
		int flags;			// Surface Flags
		int contents;		// Content flags
		hshader_t shader;	// Shader handle
		uint shader_flags;	// Shader flags
	};

	class node_t {
	public:
		int plane;			// Plane index
		int children[2];	// Child nodes, negative is a leaf (-(leaf + 1))
		bbox_t bbox;		// Bounding box of the node
		bsphere_t bsphere;	// node bounds
	};

	class leaf_t {
	public:
		int cluster;		// Visdata cluster index
		int area;			// Areaportal area
		bbox_t bbox;		// Leaf bounding box
		bsphere_t bsphere;	// bounding sphere
		int leafface;		// First leafface
		int num_leaffaces;	// Number of leaffaces
		int leafbrush;		// First leafbrush
		int num_leafbrushes;// Number of leafbrushes
	};

	class model_t {
	public:
		bbox_t bbox;		// Bounding box of the model
		bsphere_t bsphere;	// bounding sphere
		int face;			// First face for the model
		int num_faces;		// Number of faces for the model
		int brush;			// First brush for the model
		int num_brushes;	// Number of brushes for the model
	};

	struct lightvol_t {
		ubyte ambient[3];
		ubyte directional[3];
		ubyte dir[2];
	};

	class brush_t {
	public:
		int brushside;		// First brushside
		int num_brushsides;	// Number of brush sides
		int texture;		// texture index
	};

	class brushside_t {
	public:
		int plane;			// Plane index
		int texture;		// Texture index
	};

	class face_t {
	public:
		int texture;		// Texture index
		int effect;			// Effects index
		int type;			// Face Type
		int vertex;			// Index of first vertex
		int num_vertices;	// Number of vertices
		int meshvert;		// Index of first meshvert
		int num_meshverts;	// Number of meshverts
		int lightmap;		// Lightmap index
		vec2_t lm_start;	// Lightmap start
		vec2_t lm_size;		// Lightmap size
		vec3_t origin;		// Face origin
		bbox_t bbox;		// Face bounds
		vec3_t normal;		// Surface normal
		int	 patch_size_x;	// Patch x size;
		int	 patch_size_y;	// Patch y size;
		bool drawn;
		float distance;		// distance to face
//		uint flags;
	};

	bsp_t() :
		num_textures(0),
		num_planes(0), 
		num_nodes(0), 
		num_leaves(0),
		num_leaffaces(0), 
		num_leafbrushes(0),
		num_models(0),
		num_brushes(0), 
		num_brushsides(0),
		num_vertices(0),
		num_meshverts(0),
		num_faces(0),
		num_lightmaps(0),
		num_lightvols(0),
		num_visvecs(0),
		load_lightmap(0),
		visvec_size(0),
		textures(0),
		planes(0),
		nodes(0),
		leaves(0),
		leaffaces(0),
		leafbrushes(0), 
		models(0),
		brushes(0),
		brushsides(0),
		vertices(0),
		meshverts(0),
		faces(0),
		lightmaps(0),
		lightvols(0),
		visdata(0)
	{}
	~bsp_t() { destroy(); }

	int		resources_to_load();
	void	load_resource();
	void	free_resources();

	void	destroy();
	void	tesselate(display_list_t& dl, const vec3_t& eye, const frustum_t& frustum);

	// Load the various parts of the bsp
//	bool load_entities(const void* data, uint length);
	bool load_textures(const void* data, uint length);
	bool load_planes(const void* data, uint length);
	bool load_nodes(const void* data, uint length);
	bool load_leaves(const void* data, uint length);
	bool load_leaffaces(const void* data, uint length);
	bool load_leafbrushes(const void* data, uint length);
	bool load_models(const void* data, uint length);
	bool load_brushes(const void* data, uint length);
	bool load_brushsides(const void* data, uint length);
	bool load_vertices(const void* data, uint length);
	bool load_meshverts(const void* data, uint length);
//	bool load_effects(const void* data, uint length);
	bool load_faces(const void* data, uint length);
	bool load_lightmaps(const void* data, uint length);
	bool load_lightvols(const void* data, uint length);
	bool load_visdata(const void* data, uint length);

//private:

	int num_textures;
	int num_planes;
	int num_nodes;
	int num_leaves;
	int num_leaffaces;
	int num_leafbrushes;
	int num_models;
	int num_brushes;
	int num_brushsides;
	int num_vertices;
	int num_meshverts;
	int num_faces;
	int num_lightmaps;
	int num_lightvols;
	int num_visvecs;
	int visvec_size;

	int load_lightmap;

	lightvol_t* lightvols;
	lightmap_t* lightmaps;
	texture_t* textures;
	plane_t* planes;
	node_t* nodes;
	leaf_t* leaves;
	leafface_t* leaffaces;
	leafbrush_t* leafbrushes;
	model_t* models;
	brush_t* brushes;
	brushside_t* brushsides;
	vertex_t* vertices;
	meshvert_t* meshverts;
	face_t* faces;
	ubyte* visdata;

	void walk_tree(int index);
	
	bool check_vis(int from_cluster, int to_cluster) {
		return (visdata[from_cluster * visvec_size + (to_cluster >> 3)] >> (to_cluster & 0x7)) & 0x1;
	}
};

#endif
