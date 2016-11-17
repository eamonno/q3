//-----------------------------------------------------------------------------
// File: app.h
//
// The main application itself
//-----------------------------------------------------------------------------

#ifndef APP_H
#define APP_H

#include "camera.h"
#include "displaylist.h"
#include "util.h"

extern const char* const APP_NAME;

class app_t {
	// The actual application
public:
	result_t create(const char* cmd_line);
	void destroy();
	void frame();

	void set_game_map(const char* name);

	static app_t& get_instance();

private:
	app_t() {}

	camera_t ui_camera;
	display_list_t dl;
};

#define app (app_t::get_instance())

#endif
