//-----------------------------------------------------------------------------
// File: bspfile.cpp
//
// Quake 3 .bsp file
//-----------------------------------------------------------------------------

#include "bspfile.h"

#include "displaylist.h"
#include "maths.h"
#include "types.h"
#include "util.h"

#include "mem.h"
#define new mem_new

#pragma pack (push, 1)

namespace {

//	typedef int bspleafface_t;
//	typedef int bspleafbrush_t;
	typedef int bspmeshvert_t;

//	// 2d vector of ints
//	struct bspivec2_t {
//		int x;
//		int y;
//		operator vec2_t() { return vec2_t(m_itof(x), m_itof(y)); }
//	};

//	// 3d vector of floats in x, z, y order
//	struct bspvec3_t {
//		float x;
//		float z;
//		float y;
//		operator vec3_t() { return vec3_t(x, y, -z); }
//	};

//	// 3d vector of floats in x, z, y order
//	struct bspivec3_t {
//		int x;
//		int z;
//		int y;
//		operator vec3_t() { return vec3_t(m_itof(x), m_itof(y), m_itof(-z)); }
//	};

	// Bounding box composed of 2 floating point x, z, y vectors
//	struct bspbbox_t {
//		bspvec3_t mins;
//		bspvec3_t maxs;
//		operator bbox_t() { return bbox_t(mins, maxs); }
//	};

//	// Bounding box composed of 2 integer x, z, y vectors
//	struct bspibbox_t {
//		bspivec3_t mins;
//		bspivec3_t maxs;
//		operator bbox_t() { return bbox_t(mins, maxs); }
//	};

//	// Texture information
//	struct bsptexture_t {
//		char name[64];
//		int flags;
//		int contents;
//
//		operator bsp_t::texture_t()
//		{
//			bsp_t::texture_t texture;
//			u_memcpy(&texture, this, sizeof(bsp_t::texture_t));
//			return texture;
//		}
//	};

//	// Models
//	struct bspmodel_t {
//		bspbbox_t bounds;
//		int face;
//		int num_faces;
//		int brush;
//		int num_brushes;
//
//		operator bsp_t::model_t()
//		{
//			bsp_t::model_t model;
//			model.bounds = bounds;
//			model.face = face;
//			model.num_faces = num_faces;
//			model.brush = brush;
//			model.num_brushes = num_brushes;
//			return model;
//		}
//	};

//	// Brush information
//	struct bspbrush_t {
//		int brushside;
//		int num_brushsides;
//		int texture;
//
//		operator bsp_t::brush_t()
//		{
//			bsp_t::brush_t brush;
//			brush.brushside = brushside;
//			brush.num_brushsides = num_brushsides;
//			brush.texture = texture;
//			return brush;
//		}
//	};

//	// Brush sides
//	struct bspbrushside_t {
//		int plane;
//		int texture;
//
//		operator bsp_t::brushside_t() 
//		{
//			bsp_t::brushside_t brushside;
//			brushside.plane = plane;
//			brushside.texture = texture;
//			return brushside;
//		};
//	};

//	// Vertex information as stored in the bspfile
//	struct bspvertex_t {
//		bspvec3_t pos;
//		vec2_t tcsurf;
//		vec2_t tclight;
//		bspvec3_t normal;
//		color_t color;
//
//		operator vertex_t()
//		{
//			vertex_t vertex;
//			vertex.pos = pos;
//			vertex.tc0 = tcsurf;
//			vertex.tc1 = tclight;
//			vertex.normal = normal;
//			vertex.diffuse = color;
//			return vertex;
//		};
//	};

	// Effect
	struct bspeffect_t {
		char name[64];
		int brush;
		int unknown;
	};

//	// Faces
//	struct bspface_t {
//		int texture;
//		int effect;
//		int type;
//		int vertex;
//		int num_vertices;
//		int meshvert;
//		int num_meshverts;
//		int lightmap;
//		bspivec2_t lm_start;
//		bspivec2_t lm_size;
//		bspvec3_t origin;
//		bspbbox_t bounds;
//		bspvec3_t normal;
//		bspivec2_t patch_size;
//
//		operator bsp_t::face_t()
//		{
//			bsp_t::face_t face;
//			face.texture = texture;
//			face.effect = effect;
//			face.type = type;
//			face.vertex = vertex;
//			face.num_vertices = num_vertices;
//			face.meshvert = meshvert;
//			face.num_meshverts = num_meshverts;
//			face.lightmap_index = lightmap;
//			face.lm_start = lm_start;
//			face.lm_size = lm_size;
//			face.origin = origin;
//			face.bounds = bounds;
//			face.normal = normal;
//			face.patch_size_x = patch_size.x;
//			face.patch_size_y = patch_size.y;
//			return face;
//		};
//	};

//	// Stored lightmap
//	struct bsplightmap_t {
//		ubyte map[128][128][3];
//
//		// Lots of redundant copying here, an operator = would be better
//		operator bsp_t::lightmap_t() {
//			bsp_t::lightmap_t lightmap;
//
//			u_memcpy(lightmap.data, map, sizeof(map));
//			return lightmap;
//		}
//	};

//	// Light volumes
//	struct bsplightvol_t {
//		ubyte ambient[3];
//		ubyte directional[3];
//		ubyte dir[2];
//	};

	// Visdata
	struct bspvisdata_t {
		int num_vecs;		// Number of vectors
		int vec_size;		// Size of the vectors
		ubyte data[1];		// vector data (num_vecs * vec_size bytes)
	};

//	// A plane as represented in the bsp file
//	struct bspplane_t {
//		bspvec3_t normal;	// Plane normal
//		float distance;		// Distance from the origin
//		operator plane_t() { return plane_t(normal, distance); }
//	};

//	// A node as stored in the bsp file
//	struct bspnode_t {
//		int plane;
//		int children[2];
//		bspibbox_t bounds;
//
//		operator bsp_t::node_t()
//		{
//			bsp_t::node_t node;
//			node.plane = plane;
//			node.children[0] = children[0];
//			node.children[1] = children[1];
//			node.bounds = bounds;
//			return node;
//		}
//	};

//	// Leaf
//	struct bspleaf_t {
//		int cluster;		// Visdata cluster index
//		int area;			// Areaportal area
//		bspibbox_t bounds;// Leaf bounding box
//		int leafface;		// First leafface
//		int num_leaffaces;	// Number of leaffaces
//		int leafbrush;		// First leafbrush
//		int num_leafbrushes;// Number of leafbrushes
//
//		operator bsp_t::leaf_t() 
//		{
//			bsp_t::leaf_t leaf;
//			leaf.cluster = cluster;
//			leaf.area = area;
//			leaf.bounds = bounds;
//			leaf.leafface = leafface;
//			leaf.num_leaffaces = num_leaffaces;
//			leaf.leafbrush = leafbrush;
//			leaf.num_leafbrushes = num_leafbrushes;
//			return leaf;
//		}
//	};

	// A single entry in the bsp file header
	struct bspdirentry_t {
		int offset;		// Offset from start of file to this lump
		int length;		// Size of the lump
	};
};

//// The file header
//struct bspfile_t::bspheader_t {
//	int signature;	// File signiture, should equal BSPFILE_MAGIC_NUMBER
//	int version;	// File version, should equal BSPFILE_VERSION
//	union {
//		struct {
//			bspdirentry_t de_entities;		// Entities lump
//			bspdirentry_t de_textures;		// Textures lump
//			bspdirentry_t de_planes;		// Planes lump
//			bspdirentry_t de_nodes;			// Nodes lump
//			bspdirentry_t de_leaves;		// Leaves lump
//			bspdirentry_t de_leaffaces;		// Leaf faces lump
//			bspdirentry_t de_leafbrushes;	// Leaf brushes lump
//			bspdirentry_t de_models;		// Models lump
//			bspdirentry_t de_brushes;		// Brush lump
//			bspdirentry_t de_brushsides;	// Brush sides lump
//			bspdirentry_t de_vertices;		// Vertices lump
//			bspdirentry_t de_meshverts;		// Meshvert lump
//			bspdirentry_t de_effects;		// Effects lump
//			bspdirentry_t de_faces;			// Faces lump
//			bspdirentry_t de_lightmaps;		// Lightmaps lump
//			bspdirentry_t de_lightvols;		// Lightvolumes lump
//			bspdirentry_t de_visdata;		// Visdata lump
//		};
//		bspdirentry_t direntries[17];
//	};
//};

#pragma pack (pop)

#include <stdio.h>

bspfile_t::bspfile_t(file_t* f) : file(f)
{
	if (file) {
		if (file->size() >= sizeof(bspheader_t)) {
			header = reinterpret_cast<bspheader_t*>(file->data());
			if (header->signature == BSPFILE_MAGIC_NUMBER && header->version == BSPFILE_VERSION) {
				FILE* fp = fopen("level.entities", "w+");
				fwrite(file->data() + header->de_entities.offset, sizeof(char), header->de_entities.length, fp);
				fclose(fp);
				return;
			}
		}
		delete file;
		file = 0;
	}
}

bspfile_t::~bspfile_t()
{
	if (file)
		delete file;
}

bool
bspfile_t::is_valid()
{
	return file != 0;
}

int 
bspfile_t::num_textures()
	// Return the number of textures in this bsp
{
	return header->de_textures.length / sizeof(bsptexture_t);
}

int 
bspfile_t::num_planes()
	// Return the number of planes in this bsp
{
	return header->de_planes.length / sizeof(bspplane_t);
}

int 
bspfile_t::num_nodes()
	// Return the number of nodes in the bsp
{
	return header->de_nodes.length / sizeof(bspnode_t);
}

int 
bspfile_t::num_leaves()
	// Return the number of leaves in the bsp
{
	return header->de_leaves.length / sizeof(bspleaf_t);
}

int 
bspfile_t::num_leaffaces()
	// Return the number of leaf faces in the bsp
{
	return header->de_leaffaces.length / sizeof(bspleafface_t);
}

int 
bspfile_t::num_leafbrushes()
	// Return the number of leaf brushes in the bsp
{
	return header->de_leafbrushes.length / sizeof(bspleafbrush_t);
}

int
bspfile_t::num_lightmaps()
	// Return the number of lightmaps in the bsp
{
	return header->de_lightmaps.length / sizeof(bsplightmap_t);
}

int
bspfile_t::num_models()
	// Return the number of models in the bsp
{
	return header->de_models.length / sizeof(bspmodel_t);
}

int
bspfile_t::num_brushes()
	// Return the number of brushes in the bsp
{
	return header->de_brushes.length / sizeof(bspbrush_t);
}

int
bspfile_t::num_brushsides()
	// Return the number of brushsides in the bsp
{
	return header->de_brushsides.length / sizeof(bspbrushside_t);
}

int
bspfile_t::num_vertices()
	// Return the number of vertices in the bsp
{
	return header->de_vertices.length / sizeof(bspvertex_t);
}

int
bspfile_t::num_meshverts()
	// Return the number of meshverts in the bsp
{
	return header->de_meshverts.length / sizeof(bspmeshvert_t);
}

int
bspfile_t::num_faces()
	// Return the number of faces in the bsp
{
	return header->de_faces.length / sizeof(bspface_t);
}

void
bspfile_t::get_textures(bsp_t::texture_t* dest)
	// Copy the plane information into dest
{
	bsptexture_t* texture = reinterpret_cast<bsptexture_t*>(file->data() + header->de_textures.offset);
	for (int i = 0; i < num_textures(); ++i)
		*dest++ = *texture++;
}

void
bspfile_t::get_planes(plane_t* dest)
	// Copy the plane information into dest
{
	bspplane_t* plane = reinterpret_cast<bspplane_t*>(file->data() + header->de_planes.offset);
	for (int i = 0; i < num_planes(); ++i)
		*dest++ = *plane++;
}

void
bspfile_t::get_nodes(bsp_t::node_t* dest)
	// Copy the node information into dest
{
	bspnode_t* node = reinterpret_cast<bspnode_t*>(file->data() + header->de_nodes.offset);
	for (int i = 0; i < num_nodes(); ++i)
		*dest++ = *node++;
}

void
bspfile_t::get_leaves(bsp_t::leaf_t* dest)
	// Copy the leaf information into dest
{
	bspleaf_t* leaf = reinterpret_cast<bspleaf_t*>(file->data() + header->de_leaves.offset);
	for (int i = 0; i < num_leaves(); ++i)
		*dest++ = *leaf++;
}

void
bspfile_t::get_leaffaces(bsp_t::leafface_t* dest)
	// Copy the leaf face information into dest
{
	bspleafface_t* leafface = reinterpret_cast<bspleafface_t*>(file->data() + header->de_leaffaces.offset);
	for (int i = 0; i < num_leaffaces(); ++i)
		*dest++ = *leafface++;
}

void
bspfile_t::get_leafbrushes(bsp_t::leafbrush_t* dest)
	// Copy the leaf brush indices into dest
{
	bspleafbrush_t* leafbrush = reinterpret_cast<bspleafbrush_t*>(file->data() + header->de_leafbrushes.offset);
	for (int i = 0; i < num_leafbrushes(); ++i)
		*dest++ = *leafbrush++;
}

void
bspfile_t::get_lightmaps(bsp_t::lightmap_t* dest)
{
	bsplightmap_t* lightmap = reinterpret_cast<bsplightmap_t*>(file->data() + header->de_lightmaps.offset);
	for (int i = 0; i < num_lightmaps(); ++i)
		*dest++ = *lightmap++;
}

void
bspfile_t::get_models(bsp_t::model_t* dest)
	// Copy the models into dest
{
	bspmodel_t* model = reinterpret_cast<bspmodel_t*>(file->data() + header->de_models.offset);
	for (int i = 0; i < num_models(); ++i)
		*dest++ = *model++;
}

void
bspfile_t::get_brushes(bsp_t::brush_t* dest)
	// Copy the brushes into dest
{
	bspbrush_t* brush = reinterpret_cast<bspbrush_t*>(file->data() + header->de_brushes.offset);
	for (int i = 0; i < num_brushes(); ++i)
		*dest++ = *brush++;
}

void
bspfile_t::get_brushsides(bsp_t::brushside_t* dest)
	// Copy the brushsides into dest
{
	bspbrushside_t* brushside = reinterpret_cast<bspbrushside_t*>(file->data() + header->de_brushsides.offset);
	for (int i = 0; i < num_brushsides(); ++i)
		*dest++ = *brushside++;
}

void
bspfile_t::get_vertices(vertex_t* dest)
	// Copy the vertices into dest
{
	bspvertex_t* vertex = reinterpret_cast<bspvertex_t*>(file->data() + header->de_vertices.offset);
	for (int i = 0; i < num_vertices(); ++i)
		*dest++ = *vertex++;
}

void
bspfile_t::get_meshverts(bsp_t::meshvert_t* dest)
	// Copy the vertices into dest
{
	bspmeshvert_t* meshvert = reinterpret_cast<bspmeshvert_t*>(file->data() + header->de_meshverts.offset);
	for (int i = 0; i < num_meshverts(); ++i)
		*dest++ = *meshvert++;
}

void
bspfile_t::get_faces(bsp_t::face_t* dest)
	// Copy the faces into dest
{
	bspface_t* face = reinterpret_cast<bspface_t*>(file->data() + header->de_faces.offset);
	for (int i = 0; i < num_faces(); ++i)
		*dest++ = *face++;
}

void
bspfile_t::get_visdata(bsp_t::visdata_t& data)
{
	bspvisdata_t* vdata = reinterpret_cast<bspvisdata_t*>(file->data() + header->de_visdata.offset);
	data.num_vecs = vdata->num_vecs;
	data.vec_size = vdata->vec_size;
	data.vdata = new ubyte[vdata->num_vecs * vdata->vec_size];
	for (int i = 0; i < vdata->num_vecs * vdata->vec_size; ++i)
		data.vdata[i] = vdata->data[i];
}
