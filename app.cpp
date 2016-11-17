//-----------------------------------------------------------------------------
// File: app.cpp
//
// Desc: Main windows interface code
//-----------------------------------------------------------------------------

#include "app.h"
#include "console.h"
#include "pak.h"
#include "daudio.h"
#include "d3d.h"
#include "timer.h"
#include "displaylist.h"
#include "font.h"
#include "world.h"
#include "shader_parser.h"
#include "input.h"
#include "entity.h"
#include "exec.h"
#include <memory>

using std::auto_ptr;

#include "mem.h"
#define new mem_new

cvstr_t
map_callback(int argc, cvstr_t* argv)
{
	if (argc == 1)
		app.set_game_map(argv[0].c_str());
	return cvstr_t();
}

cvstr_t
quit_callback(int argc, cvstr_t* argv)
{
	PostQuitMessage(0);
	return cvstr_t();
}

cvar_str_t		console_shader("console_shader", "console", CVF_CONST);
cvar_float_t	fov("fov", 90.0f, CVF_NONE, 10.0f, 180.0f);
cvar_int_t		showfps("showfps", 1, CVF_NONE, 0, 1);
cvar_int_t		showpos("showpos", 1, CVF_NONE, 0, 1);
cvar_int_t		showrenderstats("showrenderstats", 1, CVF_NONE, 0, 1);
cvar_int_t		cleartarget("cleartarget", 1, CVF_NONE, 0, 1);
cfunc_t			cf_map("map", map_callback, 0, 1);
cfunc_t			cf_quit("quit", quit_callback, 0, 0);

const char* const APP_NAME = "Quake 3 Rip Off by Eamonn O'Brien";

app_t&
app_t::get_instance()
	// Returns the singleton instance of app_t
{
	static std::auto_ptr<app_t> instance(new app_t());
	return *instance;
}

result_t
app_t::create(const char* cmd_line)
	// Create the main application window
{
	// Read config files and commandline
	mmfile_t startup_scr("startup.scr");
	if (startup_scr.valid())
		exec.exec_script(reinterpret_cast<const char*>(startup_scr.data()), startup_scr.size());
	exec.exec_script(cmd_line, u_strlen(cmd_line));
	exec.lock_consts();	// consts are now fixed until shutdown

	console.print(DIVIDER);
	console.printf("%s\n", APP_NAME);
	console.print(DIVIDER);

	// Set up the UI camera
	vec3_t eye(0.0f, 0.0f, 2.0f);
	vec3_t at(0.0f, 0.0f, 0.0f);
	vec3_t up(0.0f, 1.0f, 0.0f);
	ui_camera.mat_view.look_at_rh(eye, at, up);
	ui_camera.mat_proj.ortho_rh(0.0f, UI_WIDTH, 0.0f, UI_HEIGHT, 1.0f, 500.0f);

	timer.init();

	console.print("Initializing PAK manager:\n");
	if (!pak.init())
		return result_t::last;
	console.print(DIVIDER);
	console.print("Initializing Audio manager:\n");
	if (!daudio.create())
		return result_t::last;

	if (d3d.init()) {
		pak_file_finder_t shader_finder;
		shader_finder.search("scripts/", "*.shader");
		console.print(DIVIDER);
		console.print("Initializing shaders:\n");
		shader_parser_t parser;
		for (int i = 0; i < shader_finder.count(); i++) {
			console.printf("Parsing shaders from file %s\n", shader_finder[i]);
			parser.parse_file(shader_finder[i]);
		}
		console.print(DIVIDER);
		if (d3d.create()) {
			console.set_shader(d3d.get_shader(*console_shader, true));
			font.set_shader(d3d.get_shader("gfx/2d/bigchars", true));
			font.set_resolution(80.0f, 60.0f);
			timer.start(TID_APP);
			return true;
		}
	}
	return false;
}

void
app_t::destroy()
{
	console.print("Cleaning Up Direct3D ... ");
	d3d.destroy();
	daudio.destroy();
	console.print("done\n");
}

player_t player;
user_controls_t controls;

void
app_t::set_game_map(const char* name)
{
	world.free_resources();
	d3d.free_resources();
	player.position = vec3_t(100.0f, 100.0f, 100.0f);
	world.set_map(name);
}

void
app_t::frame()
	// Prepare and draw a single frame
{
	static float time = 0.0f;
	static int frames;
	static char framerate[10] = "FPS: ---";
	static render_stats_t total_stats;

	char temptext[1024];	// Buffer for temporary text

	timer.mark(TID_APP);
	if (m_floor(time) == m_floor(timer.time(TID_APP))) {
		frames++;
	} else {
		time = timer.time(TID_APP);
		u_snprintf(framerate, 10, "FPS: %3d", frames);
		frames = 0;
	}

	if (!console.is_showing())
		controls.update_player(player);

	static bool tildestate;
	if (tildestate != keyboard.is_key_down(223)) {
		if (tildestate)
			console.toggle_visible();
		tildestate = !tildestate;
	}

	int num_faces = 0, num_verts = 0, num_inds = 0;
	render_stats_t stats;

	if (d3d.begin()) {
		if (*cleartarget)
			d3d.clear(D3DCLEAR_TARGET | D3DCLEAR_STENCIL | D3DCLEAR_ZBUFFER, color_t::blue);
		else
			d3d.clear(D3DCLEAR_STENCIL | D3DCLEAR_ZBUFFER);

		// Set the world camera
		if (d3d.resources_to_load()) {
			d3d.load_resource();
		} else if (world.resources_to_load()) {
			world.load_resource();
		} else if (world.is_valid()) {
			dl.clear();
			RECT rect;
			GetClientRect(hwnd, &rect);
			float aspect = m_itof(rect.bottom - rect.top) / m_itof(rect.right - rect.left);

			camera_t world_cam;
			world_cam.mat_view.look_at_rh(player.position, player.position + player.look, player.up);
			world_cam.mat_proj.perspective_fov_rh(m_deg2rad(*fov), aspect, 4.0f, 8000.0f);
			d3d.set_camera(world_cam);

			// Render the world
			world.tesselate(dl, player.position, frustum_t(world_cam.mat_view * world_cam.mat_proj));
			stats += d3d.render_list(dl);
		}
		// Render the console and overlay text
		d3d.set_camera(ui_camera);
	
		dl.clear();
		console.tesselate(dl);

		// Show framerate if required
		if (*showfps) {
			font.set_color(color_t::red);
			font.write_text(dl, 69.5f, 60.0f, framerate);
		}

		float overlay_text_line = 0.0f;

		// Show the map name
		if (world.is_valid()) {
			font.set_color(color_t::yellow);
			u_snprintf(temptext, 1024, "%s (%s)", world.level_name.c_str(), world.map_name.c_str());
			font.write_text(dl, 0.5f, 60.0f - overlay_text_line, temptext);
			overlay_text_line += 2.0f;
		}

		font.set_color(color_t::green);

		// Show position if required
		if (*showpos) {
			u_snprintf(temptext, 1024, "Position (%.2f, %.2f, %.2f) Facing (%.2f, %.2f, %.2f)", 
				player.position.x, player.position.y, player.position.z, 
				player.look.x, player.look.y, player.look.z);
			font.write_text(dl, 0.5f, 60.0f - overlay_text_line, temptext);
			overlay_text_line += 1.0f;
		}
		stats += d3d.render_list(dl);
		dl.clear();
		total_stats += stats;

		// Show rendering stats if required (the stats for these lines themselves are omitted
		// since it is a bit of a chicken and egg problem).
		if (*showrenderstats) {
			u_snprintf(temptext, 1024, "Frame render stats: %d faces, %d vertices, %d indices",
				stats.num_faces, stats.num_verts, stats.num_inds);
			font.write_text(dl, 0.5f, 60.0f - overlay_text_line, temptext);
			overlay_text_line += 1.0f;
			u_snprintf(temptext, 1024, "Total render stats: %d faces, %d vertices, %d indices",
				total_stats.num_faces, total_stats.num_verts, total_stats.num_inds);
			font.write_text(dl, 0.5f, 60.0f - overlay_text_line, temptext);
			overlay_text_line += 1.0f;
		}
		d3d.render_list(dl);

		d3d.end();
	}
}
