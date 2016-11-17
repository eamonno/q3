//-----------------------------------------------------------------------------
// File: world.h
//
// Contains the game world, including the current map, all entities, and
// anything else that only exists when running around an actual game map
//-----------------------------------------------------------------------------

#ifndef WORLD_H
#define WORLD_H

#include "bsp.h"
#include "str.h"
#include "types.h"

class world_t {
	// The game world
public:
	world_t();
	~world_t();

	void	set_map(const char* name);

	bool	is_valid()	{ return valid; }

	void	tesselate(display_list_t &dl, const vec3_t& eye, const frustum_t& frustum) 
			{ bsp.tesselate(dl, eye, frustum); }

	int		resources_to_load();
	void	load_resource();
	void	free_resources();

	static world_t& get_instance();

	str_t<64>	map_name;		// Name of the .bsp file loaded
	str_t<64>	level_name;		// Name of the level

private:
	bool	parse_entities(const char* text, uint length);

	bsp_t		bsp;
	hsound_t	soundtrack;

	bool		valid;
};

#define world (world_t::get_instance())

#endif
