//-----------------------------------------------------------------------------
// File: d3d.h
//
// Desc: Wrapper class for direct 3d rendering
//-----------------------------------------------------------------------------

#ifndef D3D_H
#define D3D_H

#include "d3dinfo.h"

extern const uint SF_ACTIVE;	// Shader is currently in use
extern const uint SF_RETAIN;	// Never unload this shader
extern const uint SF_NOMIPMAPS;	// No mipmaps for this shader
extern const uint SF_NOPICMIP;	// Never picmip this shader
extern const uint SF_CULLFRONT;	// Cull front faces
extern const uint SF_CULLBACK;	// Cull back faces (default)

struct render_stats_t {
	// Simple struct for tracking statistics on rendering

	int		num_faces;		// Number of faces rendered
	int		num_verts;		// Number of vertices referenced
	int		num_inds;		// Number of indices referenced

	render_stats_t(int nf = 0, int nv = 0, int ni = 0) :
		num_faces(nf),
		num_verts(nv),
		num_inds(ni)
	{}

	void operator+=(const render_stats_t& rs)
	{
		num_faces += rs.num_faces;
		num_verts += rs.num_verts;
		num_inds += rs.num_inds;
	}

	render_stats_t operator+(const render_stats_t& rs)
	{
		return render_stats_t(num_faces + rs.num_faces, num_verts + rs.num_verts, num_inds + rs.num_inds);
	}
};

class d3d_t {
	// Handles any Direct3D related resources.
public:
	struct shader_t;
	struct shader_pass_t;

	~d3d_t() { destroy(); }

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

	render_stats_t	render_list(display_list_t& dl);

	void		set_camera(const camera_t& camera);

	hshader_t	get_shader(const char* name, bool retain = false);
	uint		get_surface_flags(hshader_t shader);

	// Returns the number of resources that have been requested but not yet loaded
	int			resources_to_load();
	void		load_resource();
	void		free_resources();

	// Print a list of shaders to the console
	void		list_shaders(uint first, uint num);

	// The upload functions are used to pass data to the renderer
	void		upload_shader(const token_t* tokens);
	htexture_t	upload_lightmap_rgb(const ubyte* data, const int width, const int height, const char* name);
	int			upload_static_verts(const vertex_t* verts, int count);
	int			upload_static_inds(const index_t* inds, int count);

	static d3d_t& get_instance();

private:

	// Used for sorting the render list
	friend int compare_faces(const void* f1, const void* f2);

	int				num_shaders;		// Number of shaders defined
	int				num_passes;			// Number of shader passes defined
	int				num_static_verts;	// Number of static vertices
	int				num_static_inds;	// Number of static vertices

	d3dinfo_t::back_buffer_format_t*	back_buffer_format;
	d3dinfo_t::mode_t*					display_mode;

	shader_t*		shaders;			// Array of shaders
	shader_pass_t*	passes;				// Array of passes

	const char*		get_error_string(HRESULT hr);
	bool			generate_mipmaps(shader_t& texture);
	void			show_tris(display_list_t& dl);

	d3d_t();

	// COM objects, note that d3d MUST be the FIRST object declared here as its
	// destructor has to be the last called to ensure that all of the dependent
	// resources are released before d3d itself.
	d3dinfo_t							info;
	com_ptr_t<IDirect3DDevice8>			d3ddev;
	com_ptr_t<IDirect3DIndexBuffer8>	ibuf;
	com_ptr_t<IDirect3DIndexBuffer8>	sibuf;
	com_ptr_t<IDirect3DVertexBuffer8>	vbuf;
	com_ptr_t<IDirect3DVertexBuffer8>	svbuf;

	D3DPRESENT_PARAMETERS d3dpp;

	void begin_shader(hshader_t shader, htexture_t lightmap);
	void begin_pass(hshader_t shader, htexture_t lightmap, int pass);
	void end_pass(hshader_t shader, htexture_t lightmap, int pass);
	void end_shader(hshader_t shader, htexture_t lightmap);

	int upload_dynamic_verts(const vertex_t* verts, int count);
	int upload_dynamic_inds(const index_t* verts, int count);

	htexture_t	define_texture(const char* name, bool parsing);
	htexture_t	get_texture(const char* name);
	void		load_texture(htexture_t texture);
	void		ensure_loaded(htexture_t texture);

	// Functions for selecting which formats etc will be used
	result_t choose_present_params();
	result_t confirm_device(const d3dinfo_t::device_t& device);
};

#define d3d (d3d_t::get_instance())

#endif
