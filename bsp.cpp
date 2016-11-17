//-----------------------------------------------------------------------------
// File: bsp.cpp
//
// Binary space partition tree
//-----------------------------------------------------------------------------

#include "bsp.h"
#include "util.h"
#include "d3d.h"
#include "console.h"
#include "exec.h"
#include "pak.h"
#include <memory>

#include "mem.h"
#define new mem_new

using std::auto_ptr;

namespace {

	const int FACE_TYPE_POLY = 1;
	const int FACE_TYPE_PATCH = 2;
	const int FACE_TYPE_MESH = 3;
	const int FACE_TYPE_BILLBOARD = 4;

	// 2d vector of ints stored in bspfile
	struct bspivec2_t {
		int x;
		int y;
		operator vec2_t() const { return vec2_t(m_itof(x), m_itof(y)); }
	};

	// 3d vector of floats in x, z, y order as stored in bspfile
	struct bspvec3_t {
		float x;
		float z;
		float y;
		operator vec3_t() const { return vec3_t(x, y, -z); }
	};

	// 3d vector of floats in x, z, y order as stored in bspfile
	struct bspivec3_t {
		int x;
		int z;
		int y;
		operator vec3_t() const { return vec3_t(m_itof(x), m_itof(y), m_itof(-z)); }
	};

	// Bounding box composed of 2 floating point x, z, y vectors as stored in bspfile
	struct bspbbox_t {
		bspvec3_t mins;
		bspvec3_t maxs;
		operator bbox_t() const { return bbox_t(mins, maxs); }
	};

	// Bounding box composed of 2 integer x, z, y vectors as stored in bspfile
	struct bspibbox_t {
		bspivec3_t mins;
		bspivec3_t maxs;
		operator bbox_t() const { return bbox_t(mins, maxs); }
	};
};

cvar_int_t freezepvs("freezepvs", 0, 0, 1);
cvar_int_t showbspmodels("showbspmodels", 1, 0, 1);
cvar_int_t showbspcurves("showbspcurves", 1, 0, 1);

int
bsp_t::resources_to_load()
{
	return num_lightmaps - load_lightmap;
}

void
bsp_t::load_resource()
{
	if (load_lightmap < num_lightmaps) {
		// Upload the next lightmap to the graphics card
		str_t<64> name = "lightmap";
		name += str_t<64>(load_lightmap);
		lightmaps[load_lightmap].handle = d3d.upload_lightmap_rgb(
			lightmaps[load_lightmap].data,
			lightmap_t::WIDTH,
			lightmap_t::HEIGHT,
			name
		);
		++load_lightmap;
	}
}

void
bsp_t::free_resources()
{
	destroy();

	load_lightmap = 0;	// Lightmaps from this index forward need to be loaded
}

void
bsp_t::destroy()
{
	num_lightmaps = 0;
	delete [] lightmaps;
	lightmaps = 0;

	num_lightvols = 0;
	delete [] lightvols;
	lightvols = 0;
	
	num_visvecs = 0;
	visvec_size = 0;
	delete [] visdata;
	visdata = 0;

	num_textures = 0;
	delete [] textures;
	textures = 0;

	num_planes = 0;
	delete [] planes;
	planes = 0;

	num_nodes = 0;
	delete [] nodes;
	nodes = 0;

	num_leaves = 0;
	delete [] leaves;
	leaves = 0;
	
	num_leaffaces = 0;
	delete [] leaffaces;
	leaffaces = 0;
	
	num_leafbrushes = 0;
	delete [] leafbrushes;
	leafbrushes = 0;
	
	num_models = 0;
	delete [] models;
	models = 0;
	
	num_brushes = 0;
	delete [] brushes;
	brushes = 0;
	
	num_brushsides = 0;
	delete [] brushsides;
	brushsides = 0;
	
	num_vertices = 0;
	delete [] vertices;
	vertices = 0;
	
	num_meshverts = 0;
	delete [] meshverts;
	meshverts = 0;
	
	num_faces = 0;
	delete [] faces;
	faces = 0;
}

frustum_t _frustum;
vec3_t _eye;
display_list_t* _dl;
int in_cluster;
int in_area;

void
bsp_t::tesselate(display_list_t &dl, const vec3_t& eye, const frustum_t& frustum)
	// Tesselate the world into dl
{

	if (!*freezepvs) {
		_frustum = frustum;
		_eye = eye;
		_dl = &dl;
	}
	
	for (int node = 0; node >= 0; ) {
		if (is_behind(eye, planes[nodes[node].plane])) {
			node = nodes[node].children[1];
		} else {	// Point is on or before the plane
			node = nodes[node].children[0];
		}
	}

	if (!*freezepvs) {
		in_cluster = leaves[~node].cluster;
		in_area = leaves[~node].area;
	}

	// Mark all faces as invisible
	for (int face = 0; face < num_faces; ++face)
		faces[face].drawn = false;

	walk_tree(0);

	if (*showbspmodels == 0)
		return;

	// Draw the visible models
	for (int model_num = 1; model_num < num_models; ++model_num) {
		const model_t& model = models[model_num];
		if (_frustum.intersect_sphere(model.bsphere)) {
			for (int modelface = model.face; modelface < model.face + model.num_faces; ++modelface) {
				face_t& face = faces[modelface];
				hshader_t shader = textures[face.texture].shader;
				htexture_t lightmap = face.lightmap == -1 ? 0 : lightmaps[face.lightmap].handle;

				if (face.drawn == false && (face.type == FACE_TYPE_POLY || face.type == FACE_TYPE_MESH)) {
					::face_t* dlface = _dl->get_face(shader, lightmap, 0, 0);
					if (dlface == 0) {
						console.print("Unable to allocate face for bsp model\n");
						return;
					}
					dlface->base_ind = face.meshvert;
					dlface->num_inds = face.num_meshverts;
					dlface->base_vert = face.vertex;
					dlface->num_verts = face.num_vertices;
					face.drawn = true;
				} else if (face.drawn == false && (face.type == FACE_TYPE_PATCH)) {
					::face_t* dlface = _dl->get_face(shader, lightmap, 0, (face.patch_size_x - 1) * (face.patch_size_y - 1) * 6);
					if (dlface == 0) {
						console.print("Unable to allocate face for bsp model patch\n");
						return;
					}
					dlface->base_vert = face.vertex; 
					dlface->num_verts = face.num_vertices;
					index_t* ind = dlface->inds;
					int w = face.patch_size_x;
					for (int x = 0; x < face.patch_size_y - 1; ++x)
						for (int y = 0; y < w - 1; ++y) {
							*ind++ = x * w + y;
							*ind++ = (x + 1) * w + y;
							*ind++ = x * w + y + 1;
							*ind++ = x * w + y + 1;
							*ind++ = (x + 1) * w + y;
							*ind++ = (x + 1) * w + y + 1;
						}
					face.drawn = true;
				}
			}
		}
	}
}

void
bsp_t::walk_tree(int index)
{
	if (index >= 0) {
		// This is a node
		const node_t& node = nodes[index];
		if (!_frustum.intersect_sphere(node.bsphere))
			return;
		if (is_behind(_eye, planes[node.plane])) {
			// back node then front node
			walk_tree(node.children[1]);
			walk_tree(node.children[0]);
		} else {
			// front node then back node
			walk_tree(node.children[0]);
			walk_tree(node.children[1]);
		}
	} else {
		// This is a leaf
		const leaf_t& leaf = leaves[~index];	//~index = -(index + 1)
		if (!_frustum.intersect_sphere(leaf.bsphere) || leaf.area != in_area)
			return;
		if (in_cluster < 0 || check_vis(in_cluster, leaf.cluster)) {
			int last_leafface = leaf.leafface + leaf.num_leaffaces;
			for (int leafface = leaf.leafface; leafface < last_leafface; ++leafface) {
				face_t& face = faces[leaffaces[leafface]];
				hshader_t shader = textures[face.texture].shader;
				htexture_t lightmap = face.lightmap == -1 ? 0 : lightmaps[face.lightmap].handle;

				if (face.drawn == false && (face.type == FACE_TYPE_POLY || face.type == FACE_TYPE_MESH)) {
					if (face.type == FACE_TYPE_POLY) {
						if (textures[face.texture].shader_flags & (SF_CULLBACK | SF_CULLFRONT)) {
							if (dot(_eye, face.normal) < face.distance) {
								if (textures[face.texture].shader_flags & SF_CULLBACK) {
									face.drawn = true;
									continue;
								}
							} else if (textures[face.texture].shader_flags & SF_CULLFRONT) {
								face.drawn = true;
								continue;
							}
						}
					}
					::face_t* dlface = _dl->get_face(shader, lightmap, 0, 0);
					if (dlface == 0) {
						console.print("Unable to allocate face for bsp leaf\n");
						return;
					}
					dlface->base_ind = face.meshvert;
					dlface->num_inds = face.num_meshverts;
					dlface->base_vert = face.vertex;
					dlface->num_verts = face.num_vertices;
					face.drawn = true;
				} else if (face.drawn == false && (face.type == FACE_TYPE_PATCH)) {
					if (*showbspcurves == 0)
						continue;
					::face_t* dlface = _dl->get_face(shader, lightmap, 0, (face.patch_size_x - 1) * (face.patch_size_y - 1) * 6);
					if (dlface == 0) {
						console.print("Unable to allocate face for bsp patch\n");
						return;
					}
					dlface->base_vert = face.vertex; 
					dlface->num_verts = face.num_vertices;
					index_t* ind = dlface->inds;
					int w = face.patch_size_x;
					for (int x = 0; x < face.patch_size_y - 1; ++x)
						for (int y = 0; y < w - 1; ++y) {
							*ind++ = x * w + y;
							*ind++ = (x + 1) * w + y;
							*ind++ = x * w + y + 1;
							*ind++ = x * w + y + 1;
							*ind++ = (x + 1) * w + y;
							*ind++ = (x + 1) * w + y + 1;
						}
					face.drawn = true;
				}
			}
		}
	}
}

bool
bsp_t::load_textures(const void* data, uint length)
	// Retrieve the textures information from the textures block of the file
{
	#pragma pack (push, 1)
	// Texture information is stored in this format in the bspfile
	struct bsptexture_t {
		char name[64];
		int flags;
		int contents;
	};
	#pragma pack (pop)

	num_textures = length / sizeof(bsptexture_t);
	if (num_textures) {
		textures = new texture_t[num_textures];
		const bsptexture_t* bsptextures = static_cast<const bsptexture_t*>(data);
		for (int i = 0; i < num_textures; ++i) {
			textures[i].name = bsptextures[i].name;
			textures[i].flags = bsptextures[i].flags;
			textures[i].contents = bsptextures[i].contents;
			textures[i].shader = d3d.get_shader(textures[i].name);
			textures[i].shader_flags = d3d.get_surface_flags(textures[i].shader);
		}
	}
	return true;
}

bool
bsp_t::load_planes(const void* data, uint length)
	// Retrieve the planes information from the planes lump
{
	#pragma pack (push, 1)
	// A plane as represented in the bsp file
	struct bspplane_t {
		bspvec3_t normal;	// Plane normal
		float distance;		// Distance from the origin
	};
	#pragma pack (pop)

	num_planes = length / sizeof(bspplane_t);
	planes = new plane_t[num_planes];
	const bspplane_t* bspplanes = static_cast<const bspplane_t*>(data);
	for (int i = 0; i < num_planes; ++i) {
		planes[i].normal = bspplanes[i].normal;
		planes[i].distance = bspplanes[i].distance;
	}
	return true;
}

bool
bsp_t::load_nodes(const void* data, uint length)
	// Retrieve the nodes information from the nodes lump
{
	#pragma pack (push, 1)
	// A node as stored in the bsp file
	struct bspnode_t {
		int plane;
		int children[2];
		bspibbox_t bbox;
	};
	#pragma pack (pop)

	num_nodes = length / sizeof(bspnode_t);
	nodes = new node_t[num_nodes];
	const bspnode_t* bspnodes = static_cast<const bspnode_t*>(data);
	for (int i = 0; i < num_nodes; ++i) {
		nodes[i].plane = bspnodes[i].plane;
		nodes[i].children[0] = bspnodes[i].children[0];
		nodes[i].children[1] = bspnodes[i].children[1];
		nodes[i].bbox = bspnodes[i].bbox;
		nodes[i].bsphere.midpoint = nodes[i].bbox.midpoint();
		nodes[i].bsphere.radius = nodes[i].bbox.diagonal_length() * 0.5f;
	}
	return true;
}

bool
bsp_t::load_leaves(const void* data, uint length)
	// Retrieve the leaves information from the leaves lump
{
	#pragma pack (push, 1)
	// Leaf
	struct bspleaf_t {
		int cluster;		// Visdata cluster index
		int area;			// Areaportal area
		bspibbox_t bbox;	// Leaf bounding box
		int leafface;		// First leafface
		int num_leaffaces;	// Number of leaffaces
		int leafbrush;		// First leafbrush
		int num_leafbrushes;// Number of leafbrushes
	};
	#pragma pack (pop)

	// Get the leaf information
	num_leaves = length / sizeof(bspleaf_t);
	leaves = new leaf_t[num_leaves];
	const bspleaf_t* bspleaves = static_cast<const bspleaf_t*>(data);
	for (int i = 0; i < num_leaves; ++i) {
		leaves[i].cluster = bspleaves[i].cluster;
		leaves[i].area = bspleaves[i].area;
		leaves[i].bbox = bspleaves[i].bbox;
		leaves[i].bsphere.midpoint = leaves[i].bbox.midpoint();
		leaves[i].bsphere.radius = leaves[i].bbox.diagonal_length() * 0.5f;
		leaves[i].leafface = bspleaves[i].leafface;
		leaves[i].num_leaffaces = bspleaves[i].num_leaffaces;
		leaves[i].leafbrush = bspleaves[i].leafbrush;
		leaves[i].num_leafbrushes = bspleaves[i].num_leafbrushes;
	}
	return true;
}

bool
bsp_t::load_leaffaces(const void* data, uint length)
	// Retrieve the leaf face information from the leaffaces lump
{
	typedef int bspleafface_t;

	num_leaffaces = length / sizeof(bspleafface_t);
	leaffaces = new leafface_t[num_leaffaces];
	const bspleafface_t* bspleaffaces = static_cast<const bspleafface_t*>(data);
	for (int i = 0; i < num_leaffaces; ++i)
		leaffaces[i] = bspleaffaces[i];
	return true;
}

bool
bsp_t::load_leafbrushes(const void* data, uint length)
	// Retrieve the leaf brush information from the leaffaces lump
{
	typedef int bspleafbrush_t;

	num_leafbrushes = length / sizeof(bspleafbrush_t);
	leafbrushes = new leafbrush_t[num_leafbrushes];
	const bspleafbrush_t* bspleafbrushes = static_cast<const bspleafbrush_t*>(data);
	for (int i = 0; i < num_leafbrushes; ++i)
		leafbrushes[i] = bspleafbrushes[i];
	return true;
}

bool
bsp_t::load_models(const void* data, uint length)
	// Retrieve the models information from the models lump
{
	#pragma pack (push, 1)
	// Model as stored in the bsp file
	struct bspmodel_t {
		bspbbox_t bbox;
		int face;
		int num_faces;
		int brush;
		int num_brushes;
	};
	#pragma pack (pop)

	num_models = length / sizeof(bspmodel_t);
	models = new model_t[num_models];
	const bspmodel_t* bspmodels = static_cast<const bspmodel_t*>(data);
	for (int i = 0; i < num_models; ++i) {
		models[i].bbox = bspmodels[i].bbox;
		models[i].bsphere.midpoint = models[i].bbox.midpoint();
		models[i].bsphere.radius = models[i].bbox.diagonal_length() * 0.5f;
		models[i].face = bspmodels[i].face;
		models[i].num_faces = bspmodels[i].num_faces;
		models[i].brush = bspmodels[i].brush;
		models[i].num_brushes = bspmodels[i].num_brushes;
	}
	return true;
}

bool
bsp_t::load_brushes(const void* data, uint length)
	// Retrieve the brushes information from the brushes lump
{
	#pragma pack (push, 1)
	// Brush information
	struct bspbrush_t {
		int brushside;
		int num_brushsides;
		int texture;
	};
	#pragma pack (pop)

	num_brushes = length / sizeof(bspbrush_t);
	brushes = new brush_t[num_brushes];
	const bspbrush_t* bspbrushes = static_cast<const bspbrush_t*>(data);
	for (int i = 0; i < num_brushes; ++i) {
		brushes[i].brushside = bspbrushes[i].brushside;
		brushes[i].num_brushsides = bspbrushes[i].num_brushsides;
		brushes[i].texture = bspbrushes[i].texture;
	}
	return true;
}

bool
bsp_t::load_brushsides(const void* data, uint length)
	// Retrieve the brush sides information from the brush sides lump
{
	#pragma pack (push, 1)
	// Brush sides
	struct bspbrushside_t {
		int plane;
		int texture;
	};
	#pragma pack (pop)

	num_brushsides = length / sizeof(bspbrushside_t);
	brushsides = new brushside_t[num_brushsides];
	const bspbrushside_t* bspbrushsides = static_cast<const bspbrushside_t*>(data);
	for (int i = 0; i < num_brushsides; ++i) {
		brushsides[i].plane = bspbrushsides[i].plane;
		brushsides[i].texture = bspbrushsides[i].plane;
	}
	return true;
}

bool
bsp_t::load_vertices(const void* data, uint length)
	// Retrieve the vertex information from the vertices lump
{
	#pragma pack (push, 1)
	// Vertex information as stored in the bspfile
	struct bspvertex_t {
		bspvec3_t pos;
		vec2_t tc0;
		vec2_t tc1;
		bspvec3_t normal;
		color_t color;
	};
	#pragma pack (pop)
	
	num_vertices = length / sizeof(bspvertex_t);
	vertices = new vertex_t[num_vertices];
	const bspvertex_t* bspvertices = static_cast<const bspvertex_t*>(data);
	for (int i = 0; i < num_vertices; ++i) {
		vertices[i].pos = bspvertices[i].pos;
		vertices[i].tc0 = bspvertices[i].tc0;
		vertices[i].tc1 = bspvertices[i].tc1;
		vertices[i].normal = bspvertices[i].normal;
		vertices[i].diffuse = bspvertices[i].color;
	}
	return true;
}

bool
bsp_t::load_meshverts(const void* data, uint length)
	// Retrieve the meshverts info from the meshverts lump
{
	typedef int bspmeshvert_t;

	num_meshverts = length / sizeof(bspmeshvert_t);
	meshverts = new meshvert_t[num_meshverts];
	const bspmeshvert_t* bspmeshverts = static_cast<const bspmeshvert_t*>(data);
	for (int i = 0; i < num_meshverts; ++i)
		meshverts[i] = bspmeshverts[i];
	return true;
}

//bool
//bsp_t::load_effects(const void* data, uint length)
//{
//	return true;
//}

bool
bsp_t::load_faces(const void* data, uint length)
	// Retrieve the face information from the bsp file
{
	#pragma pack (push, 1)
	// Faces as stored in the bsp file
	struct bspface_t {
		int texture;
		int effect;
		int type;
		int vertex;
		int num_vertices;
		int meshvert;
		int num_meshverts;
		int lightmap;
		bspivec2_t lm_start;
		bspivec2_t lm_size;
		bspvec3_t origin;
		bspbbox_t bbox;
		bspvec3_t normal;
		int patch_size_x;
		int patch_size_y;
	};
	#pragma pack (pop)

	num_faces = length / sizeof(bspface_t);
	faces = new face_t[num_faces];
	const bspface_t* bspfaces = static_cast<const bspface_t*>(data);
	for (int i = 0; i < num_faces; ++i) {
		faces[i].texture = bspfaces[i].texture;
		faces[i].effect = bspfaces[i].effect;
		faces[i].type = bspfaces[i].type;
		faces[i].vertex = bspfaces[i].vertex;
		faces[i].num_vertices = bspfaces[i].num_vertices;
		faces[i].meshvert = bspfaces[i].meshvert;
		faces[i].num_meshverts = bspfaces[i].num_meshverts;
		faces[i].lightmap = bspfaces[i].lightmap;
		faces[i].lm_start = bspfaces[i].lm_start;
		faces[i].lm_size = bspfaces[i].lm_size;
		faces[i].origin = bspfaces[i].origin;
		faces[i].bbox = bspfaces[i].bbox;
		faces[i].normal = bspfaces[i].normal;
		faces[i].patch_size_x = bspfaces[i].patch_size_x;
		faces[i].patch_size_y = bspfaces[i].patch_size_y;
		faces[i].distance = dot(faces[i].normal, faces[i].origin);
	}
	return true;
}

bool
bsp_t::load_lightmaps(const void* data, uint length)
	// Retrieve the lightmap information from the bsp file
{
	#pragma pack (push, 1)
	// Format of lightmap as stored in the bspfile
	struct bsplightmap_t {
		ubyte data[128 * 128 * 3];
	};
	#pragma pack (pop)

	num_lightmaps = length / sizeof(bsplightmap_t);
	lightmaps = new lightmap_t[num_lightmaps];
	const bsplightmap_t* bsplightmaps = static_cast<const bsplightmap_t*>(data);
	for (int i = 0; i < num_lightmaps; ++i)
		u_memcpy(lightmaps[i].data, bsplightmaps[i].data, 128 * 128 * 3);
	return true;
}

bool
bsp_t::load_lightvols(const void* data, uint length)
	// Retreive the light volume data from the bsp file
{
	#pragma pack (push, 1)
	// Light volumes as stored in the bsp file
	struct bsplightvol_t {
		ubyte ambient[3];
		ubyte directional[3];
		ubyte dir[2];
	};
	#pragma pack (pop)

	num_lightvols = length / sizeof(bsplightvol_t);
	lightvols = new lightvol_t[num_lightvols];
	const bsplightvol_t* bsplightvols = static_cast<const bsplightvol_t*>(data);
	for (int i = 0; i < num_lightvols; ++i) {
		lightvols[i].ambient[0] = bsplightvols[i].ambient[0];
		lightvols[i].ambient[1] = bsplightvols[i].ambient[1];
		lightvols[i].ambient[2] = bsplightvols[i].ambient[2];
		lightvols[i].directional[0] = bsplightvols[i].directional[0];
		lightvols[i].directional[1] = bsplightvols[i].directional[1];
		lightvols[i].directional[2] = bsplightvols[i].directional[2];
		lightvols[i].dir[0] = bsplightvols[i].dir[0];
		lightvols[i].dir[1] = bsplightvols[i].dir[1];
	}
	return true;
}

bool
bsp_t::load_visdata(const void* data, uint length)
	// Retrieve the cluster to cluster visibility data from the bsp file
{
	#pragma pack (push, 1)
	// visdata as stored in the bsp file
	struct bspvisdata_t {
		int num_vecs;		// Number of vectors
		int vec_size;		// Size of the vectors
		ubyte data[1];		// vector data (num_vecs * vec_size bytes)
	};
	#pragma pack (pop)

	const bspvisdata_t* bspvisdata = static_cast<const bspvisdata_t*>(data);
	num_visvecs = bspvisdata->num_vecs;
	visvec_size = bspvisdata->vec_size;
	visdata = new ubyte[num_visvecs * visvec_size];
	for (int i = 0; i < num_visvecs * visvec_size; ++i)
		visdata[i] = bspvisdata->data[i];
	return true;
}
