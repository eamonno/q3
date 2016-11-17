//-----------------------------------------------------------------------------
// File: camera.h
//
// Camera control
//-----------------------------------------------------------------------------

#ifndef CAMERA_H
#define CAMERA_H

#include "maths.h"

class camera_t {
	// Camera which moves through the scene
public:
	camera_t() { mat_world = mat_view = mat_proj = matrix_t::identity; }

	matrix_t mat_world;
	matrix_t mat_view;
	matrix_t mat_proj;
};

#endif
