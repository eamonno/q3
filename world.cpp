//-----------------------------------------------------------------------------
// File: world.cpp
//
// The world contains the bsp for the game, and all the game entities. 
//-----------------------------------------------------------------------------

#include "world.h"
#include "pak.h"
#include "types.h"
#include "console.h"
#include "d3d.h"
#include "entity.h"
#include <memory>

#include "mem.h"
#define new mem_new

using std::auto_ptr;

namespace {
	const int BSPFILE_MAGIC_NUMBER = 0x50534249;	// "IBSP"
	const int BSPFILE_VERSION = 0x2e;	// Version number to read

#pragma pack (push, 1)

	// The file header structure for bsp files
	struct bspheader_t {
		struct direntry_t {
			uint offset;		// Offset from start of file to this lump
			uint length;		// Size of the lump
		};

		uint signature;	// File signiture, should equal BSPFILE_MAGIC_NUMBER
		uint version;	// File version, should equal BSPFILE_VERSION
		direntry_t de_entities;		// Entities lump
		direntry_t de_textures;		// Textures lump
		direntry_t de_planes;		// Planes lump
		direntry_t de_nodes;		// Nodes lump
		direntry_t de_leaves;		// Leaves lump
		direntry_t de_leaffaces;	// Leaf faces lump
		direntry_t de_leafbrushes;	// Leaf brushes lump
		direntry_t de_models;		// Models lump
		direntry_t de_brushes;		// Brush lump
		direntry_t de_brushsides;	// Brush sides lump
		direntry_t de_vertices;		// Vertices lump
		direntry_t de_meshverts;	// Meshvert lump
		direntry_t de_effects;		// Effects lump
		direntry_t de_faces;		// Faces lump
		direntry_t de_lightmaps;	// Lightmaps lump
		direntry_t de_lightvols;	// Lightvolumes lump
		direntry_t de_visdata;		// Visdata lump
	};
#pragma pack (pop)

}

world_t::world_t() :
	valid(false)
{
}

world_t::~world_t()
{
	free_resources();
	bsp.destroy();
}

world_t&
world_t::get_instance()
{
	static auto_ptr<world_t> instance(new world_t());
	return *instance;
}

int
world_t::resources_to_load()
{
	return bsp.resources_to_load();
}

void
world_t::set_map(const char* name)
{
	valid = false;	// Level is no longer good for rendering

	// Release any resources in use
	bsp.free_resources();

	map_name = name;
	filename_t filename = "maps/";
	filename += name;
	filename += ".bsp";

	// Open the file
	auto_ptr<file_t> file(pak.open_file(filename));

	if (file.get() == 0) {	// Check that the file was opened
		console.printf("Failed to open bsp file %s: file not found\n", filename.c_str());
		return;
	}
	if (file->size() < sizeof(bspheader_t)) {	// Check file is big enough to be a bsp
		console.printf("Failed to open bsp file %s: file too small for valid bsp file\n", filename.c_str());
		return;
	}

	bspheader_t* header = reinterpret_cast<bspheader_t*>(file->data());
	if (header->signature != BSPFILE_MAGIC_NUMBER) {	// Check the magic number for the file
		console.printf("Failed to load bsp file %s: file signature incorrect\n", filename.c_str());
		return;
	}
	if (header->version != BSPFILE_VERSION) {	// Check the file version
		console.printf("Failed to load bsp file %s: file version is not %d\n", filename.c_str(), BSPFILE_VERSION);
		return;
	}

	// Now load the various parts of the bsp itself
//	if (!load_entities(file->data() + header.de_entities.offset, header.de_entities.length))
//		return false;
	if (!bsp.load_textures(file->data() + header->de_textures.offset, header->de_textures.length))
		return;
	if (!bsp.load_planes(file->data() + header->de_planes.offset, header->de_planes.length))
		return;
	if (!bsp.load_nodes(file->data() + header->de_nodes.offset, header->de_nodes.length))
		return;
	if (!bsp.load_leaves(file->data() + header->de_leaves.offset, header->de_leaves.length))
		return;
	if (!bsp.load_leaffaces(file->data() + header->de_leaffaces.offset, header->de_leaffaces.length))
		return;
	if (!bsp.load_leafbrushes(file->data() + header->de_leafbrushes.offset, header->de_leafbrushes.length))
		return;
	if (!bsp.load_models(file->data() + header->de_models.offset, header->de_models.length))
		return;
	if (!bsp.load_brushes(file->data() + header->de_brushes.offset, header->de_brushes.length))
		return;
	if (!bsp.load_brushsides(file->data() + header->de_brushsides.offset, header->de_brushsides.length))
		return;
	if (!bsp.load_vertices(file->data() + header->de_vertices.offset, header->de_vertices.length))
		return;
	if (!bsp.load_meshverts(file->data() + header->de_meshverts.offset, header->de_meshverts.length))
		return;
//	if (!load_effects(file->data() + header->de_effects.offset, header->de_effects.length))
//		return;
	if (!bsp.load_faces(file->data() + header->de_faces.offset, header->de_faces.length))
		return;
	if (!bsp.load_lightmaps(file->data() + header->de_lightmaps.offset, header->de_lightmaps.length))
		return;
	if (!bsp.load_lightvols(file->data() + header->de_lightvols.offset, header->de_lightvols.length))
		return;
	if (!bsp.load_visdata(file->data() + header->de_visdata.offset, header->de_visdata.length))
		return;

	d3d.upload_static_verts(bsp.vertices, bsp.num_vertices);
	d3d.upload_static_inds(bsp.meshverts, bsp.num_meshverts);

	// Load the world entities directly into the world
	if (!parse_entities(reinterpret_cast<const char*>(file->data() + header->de_entities.offset), header->de_entities.length))
		return;

	valid = true;	// Everything worked, it is safe to render the level again
}

bool
world_t::parse_entities(const char* text, uint length)
{
// Dump the entities to a text file
//
	filename_t fname = map_name;
	fname += "_entities.txt";

	FILE* fp = fopen(fname.c_str(), "w+");
	fwrite(text, sizeof(char), length, fp);
	fclose(fp);

	entity_properties_t properties;
	entity_properties_t::property_t propname;
	entity_properties_t::property_t propvalue;

	tokenizer_t tok(text, length);
	while (tok.next_token() && u_strcmp(tok.current_token(), "{") == 0) {
		properties.reset();
		while (tok.next_token() && u_strcmp(tok.current_token(), "}") != 0) {
			tok.current_token(propname, propname.size());
			tok.next_token(propvalue, propname.size());
			properties.set_property(propname, propvalue);
		}
		switch (properties.type) {
		case ENTTYPE_WORLDSPAWN:
			level_name = properties.prop_message;
			break;
		default:
			console.printf("unrecognized world entity %s\n", properties.prop_classname.c_str());
		}
	}

	return true;
}

void
world_t::load_resource()
{
	bsp.load_resource();
}

void
world_t::free_resources()
{
	valid = false;
	bsp.free_resources();
}
