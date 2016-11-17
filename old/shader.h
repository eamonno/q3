//-----------------------------------------------------------------------------
// File: shader.h
//
// Quake 3 shaders
//-----------------------------------------------------------------------------

#ifndef SHADER_H
#define SHADER_H

#include "d3d.h"
#include "util.h"
#include "file.h"

#include "parsed_shader.h"

class shader_manager_t {
	// Manages all the shaders that will be needed by the app
public:
	~shader_manager_t() { destroy(); }

	void parse_file(const char* filename);

	int num_shaders() const { return shaders.size(); }
	int num_stages() const;
	int num_errors() const { return errors; }

	shader_t* get_shader(const char* name);
	void release_shader(shader_t* shader);

	void destroy();

	static shader_manager_t& get_instance();

private:
	shader_manager_t() : errors(0) {}

	void warn(const char* filename, const tokenizer_t& tok, const char* message);

	int errors;

	vector_t<shader_t*> shaders;
	vector_t<int> ref_counts;
};

#define shader_manager (shader_manager_t::get_instance())

#endif
