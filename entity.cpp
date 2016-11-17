//-----------------------------------------------------------------------------
// File: entity.cpp
//
// Desc: Game entities
//-----------------------------------------------------------------------------

#include "entity.h"
#include "console.h"

void
entity_properties_t::reset()
{
	type = ENTTYPE_UNKNOWN;

	prop_angle[0] = '\0';
	prop_classname[0] = '\0';
	prop_color[0] = '\0';
//	prop_delay[0] = '\0';
//	prop_dmg[0] = '\0';
//	prop_health[0] = '\0';
	prop_light[0] = '\0';
	prop_lip[0] = '\0';
	prop_message[0] = '\0';
	prop_minlight[0] = '\0';
	prop_model[0] = '\0';
	prop_model2[0] = '\0';
//	prop_music[0] = '\0';
	prop_nobots[0] = '\0';
	prop_noise[0] = '\0';
//	prop_notfree[0] = '\0';
//	prop_notteam[0] = '\0';
	prop_origin[0] = '\0';
	prop_radius[0] = '\0';
	prop_random[0] = '\0';
//	prop_range[0] = '\0';
	prop_roll[0] = '\0';
	prop_spawnflags[0] = '\0';
	prop_speed[0] = '\0';
	prop_target[0] = '\0';
	prop_targetname[0] = '\0';
	prop_team[0] = '\0';
	prop_wait[0] = '\0';
//	prop_weight[0] = '\0';
}

void
entity_properties_t::set_property(const property_t& name, const property_t& value)
{
	bool success = true;

	if (name.cmpi("angle") == 0) {
		prop_angle = value;
	} else if (name.cmpi("classname") == 0) {
		prop_classname = value;
		if (prop_classname.cmpi("worldspawn") == 0)
			type = ENTTYPE_WORLDSPAWN;
	} else if (name.cmpi("_color") == 0) {
		prop_color = value;
	} else if (name.cmpi("light") == 0) {
		prop_light = value;
	} else if (name.cmpi("lip") == 0) {
		prop_lip = value;
	} else if (name.cmpi("message") == 0) {
		prop_message = value;
	} else if (name.cmpi("_minlight") == 0) {
		prop_minlight = value;
	} else if (name.cmpi("model") == 0) {
		prop_model = value;
	} else if (name.cmpi("model2") == 0) {
		prop_model2 = value;
	} else if (name.cmpi("nobots") == 0) {
		prop_nobots = value;
	} else if (name.cmpi("noise") == 0) {
		prop_noise = value;
	} else if (name.cmpi("origin") == 0) {
		prop_origin = value;
	} else if (name.cmpi("radius") == 0) {
		prop_radius = value;
	} else if (name.cmpi("random") == 0) {
		prop_random = value;
	} else if (name.cmpi("roll") == 0) {
		prop_roll = value;
	} else if (name.cmpi("spawnflags") == 0) {
		prop_spawnflags = value;
	} else if (name.cmpi("speed") == 0) {
		prop_speed = value;
	} else if (name.cmpi("target") == 0) {
		prop_target = value;
	} else if (name.cmpi("targetname") == 0) {
		prop_targetname = value;
	} else if (name.cmpi("team") == 0) {
		prop_team = value;
	} else if (name.cmpi("wait") == 0) {
		prop_wait = value;
	} else {
		console.printf("unknown entity property \"%s\", \"%s\")\n", name.c_str(), value.c_str());
	}
}
