//-----------------------------------------------------------------------------
// File: entity.h
//
// Base class for entities in the world, and classes for the parsing of those
// entities
//-----------------------------------------------------------------------------

#ifndef ENTITY_H
#define ENTITY_H

#include "maths.h"
#include "input.h"
#include "timer.h"
#include "str.h"

// Entity flags
#define EF_MOVE_FORWARD		0x001
#define EF_MOVE_BACKWARD	0x002
#define EF_MOVE_LEFT		0x004
#define EF_MOVE_RIGHT		0x008
#define EF_MOVE_UP			0x010
#define EF_MOVE_DOWN		0x020
#define EF_TURN_LEFT		0x040
#define EF_TURN_RIGHT		0x080
#define EF_TURN_UP			0x100
#define EF_TURN_DOWN		0x200

class entity_t {
	// A single active object in the world
public:
	entity_t() : position(vec3_t::origin) {}

	vec3_t	position;	// Where in the world is this entity
};

class player_t : public entity_t {
	// A player representation
public:
	player_t() : rotation(vec3_t::origin), look(0.0f, 0.0f, 1.0f), up(0.0f, 1.0f, 0.0f) {}

	vec3_t	rotation;	// (x, y, z) = (pitch, roll, yaw)
	vec3_t	look;		// direction the player is looking
	vec3_t	up;			// Up vector for looking
};

class user_controls_t {
	// Updates an entity based on input from the user
public:
	int key_move_forward;
	int key_move_backward;
	int key_move_left;
	int key_move_right;
	int key_move_up;
	int key_move_down;
	int key_turn_left;
	int key_turn_right;
	int key_turn_up;
	int key_turn_down;

	float move_speed;	// Advance this distance per second
	float turn_speed;	// Turn this amount per sec (degrees)

	user_controls_t() :
		key_move_forward(VK_UP),
		key_move_backward(VK_DOWN),
		key_move_left(VK_LEFT),
		key_move_right(VK_RIGHT),
		key_move_up(VK_HOME),
		key_move_down(VK_END),
		key_turn_left(VK_DELETE),
		key_turn_right(VK_INSERT),
		key_turn_up(VK_PRIOR),
		key_turn_down(VK_NEXT),
		move_speed(400.0f),
		turn_speed(30.0f)
	{}

	void update_player(player_t& player) {
		// Create the vector for movement

		if (keyboard.is_key_down(key_move_forward)) {
			player.position += player.look * (move_speed * timer.elapsed(TID_APP));
		}
		if (keyboard.is_key_down(key_move_backward)) {
			player.position -= player.look * (move_speed * timer.elapsed(TID_APP));
		}
		if (keyboard.is_key_down(key_move_left)) {
			player.position -= cross(player.look, player.up) * (move_speed * timer.elapsed(TID_APP));
		}
		if (keyboard.is_key_down(key_move_right)) {
			player.position += cross(player.look, player.up) * (move_speed * timer.elapsed(TID_APP));
		}
		// Vertical movement (ignores facing, moves on y axis
		if (keyboard.is_key_down(key_move_up))
			player.position.y += move_speed * timer.elapsed(TID_APP);
		if (keyboard.is_key_down(key_move_down))
			player.position.y -= move_speed * timer.elapsed(TID_APP);

		// Turning on the y (yaw) axis
		if (keyboard.is_key_down(key_turn_left))
			player.rotation.y += turn_speed * timer.elapsed(TID_APP);
		if (keyboard.is_key_down(key_turn_right))
			player.rotation.y -= turn_speed * timer.elapsed(TID_APP);

		// Turning on the x (pitch) axis constrained at vertical looking
		if (keyboard.is_key_down(key_turn_up))
			player.rotation.x += turn_speed * timer.elapsed(TID_APP);
		if (keyboard.is_key_down(key_turn_down))
			player.rotation.x -= turn_speed * timer.elapsed(TID_APP);

		// Mouse movement
		player.rotation.x -= mouse.change().y;
		player.rotation.y -= mouse.change().x;

		// Keep the value for y rotation between 0 and 360
		while (player.rotation.y >= 360.0f)
			player.rotation.y -= 360.0f;
		while (player.rotation.y < 0.0f)
			player.rotation.y += 360.0f;

		// Constrain x rotation so that you cant look beyond vertically up or down
		if (player.rotation.x > 90.0f)
			player.rotation.x = 90.0f;
		if (player.rotation.x < -90.0f)
			player.rotation.x = -90.0f;

		mouse.reset();

		// Update the players look vector
		matrix_t rot_x, rot_y;
		rot_x.rotation_x(m_deg2rad(player.rotation.x));
		rot_y.rotation_y(m_deg2rad(player.rotation.y));
		player.look = (rot_x * rot_y).transform_point(vec3_t(0.0f, 0.0f, 1.0f));
		rot_x.rotation_x(m_deg2rad(player.rotation.x - 90.0f));
		player.up = (rot_x * rot_y).transform_point(vec3_t(0.0f, 0.0f, 1.0f));
	}
};

enum entity_type_t {
	ENTTYPE_UNKNOWN,
	ENTTYPE_WORLDSPAWN,
};

class entity_properties_t {
	// Parsed entity properties from the entities section of a bsp file
public:
	typedef str_t<64> property_t;

	// The properties as stored in the file
	property_t prop_angle;
	property_t prop_classname;
	property_t prop_color;
//	property_t prop_delay;
//	property_t prop_dmg;
//	property_t prop_health;
	property_t prop_light;
	property_t prop_lip;
	property_t prop_message;
	property_t prop_minlight;
	property_t prop_model;
	property_t prop_model2;
//	property_t prop_music;
	property_t prop_nobots;
	property_t prop_noise;
//	property_t prop_notfree;
//	property_t prop_notteam;
	property_t prop_origin;
	property_t prop_radius;
	property_t prop_random;
//	property_t prop_range;
	property_t prop_roll;
	property_t prop_spawnflags;
	property_t prop_speed;
	property_t prop_target;
	property_t prop_targetname;
	property_t prop_team;
	property_t prop_wait;
//	property_t prop_weight;

	// The various properties after parsing
	entity_type_t type;

	void reset();
	void set_property(const property_t& name, const property_t& value);

private:

};

#endif
