//-----------------------------------------------------------------------------
// File: d3ddev.h
//
// Desc: Wrapper class for direct 3d rendering
//-----------------------------------------------------------------------------

#ifndef D3DDEV_H
#define D3DDEV_H
#endif

#include "d3d.h"

extern const uint SF_ACTIVE;	// Shader is currently in use
extern const uint SF_RETAIN;	// Never unload this shader
extern const uint SF_NOMIPMAPS;	// No mipmaps for this shader
extern const uint SF_NOPICMIP;	// Never picmip this shader
extern const uint SF_CULLFRONT;	// Cull front faces
extern const uint SF_CULLBACK;	// Cull back faces (default)

class d3ddev_t {
	// Handles any Direct3D related resources.
public:
	struct texture_t;
	struct shader_t;
	struct shader_pass_t;

	~d3ddev_t() { destroy(); }

	result_t init();
	result_t create();

	void destroy();

	bool lost();

	bool begin();
	void end();

	void clear(uint flags = D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		color_t color = color_t::black, float z = 1.0f, uint stencil = 0,
		uint count = 0, const D3DRECT* rects = NULL)
	{ d3ddev->Clear(count, rects, flags, color, z, stencil); }

	void set_camera(const camera_t& camera);

	void render_list(display_list_t& dl);

	hshader_t	get_shader(const char* name, bool retain = false);
	uint		get_surface_flags(hshader_t shader);

	// Returns the number of resources that have been requested but not yet loaded
	int			resources_to_load()		{ return textures_to_load; }
	void		load_resource();
	void		free_resources();

	// The upload functions are used to pass data to the renderer
	void		upload_shader(const token_t* tokens);
	htexture_t	upload_lightmap_rgb(const ubyte* data, const int width, const int height, const char* name);
	int			upload_static_verts(const vertex_t* verts, int count);
	int			upload_static_inds(const index_t* inds, int count);

	static d3ddev_t& get_instance();

private:

	void show_tris(display_list_t& dl);
	void render_no_shader(const face_t* face);

	int num_textures;
	int num_shaders;
	int num_passes;

	shader_t*		shaders;
	texture_t*		textures;
	shader_pass_t*	passes;

	const char* get_error_string(HRESULT hr);
	bool		generate_mipmaps(texture_t& texture);

	int		textures_to_load;

	d3d_t();

	// COM objects, note that d3d MUST be the FIRST object declared here as its
	// destructor has to be the last called to ensure that all of the dependent
	// resources are released before d3d itself.
	com_ptr_t<IDirect3DDevice8>			d3ddev;
	com_ptr_t<IDirect3DIndexBuffer8>	ibuf;
	com_ptr_t<IDirect3DIndexBuffer8>	sibuf;
	com_ptr_t<IDirect3DVertexBuffer8>	vbuf;
	com_ptr_t<IDirect3DVertexBuffer8>	svbuf;

	int			num_static_verts;	// Number of static vertices
	int			num_static_inds;	// Number of static vertices

	D3DPRESENT_PARAMETERS d3dpp;

	hshader_t current_shader;
	htexture_t current_lightmap;
	int current_pass;
	int current_shader_passes;

	void begin_shader(hshader_t shader, htexture_t lightmap);
	void begin_pass(int num);
	void end_shader();
	void end_pass();

	d3dinfo_t info;

	htexture_t define_texture(const char* name);
	htexture_t get_texture(const char* name);
	void load_texture(htexture_t texture, int flags);

	// Functions for selecting which formats etc will be used
	result_t choose_adapter();
	result_t choose_adapter_device();
	result_t choose_adapter_format();
	result_t choose_adapter_mode();
	result_t choose_adapter_back_buffer_format();
	result_t choose_adapter_depth_stencil_format();
	result_t confirm_device(const d3dinfo_t::device_t& device);
};

#define d3ddev (d3ddev_t::get_instance())

#define D3DDEV_H
