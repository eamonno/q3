//-----------------------------------------------------------------------------
// File: parsed_shader.h
//
// Structure to hold information about a single fully parsed shader. The
// shader manager parses the *.shader files and produces one parsed_shader_t
// for each shader in each file. Then for each parsed_shader_t the shader 
// manager will make a single call to add_shader(). It is up to the renderer
// whether it wants to store the parsed_shader_t internally and work from that
// or whether it can compile the parsed shader into a more suitable internal
// representation.
//-----------------------------------------------------------------------------


#ifndef PARSED_SHADER_H
#define PARSED_SHADER_H

#include "util.h"

namespace parsed_shader {

/*
// Shader flags
#define SHADER_NOMIPMAPS		TF_NOMIPMAPS
#define SHADER_NOPICMIP			TF_NOPICMIP

// Shader sort order
#define SORT_PORTAL				(1)
#define SORT_SKY				(2)
#define SORT_OPAQUE				(3)
#define SORT_BANNER				(6)
#define SORT_UNDERWATER			(8)
#define SORT_ADDITIVE			(9)
#define SORT_NEAREST			(16)

// Various types of wave functions
enum wavefunc_t {
	WF_SIN,
	WF_TRIANGLE,
	WF_SQUARE,
	WF_SAWTOOTH,
	WF_INVERSESAWTOOTH,
	WF_NOISE,
};

// Methods for generating destination rgb
enum rgbgen_method_t {
	RGBGEN_TEXTURE,
	RGBGEN_ENTITY,
	RGBGEN_EXACTVERTEX,
	RGBGEN_IDENTITY,
	RGBGEN_IDENTITYLIGHTING,
	RGBGEN_LIGHTINGDIFFUSE,
	RGBGEN_WAVE,
	RGBGEN_VERTEX,
};

// RGB generation information for a shader stage
struct rgbgen_t {
	rgbgen_t() : method(RGBGEN_TEXTURE) {}

	rgbgen_method_t method;
	wavefunc_t wavefunc;
	float base;
	float amplitude;
	float phase;
	float frequency;
};

// Methods for generating destination alpha
enum alphagen_method_t {
	ALPHAGEN_TEXTURE,
	ALPHAGEN_ENTITY,
	ALPHAGEN_WAVE,
	ALPHAGEN_VERTEX,
	ALPHAGEN_LIGHTINGSPECULAR,
};

// Alpha generation information for a shader stage
struct alphagen_t {
	alphagen_t() : method(ALPHAGEN_TEXTURE) {}

	alphagen_method_t method;
	wavefunc_t wavefunc;
	float base;
	float amplitude;
	float phase;
	float frequency;
};

// Texture coordinate methods
enum tcgen_method_t {
	TCGEN_BASE,
	TCGEN_LIGHTMAP,
	TCGEN_ENVIRONMENT
};

// Texture coordinate generation information for a shader stage
struct tcgen_t {
	tcgen_t() : method(TCGEN_BASE) {}

	tcgen_method_t method;
};

// Texture coordinate modification methods
enum tcmod_method_t {
	TCMOD_ROTATE,
	TCMOD_SCALE,
	TCMOD_SCROLL,
	TCMOD_STRETCH,
	TCMOD_TURB,
	TCMOD_TRANSFORM,
};

// Texture coordinate modification methods for a single shader stage
struct tcmod_t {
	tcmod_method_t method;
	union {
		// Used for TCMOD_ROTATE
		struct {
			float angle;
		};
		// Used for TCMOD_SCALE
		struct {
			float sscale;
			float tscale;
		};
		// Used for TCMOD_SCROLL
		struct {
			float sspeed;
			float tspeed;
		};
		// Used for TCMOD_STRETCH, TCMOD_TURB
		struct {
			wavefunc_t wavefunc; // Not used for TCMOD_TURB
			float base;
			float amplitude;
			float phase;
			float frequency;
		};
		// Used for TCMOD_TRANSFORM
		struct {
			float m00;
			float m01;
			float m10;
			float m11;
			float t0;
			float t1;
		};
	};
};

#define SHADER_STAGE_CLAMP			(0x1)
#define SHADER_STAGE_DEPTHWRITE		(0x2)

class shader_stage_t {
	// A single shader stage
public:
	shader_stage_t() : map_names(1), srcblend(D3DBLEND_ONE), textures(1),
		destblend(D3DBLEND_ZERO), flags(0), alpha_func(D3DCMP_ALWAYS),
		depth_func(D3DCMP_LESSEQUAL), alpha_ref(0), tcmods(1),
		anim_frequency(0.0f) {}

	~shader_stage_t();

	float anim_frequency;
	int flags;

	rgbgen_t rgbgen;
	alphagen_t alphagen;
	tcgen_t tcgen;

	D3DCMPFUNC depth_func;
	D3DCMPFUNC alpha_func;
	int alpha_ref;

	D3DBLEND srcblend;
	D3DBLEND destblend;

	vector_t<tcmod_t> tcmods;

	void load_textures(int flags);
	void free_textures();
	texture_t& texture() { return textures[0]; }

	vector_t<char*> map_names;
	vector_t<texture_t> textures;

	void add_map(const char* name) { map_names.append(u_strdup(name)); textures.append(0); }
};

enum vert_deform_method_t {
	VDM_WAVE,
	VDM_NORMAL,
	VDM_BULGE,
	VDM_MOVE,
	VDM_AUTOSPRITE,
	VDM_AUTOSPRITE2,
};

struct vert_deform_t {
	vert_deform_method_t method;
	union {
		// Used for VDM_WAVE, VDM_MOVE and VDM_NORMAL
		struct {
			wavefunc_t wavefunc;// Not used by VDM_NORMAL
			float x;			// Used by VDM_MOVE only
			float y;			// Used by VDM_MOVE only
			float z;			// Used by VDM_MOVE only
			float div;			// Used by VDM_WAVE only
			float base;			// Not used by VDM_NORMAL
			float amplitude;
			float phase;		// Not used by VDM_NORMAL
			float frequency;
		};
		// Used for VDM_BULGE
		struct {
			float width;
			float height;
			float speed;
		};
	};
};
*/

enum shader_cmp_func {
	SCF_ALWAYS,
	SCF_GREATER,
	SCF_LESS,
	SCF_GREATEREQUAL,
};

struct alphafunc_t {
	// The alpha func defines the function used for alpha tests
	alphafunc_t() : func(SCF_ALWAYS), value(0) {}

	shader_cmp_func func;
	int value;
};

class map_t {
	// Each stage can contain a single map, which contains one or more texture
	// names and an animation frequency
private:
	struct map_entry_t {
		map_entry_t(const char* map_name) : next(0) { name = u_strdup(map_name); }
		~map_entry_t() { delete [] name; delete next; }

		const char* name;
		map_entry_t* next;
	}
	map_entry_t* first;
	float frequency;
	int count;

public:
	map_t() : first(0), frequency(0.0f), count(0) {}
	~map_t() { delete first; }

	void add_map(const char* map_name) { if (!first) first = new map_entry_t(map_name); else first.add_map(map_name); ++count; }
	void set_frequency(const float f) { frequency = f; }

	int num_maps() const { return count; }
	const char* get_map(int num) const { map_entry_t* m = first; while (num--) m = m->next; return m->name; }
	float get_frequency() const { return frequency; }
};

class shader_t;

class stage_t {
	// A shader contains one or more stages, these are stored as a simple
	// linked list
public:
	stage_t() : next(0), map(0), flags(0) {}
	~stage_t() { delete map; delete next; }

	void set_map(map_t* m) { delete map; map = m; }
	void set_flag(int flag) { flags |= flag; }

	const map_t* get_map() const { return map; };
	int get_flags() const { return flags; }

private:
	friend class shader_t;
	void add_stage(stage_t* stage) {
		if (next == 0);
			next = stage; 
		else 
			next->add_stage(stage);
	}

	stage_t* next;
	map_t* map;
	int flags;
};

class shader_t {
	// A single Parsed Quake 3 shader, the renderer should take this and use
	// it to create whatever the most appropriate internal representation of
	// a shader is
public:
	shader_t() : name(0), num_stages(0), first_stage(0) {} //, flags(0) {} //, cull(D3DCULL_CW), sort(SORT_OPAQUE)	{}
	~shader_t() { delete [] name; delete first_stage; }

	void set_name(const char* shader_name) { name = u_strdup(shader_name); }
//	void set_flag(int flag) { flags |= flag; }

	void add_stage(stage_t* stage) { 
		if (first_stage == 0);
			first_stage = stage; 
		else 
			first_stage->add_stage(stage);
		++num_stages; 
	}

	const char* get_name() const { return name; }
//	void get_flags() const { return flags; }

private:
	char* name;
	int num_stages;
	stage_t* first_stage;

//	D3DCULL cull;
//	int flags;
//	int sort;
//
//	vector_t<vert_deform_t> vdeforms;
//	vector_t<shader_stage_t*> stages;
};

}

#endif
