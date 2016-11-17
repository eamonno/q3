//-----------------------------------------------------------------------------
// File: d3d.cpp
//
// Implementation of d3ddev_t. 
// Shaders vs Textures.
// There are two types of shader, textures and shaders, a shader is a shader
// as defined by a script in a shader file, a texture represents a single
// texture applied to a surface. These are all defined as a single resource
// type the shader however the terms shader and texture are used in the comments
// and the appropriate handle types are used throughout the code
//-----------------------------------------------------------------------------

#include "d3d.h"
#include "console.h"
#include "pak.h"
#include "util.h"
#include "timer.h"
#include "ui.h"
#include <memory>
#include "win.h"
#include "exec.h"
#include "mem.h"
#include <d3dx8.h>

#define new mem_new
#undef d3d

using std::auto_ptr;

cvstr_t
shaders_callback(int argc, cvstr_t* args)
{
	d3d_t::get_instance().list_shaders(0, 0);
	return cvstr_t();
}

cfunc_t cf_shaders("shaders", shaders_callback);

// General Purpose cvars
cvar_int_t showtris("showtris", 0, CVF_NONE, 0, 2);
cvar_int_t display_width("display_width", 640, CVF_CONST);
cvar_int_t display_height("display_height", 480, CVF_CONST);
cvar_int_t display_color_depth("display_color_depth", 32, CVF_CONST);
cvar_int_t display_z_depth("display_z_depth", 24, CVF_CONST);
cvar_int_t display_stencil_depth("display_stencil_depth", 8, CVF_CONST);
cvar_int_t display_back_buffers("display_back_buffers", 2, CVF_CONST, 2, 3);
cvar_int_t display_fullscreen("display_fullscreen", 0, CVF_CONST, 0, 1);
cvar_int_t display_vsync("display_vsync", 0, CVF_CONST, 0, 1);
cvar_int_t d3d_adapter("d3d_adapter", 0, CVF_CONST, 0);
cvar_int_t d3d_device("d3d_device", 0, CVF_CONST, 0, 1);
	// 0 = use the hardware device
	// 1 = use the reference device

// Configuration cvars, highly unlikely to change, changes to these may cause
// some nasty behaviour
cvar_int_t max_shaders("max_shaders", 4096, CVF_CONST);
cvar_int_t max_shader_passes("max_shader_passes", 4096, CVF_CONST);
cvar_int_t max_static_verts("max_static_verts", 60000, CVF_CONST);
cvar_int_t max_static_inds("max_static_inds", 120000, CVF_CONST);
cvar_int_t max_dynamic_verts("max_dynamic_verts", 20000, CVF_CONST);
cvar_int_t max_dynamic_inds("max_dynamic_inds", 20000, CVF_CONST);

// Surface flags
const uint SF_PRECACHE	= 0x01;	// Precache this shader
const uint SF_CACHED	= 0x02;	// Texture is loaded into memory
const uint SF_RETAIN	= 0x04;	// Never unload this shader
const uint SF_NOMIPMAPS	= 0x08;	// No mipmaps for this shader
const uint SF_NOPICMIP	= 0x10;	// Never picmip this shader
const uint SF_CULLFRONT	= 0x20;	// Cull front faces
const uint SF_CULLBACK	= 0x40;	// Cull back faces (default)

namespace {
	// Most textures for an anim map
	const int MAX_PASS_MAPS = 8;
	// Most texture coordinate mods for a pass
	const int MAX_PASS_TCMODS = 3;

	const DWORD FIXED_VERTEX_FORMAT = D3DFVF_XYZ
									| D3DFVF_DIFFUSE
									| D3DFVF_NORMAL
									| D3DFVF_TEX2
									| D3DFVF_TEXCOORDSIZE2(0)
									| D3DFVF_TEXCOORDSIZE2(1);

	// Sort orders
	const int SORT_PORTAL		= 1;
	const int SORT_SKY			= 2;
	const int SORT_OPAQUE		= 3;
	const int SORT_BANNER		= 6;
	const int SORT_UNDERWATER	= 8;
	const int SORT_ADDITIVE		= 9;
	const int SORT_NEAREST		= 16;

	// 0 = null shader, just use 0, no define
	const htexture_t HT_NOSHADER = 1;	// Texture handle to refer to the whiteimage
	const htexture_t HT_WHITEIMAGE = 2;	// Texture handle to refer to the whiteimage
	const htexture_t HT_LIGHTMAP = 3;	// Texture handle to refer to the current face lightmap
	const int NUM_RESERVED_SHADERS = 4;	// Number of shader slots reserved for programatically defined shaders

//	const DWORD vertex_shader_decl[] = {
//		D3DVSD_STREAM(0),
//		D3DVSD_REG(D3DVSDE_POSITION,  D3DVSDT_FLOAT3),
//		D3DVSD_REG(D3DVSDE_NORMAL,    D3DVSDT_FLOAT3),
//		D3DVSD_REG(D3DVSDE_DIFFUSE,   D3DVSDT_D3DCOLOR),
//		D3DVSD_REG(D3DVSDE_TEXCOORD0, D3DVSDT_FLOAT2),
//		D3DVSD_REG(D3DVSDE_TEXCOORD1, D3DVSDT_FLOAT2),
//		D3DVSD_END(),
//	};
//
//	const int VSCONST_CLIP_MATRIX = 4;			// Clip matrix starts at const 4
//
//	const char vertex_shader_src[] = 
//		"vs.1.1                 // Shader version 1.1\n"
//		"m4x4 oPos, v0, c4      // Transform pos\n"
//		"mov oD0, v5            // Vertex Diffuse color\n"
//		"mov oT0.xy, v7         // Tex-coord 0\n"
//		"mov oT1.xy, v8         // Tex-coord 1\n";

	// Pass flags
	const int PF_ALPHABLEND	= 0x01;	// Alpha blending enabled
	const int PF_ALPHATEST	= 0x02;	// Pass requires that alpha test be enabled
	const int PF_CLAMP		= 0x04;	// Clamp texture co-ords for this pass
	const int PF_ENVIRONMENT= 0x08;	// Environment map
	const int PF_NOZWRITE	= 0x10;	// Z write should be disabled for this pass
	const int PF_USETC1		= 0x20;	// Use the lightmap texture co-ords

	inline bool
	shader_name_cmp(const char* name1, const char* name2)
		// Compare two shader names
	{
		const char* n1 = name1;
		const char* n2 = name2;

		// Compare the two without case sensitivity
		while (*n1 && u_tolower(*n1) == u_tolower(*n2)) {
			++n1;
			++n2;
		}
		// If at the end of both they match
		if (*n1 == '\0' && *n2 == '\0')
			return true;

		if (n1 != name1 && *(n1 - 1) == '.') {
			// If name1 ends in .tga and name2 is ended or ends in "jpg" its a match
			if (u_fncmp(n1, "tga") == 0)
				if (*n2 == '\0' || u_fncmp(n2, "jpg") == 0)
					return true;
			// If name1 ends in .tga and name2 is ended or ends in ".jpg" its a match
			if (u_fncmp(n1, "jpg") == 0)
				if (*n2 == '\0' || u_fncmp(n2, "tga") == 0)
					return true;
		} else if (*n1 == '\0') {
			if (u_fncmp(n2, ".jpg") == 0 || u_fncmp(n2, ".tga") == 0)
				return true;
		} else if (*n2 == '\0') {
			if (u_fncmp(n1, ".jpg") == 0 || u_fncmp(n1, ".tga") == 0)
				return true;
		}
		return false;
	}
}

enum wave_type_t {
	WAVE_INVERSESAWTOOTH,
	WAVE_NOISE,
	WAVE_SAWTOOTH,
	WAVE_SIN,
	WAVE_SQUARE,
	WAVE_TRIANGLE,
};

struct wavefunc_t {
	wave_type_t	type;
	float		base;
	float		amplitude;
	float		phase;
	float		frequency;

	float clamp_value(float time) const {
		float pos = time * frequency + phase;
		switch (type) {
		case WAVE_INVERSESAWTOOTH:
			return m_clamp(base + amplitude * m_inversesawtooth_wave(pos));
		case WAVE_SAWTOOTH:
			return m_clamp(base + amplitude * m_sawtooth_wave(pos));
		case WAVE_SIN:
			return m_clamp(base + amplitude * m_sin_wave(pos));
		case WAVE_SQUARE:
			return m_clamp(base + amplitude * m_square_wave(pos));
		case WAVE_TRIANGLE:
			return m_clamp(base + amplitude * m_triangle_wave(pos));
		}
		return 0.5f;	// keep the compiler happy
	}

	float value(float time) const {
		float pos = time * frequency + phase;
		switch (type) {
		case WAVE_INVERSESAWTOOTH:
			return base + amplitude * m_inversesawtooth_wave(pos);
		case WAVE_SAWTOOTH:
			return base + amplitude * m_sawtooth_wave(pos);
		case WAVE_SIN:
			return base + amplitude * m_sin_wave(pos);
		case WAVE_SQUARE:
			return base + amplitude * m_square_wave(pos);
		case WAVE_TRIANGLE:
			return base + amplitude * m_triangle_wave(pos);
		}
		return 0.5f;	// keep the compiler happy
	}
};

enum tcmod_type_t {
	TCMOD_ROTATE,
	TCMOD_SCALE,
	TCMOD_SCROLL,
	TCMOD_STRETCH,
};

enum rgbgen_type_t {
	RGBGEN_IDENTITY,
	RGBGEN_IDENTITYLIGHTING,
	RGBGEN_VERTEX,
	RGBGEN_WAVE,
};

enum alphagen_type_t {
	ALPHAGEN_IDENTITY,
	ALPHAGEN_VERTEX,
	ALPHAGEN_WAVE,
};

struct tcmod_t {
	tcmod_type_t	type;
	union {
		float angle;		// used for rotate
		struct {
			float	x;		// used for scale and scroll
			float	y;		// used for scale and scroll
		};
		wavefunc_t	wave;	// used for stretch
	};
};

enum shader_type_t {
	STYPE_TEXTURE,		// Simple lightmap lit texture
	STYPE_SHADER			// Fully defined shader
};

struct d3d_t::shader_t {
	int			type;			// Shader type
	str_t<64>	name;			// Shader name
	int			flags;			// Shader flags
	int			sort;			// Sort order
	com_ptr_t<IDirect3DTexture8> texture;	// D3D texture, STYPE_TEXTURE only
											// not in union to ensure destructor
	int	num_passes;		// Number of passes for STYPE_SHADER
	union {
		int	first_pass;		// First pass for STYPE_SHADER
		hshader_t	texture;		// Texture index for STYPE_TEXTURE
	};
};

struct d3d_t::shader_pass_t {

	alphagen_type_t	alphagen;
	wavefunc_t		alphagen_wave;
	rgbgen_type_t	rgbgen;
	wavefunc_t		rgbgen_wave;
	int				flags;
	D3DCMPFUNC		alpha_func;
	uint			alpha_ref;
	D3DBLEND		src_blend;
	D3DBLEND		dest_blend;
	D3DCMPFUNC		depth_func;
	float			anim_freq;		// Animation frequency
	int				num_maps;		// Number of texture handles
	htexture_t		maps[MAX_PASS_MAPS];
	int				num_tcmods;
	tcmod_t			tcmods[MAX_PASS_TCMODS];

	htexture_t	map() const
	{ 
		if (num_maps == 0)
			return 0;
		else if (num_maps == 1)
			return maps[0];
		else
			return maps[m_ftoi(timer.time(TID_APP) * anim_freq) % num_maps];
	}
};

d3d_t::d3d_t() : 
	num_shaders(0),
	num_passes(0),
	shaders(0),
	passes(0),
	num_static_verts(0),
	num_static_inds(0)
{ 
	u_zeromem(&d3dpp, sizeof(d3dpp)); 
}

d3d_t&
d3d_t::get_instance()
{
	static std::auto_ptr<d3d_t> instance(new d3d_t());
	return *instance;
}

uint
d3d_t::get_surface_flags(hshader_t shader)
{
	u_assert(shader >= 0 && shader < num_shaders);
	return shaders[shader].flags;
}

result_t
d3d_t::init()
	// Initialises the device, scans for available display modes etc, does not
	// actually set a display mode
{
	shaders = new shader_t[*max_shaders];
	passes = new shader_pass_t[*max_shader_passes];

	// Declare some programatically generated shaders

	// Null shader, just allows use of 0 as a null handle without the need to
	// constantly offset by 1
	shaders[0].name = "<null>";
	shaders[0].type = STYPE_TEXTURE;
	shaders[0].flags = SF_RETAIN;
	

	// Specific noshader shader
	shaders[1].name = "noshader";
	shaders[1].type = STYPE_TEXTURE;
	shaders[1].flags = SF_RETAIN;

	// A plain white texture, generated in create
	shaders[2].name = "$whiteimage";
	shaders[2].type = STYPE_TEXTURE;
	shaders[2].flags = SF_RETAIN;

	// Shader handle set asside for lightmap textures
	shaders[3].name = "$lightmap";
	shaders[3].type = STYPE_TEXTURE;
	shaders[3].flags = SF_RETAIN;

	num_shaders = NUM_RESERVED_SHADERS;

	return info.create();
}

result_t
d3d_t::create()
	// Create the Direct 3d device, returns success or failure
{
	if (!choose_present_params())
		return result_t::last;

	uint behaviour = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	if (back_buffer_format->format->device->caps.DevCaps & D3DDEVCAPS_PUREDEVICE)
		behaviour |= D3DCREATE_PUREDEVICE;	// Create a pure device if available

//	if (chosen_device->caps.VertexShaderVersion < D3DVS_VERSION(1, 1))
//		return "Vertex Shader version 1.1 not supported";

	HRESULT hr;

	hr = info->CreateDevice(
		back_buffer_format->format->device->adapter->ordinal,
		back_buffer_format->format->device->type,
		hwnd, 
		behaviour,
		&d3dpp,
		&d3ddev
	);
	if (FAILED(hr))
		return "IDirect3D8->CreateDevice() failed";

	// Create the dynamic usage index buffer
	hr = d3ddev->CreateIndexBuffer(
		*max_dynamic_inds * sizeof(index_t),
		D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_DEFAULT,
		&ibuf
	);
	if (FAILED(hr))
		return "IDirect3D8->CreateIndexBuffer() failed for dynamic index buffer";

	// Create the dynamic usage vertex buffer
	hr = d3ddev->CreateVertexBuffer(
		*max_dynamic_verts * sizeof(vertex_t),
		D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		FIXED_VERTEX_FORMAT,
		D3DPOOL_DEFAULT,
		&vbuf
	);
	if (FAILED(hr))
		return "IDirect3D8->CreateVertexBuffer() failed for dynamic vertex buffer";

	// Create the static usage index buffer
	d3ddev->CreateIndexBuffer(
		*max_static_inds * sizeof(index_t),
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_DEFAULT,
		&sibuf
	);
	if (FAILED(hr))
		return "IDirect3D8->CreateIndexBuffer() failed for static index buffer";

	// Create the static usage vertex buffer
	d3ddev->CreateVertexBuffer(
		*max_static_verts * sizeof(vertex_t),
		D3DUSAGE_WRITEONLY,
		FIXED_VERTEX_FORMAT,
		D3DPOOL_DEFAULT,
		&svbuf
	);
	if (FAILED(hr))
		return "IDirect3D8->CreateVertexBuffer() failed for static vertex buffer";

	// One time state changes
//	d3ddev->SetRenderState(D3DRS_DITHERENABLE, TRUE);
	d3ddev->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE);
	d3ddev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	d3ddev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	
	// Texture filtering options
	d3ddev->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	d3ddev->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	d3ddev->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
	d3ddev->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	d3ddev->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	d3ddev->SetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);

	d3ddev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
	d3ddev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	d3ddev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	d3ddev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	d3ddev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	d3ddev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

	d3ddev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	d3ddev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	d3ddev->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	d3ddev->SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	d3ddev->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	d3ddev->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

	d3ddev->SetStreamSource(0, vbuf, sizeof(vertex_t));
	d3ddev->SetVertexShader(FIXED_VERTEX_FORMAT);

	// Create $whiteimage here

//	// Compile the vertex shader
//	com_ptr_t<ID3DXBuffer> assembled_shader;
//	com_ptr_t<ID3DXBuffer> assembler_errors;
//
//	HRESULT hr = D3DXAssembleShader(
//		vertex_shader_src,
//		sizeof(vertex_shader_src) - 1,
//		0,
//		0,
//		&assembled_shader,
//		&assembler_errors
//	);
//	if (FAILED(hr)) {
//		console.print("Failed to assemble vertex shader:\n");
//		console.print(static_cast<char*>(assembler_errors->GetBufferPointer()));
//		return "Failed to assemble vertex shader";
//	}
//	hr = d3ddev->CreateVertexShader(
//		vertex_shader_decl,
//		static_cast<DWORD*>(assembled_shader->GetBufferPointer()),
//		&vertex_shader,
//		0
//	);
//	if (FAILED(hr)) {
//		console.print("IDirect3DDevice8->CreateVertexShader() failed:\n");
//		console.printf("%s\n", get_error_string(hr));
//		return "Failed to create vertex shader";
//	}
//	d3ddev->SetVertexShader(vertex_shader);

	float gamma = 0.75f;

	D3DGAMMARAMP ramp;
	for (int i=0 ; i<256 ; i++)
	{
		//int val = pow(m_itof(i), 1.0f / gamma);
		int val = i * (512 - i);
		ramp.red[i] = u_min(val, 65535);
		ramp.green[i] = u_min(val, 65535);
		ramp.blue[i] = u_min(val, 65535);
	}	
	d3ddev->SetGammaRamp(D3DSGR_NO_CALIBRATION, &ramp);

	return true;
}

const char*
d3d_t::get_error_string(HRESULT hr)
{
	const int MAX_ERROR_STRING_LENGTH = 1024;

	static char str[MAX_ERROR_STRING_LENGTH];

	HRESULT hre = D3DXGetErrorString(hr, str, MAX_ERROR_STRING_LENGTH);
	if (FAILED(hre))
		u_snprintf(str, MAX_ERROR_STRING_LENGTH, "D3DXGetErrorString() failed for hresult %#x", hr);
	return str;
}

void
d3d_t::destroy()
	// Free any resources used by the device
{
	delete [] shaders;
	num_shaders = 0;
	shaders = 0;

	delete [] passes;
	num_passes = 0;
	passes = 0;

	if (d3ddev) {
		d3ddev->SetIndices(NULL, 0);
		d3ddev->SetStreamSource(0, NULL, 0);
		d3ddev->SetTexture(0, NULL);
		d3ddev->SetTexture(1, NULL);
	}
	ibuf = 0;
	sibuf = 0;
	vbuf = 0;
	svbuf = 0;
	d3ddev = 0;
	info.destroy();
}

bool
d3d_t::lost()
	// Returns true if the device has been lost, for example if someone uses
	// alt+tab to go to a different window while in full screen mode. If this
	// happens then all video memory resources should be freed and recreated
{
	HRESULT hr = d3ddev->TestCooperativeLevel();
	if (hr == D3DERR_DEVICELOST)
		return true;
	return false;
}

int next_index = 0;
int next_vertex = 0;

bool
d3d_t::begin()
{
//	d3ddev->SetTransform(D3DTS_WORLD, NULL);
//	d3ddev->SetTransform(D3DTS_VIEW, NULL);
//	d3ddev->SetTransform(D3DTS_PROJECTION, NULL);
	next_vertex = 0;
	next_index = 0;
	return SUCCEEDED(d3ddev->BeginScene());
}

void
d3d_t::end()
{
	d3ddev->EndScene();
	d3ddev->Present(NULL, NULL, NULL, NULL); 
}

matrix_t world_matrix;

void
d3d_t::set_camera(const camera_t& camera)
	// Set the camera to be used for upcoming rendering
{

	d3ddev->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX*>(&camera.mat_world));
	d3ddev->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX*>(&camera.mat_view));
	d3ddev->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&camera.mat_proj));

	world_matrix = camera.mat_world;

//	matrix_t clip_mat = camera.mat_world * camera.mat_view * camera.mat_proj	;
//	clip_mat.transpose();
//	d3ddev->SetVertexShaderConstant(VSCONST_CLIP_MATRIX, &clip_mat, 4);

//	d3ddev->SetTransform(D3DTS_TEXTURE0, reinterpret_cast<const D3DXMATRIX*>(&rot));
//	d3ddev->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3 | D3DTTFF_PROJECTED );
}

int
compare_faces(const void* f1, const void* f2)
{
	const face_t* face1 = static_cast<const face_t*>(f1);
	const face_t* face2 = static_cast<const face_t*>(f2);

	const d3d_t& inst = d3d_t::get_instance();

	// Sort by sort order, then shader, then lightmap
	if (inst.shaders[face1->shader].sort != inst.shaders[face2->shader].sort)
		return inst.shaders[face1->shader].sort - inst.shaders[face2->shader].sort;
	else if (face1->shader != face2->shader)
		return face1->shader - face2->shader;
	else
		return face1->lightmap - face2->lightmap;
}

render_stats_t
d3d_t::render_list(display_list_t& dl)
	// Render the face in question
{
	qsort(dl.face_buffer(), dl.num_faces(), sizeof(face_t), compare_faces);

	render_stats_t stats;

	const face_t* face;
	for (const face_t* f = dl.face_buffer(); f != dl.face_buffer_end(); f = (face == f ? f + 1 : face)) {
		face = f;
		begin_shader(f->shader, f->lightmap);
		for (int pass = 0; pass < shaders[f->shader].num_passes; ++pass) {
			begin_pass(f->shader, f->lightmap, pass);
			for (face = f; face->shader == f->shader && face->lightmap == f->lightmap && face != dl.face_buffer_end(); ++face) {
				// Update render stats
				stats += render_stats_t(1, face->num_verts, face->num_inds);

				// Setup the vertex data
				int base_vertex;
				if (face->verts) {
					base_vertex = upload_dynamic_verts(face->verts, face->num_verts);
					d3ddev->SetStreamSource(0, vbuf, sizeof(vertex_t));
				} else {
					d3ddev->SetStreamSource(0, svbuf, sizeof(vertex_t));
					base_vertex = face->base_vert;
				}

				// Setup the index data
				int base_index;
				if (face->inds) {
					base_index = upload_dynamic_inds(face->inds, face->num_inds);
					d3ddev->SetIndices(ibuf, base_vertex);
				} else {
					d3ddev->SetIndices(sibuf, base_vertex);
					base_index = face->base_ind;
				}

				// Now do the drawing
				HRESULT hr = d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, face->num_verts, base_index, face->num_inds / 3);
				u_assert(SUCCEEDED(hr));
			}
			end_pass(f->shader, f->lightmap, pass);
		}
		end_shader(f->shader, f->lightmap);
	}

	if (*showtris)
		show_tris(dl);

	return stats;
}

void
d3d_t::show_tris(display_list_t& dl)
	// Render the face in question
{
	// Set up the render states for showtris
	d3ddev->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	d3ddev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	d3ddev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	if (*showtris == 1) {	// White lines
		d3ddev->SetRenderState(D3DRS_TEXTUREFACTOR, color_t::white);
	} else {	// Blend diffuse colour with white for lines
		d3ddev->SetRenderState(D3DRS_TEXTUREFACTOR, 0x1fffffff);
		d3ddev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
		d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_BLENDFACTORALPHA);	// use diffuse color
	}

	for (const face_t* face = dl.face_buffer(); face != dl.face_buffer_end(); ++face) {
		// Setup the vertex data
		int base_vertex;
		if (face->verts) {
			base_vertex = upload_dynamic_verts(face->verts, face->num_verts);
			d3ddev->SetStreamSource(0, vbuf, sizeof(vertex_t));
		} else {
			d3ddev->SetStreamSource(0, svbuf, sizeof(vertex_t));
			base_vertex = face->base_vert;
		}

		// Setup the index data
		int base_index;
		if (face->inds) {
			base_index = upload_dynamic_inds(face->inds, face->num_inds);
			d3ddev->SetIndices(ibuf, base_vertex);
		} else {
			d3ddev->SetIndices(sibuf, base_vertex);
			base_index = face->base_ind;
		}

		// Now do the drawing
		HRESULT hr = d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, face->num_verts, base_index, face->num_inds / 3);
		u_assert(SUCCEEDED(hr));
	}

	// Reset the render states to defaults
	if (*showtris == 2)
		d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	d3ddev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	d3ddev->SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff);
	d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	d3ddev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	d3ddev->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
}


htexture_t
d3d_t::define_texture(const char* name, bool parsing)
	// Stores a texture name, makes sure the file exists on disk but does NOT
	// load the texture into memory. Parsing says whether this texture is being
	// loaded as part of shader parsing at startup, if so then the error messages
	// are not printed if c_verbose_shader_parsing is false
{
	u_assert(num_shaders < *max_shaders);

	htexture_t ht = get_texture(name);
	if (ht != 0)
		return ht;

	shader_t& shader = shaders[num_shaders];

	// Set the defaults for STYPE_TEXTURE shaders
	shader.type = STYPE_TEXTURE;
	shader.name = name;	// First token is the shader name
	shader.flags = SF_CULLBACK;
	shader.sort = SORT_OPAQUE;
	shader.texture = 0;
	shader.num_passes = 1;
	shader.first_pass = -1;

	if (pak.file_exists(shader.name))
		return num_shaders++;

	int length = shader.name.length();

	if (length < 4)
		return 0;

	// If name ends in ".tga" try with a ".jpg" extension and vice versa
	if (u_fncmp(&shader.name[length - 4], ".tga") == 0) {
		u_strcpy(&shader.name[length - 3], "jpg");
		if (pak.file_exists(shader.name))
			return num_shaders++;
	} else if (u_fncmp(&shader.name[length - 4], ".jpg") == 0) {
		u_strcpy(&shader.name[length - 3], "tga");
		if (pak.file_exists(shader.name))
			return num_shaders++;
	} 

	// That didnt work, try just adding the tga extension
	shader.name += ".tga";
	if (pak.file_exists(shader.name))
		return num_shaders++;

	// That didnt work, try just adding the jpg extension
	shader.name[length] = '\0';
	shader.name += ".jpg";
	if (pak.file_exists(shaders[num_shaders].name))
		return num_shaders++;

	// No such shader, just fail it
	if (!parsing || *verbose_shader_parsing)
		console.printf("Texture not found: %s\n", name);

	return 0;
}

htexture_t
d3d_t::get_texture(const char* name)
	// Return the texture handle of 0 if the texture has not yet been defined. This
	// is an internal method and only returns ST_TEXTURE shader handles
{
	// Check the exact name
	for (int i = 0; i < num_shaders; ++i)
		if ((shaders[i].type == STYPE_TEXTURE) && shader_name_cmp(name, shaders[i].name))
			return i;
	return 0;
}

void
d3d_t::ensure_loaded(htexture_t texture)
{
	if (texture >= NUM_RESERVED_SHADERS && shaders[texture].texture == 0)
		load_texture(texture);
}

hshader_t 
d3d_t::get_shader(const char* name, bool retain)
	// Return the handle to the named shader
{
	hshader_t handle = 0;

	// Search for a shader with that name
	for (int i  = 0; i < num_shaders && handle == 0; ++i)
		if (shaders[i].type == STYPE_SHADER && shader_name_cmp(name, shaders[i].name))
			handle = i;
	
	// Define a texture with that name if no texture found
	if (handle == 0)
		handle = define_texture(name, false);

	if (handle) {
		int flags = retain ? SF_RETAIN | SF_PRECACHE : SF_PRECACHE;
		shader_t& shader = shaders[handle];
		if (!(shader.flags & SF_CACHED)) {
			shader.flags |= flags;
			if (shader.type == STYPE_SHADER) {
				for (int p = 0; p < shader.num_passes; ++p) {
					shader_pass_t& pass = passes[shader.first_pass + p];
					for (int m = 0; m < pass.num_maps; ++m) {
						shader_t& map = shaders[pass.maps[m]];
						if (!(map.flags & SF_CACHED))
							map.flags |= flags;
					}
				}
			}
		}
	} else {
		console.printf("get_shader: shader not found %s\n", name);
	}
	return handle;
}

void
d3d_t::begin_shader(hshader_t shader, htexture_t lightmap)
{
	u_assert(shader < num_shaders);
	u_assert(lightmap < num_shaders);

//	console.printf("begin_shader %s %s\n", shaders[shader].name.c_str(), shaders[lightmap].name.c_str());

	if (shaders[shader].flags & SF_CULLFRONT)
		d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	else if (!(shaders[shader].flags & SF_CULLBACK))
		d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
}

void
d3d_t::begin_pass(hshader_t shader, htexture_t lightmap, int passno)
{
	color_t modulate(color_t::identity);

//	console.printf("    begin_pass %s %s %d\n", shaders[shader].name.c_str(), shaders[lightmap].name.c_str(), passno);

	if (shaders[shader].type == STYPE_SHADER) {
		const shader_pass_t& pass = passes[shaders[shader].first_pass + passno];

		if (pass.flags & PF_ALPHABLEND) {
			d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			d3ddev->SetRenderState(D3DRS_SRCBLEND, pass.src_blend);
			d3ddev->SetRenderState(D3DRS_DESTBLEND, pass.dest_blend);
		}

		if (pass.flags & PF_ALPHATEST) {
			d3ddev->SetRenderState(D3DRS_ALPHAFUNC, pass.alpha_func);
			d3ddev->SetRenderState(D3DRS_ALPHAREF, pass.alpha_ref);
			d3ddev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
		}

		if (pass.flags & PF_CLAMP) {
			d3ddev->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
			d3ddev->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
		}

		if (pass.flags & PF_NOZWRITE)
			d3ddev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

		if (pass.flags & PF_USETC1)
			d3ddev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

		if (pass.depth_func != D3DCMP_LESSEQUAL)
			d3ddev->SetRenderState(D3DRS_ZFUNC, pass.depth_func);

		if (pass.alphagen != ALPHAGEN_IDENTITY) {
			d3ddev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			if (pass.alphagen == ALPHAGEN_WAVE) {
				d3ddev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
				modulate.set_a(pass.alphagen_wave.clamp_value(timer.time(TID_APP)));
			}
		}

		if (pass.rgbgen != RGBGEN_IDENTITY) {
			d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			if (pass.rgbgen == RGBGEN_WAVE) {
				d3ddev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
				modulate.set_r(pass.rgbgen_wave.clamp_value(timer.time(TID_APP)));
				modulate.g = modulate.r;
				modulate.b = modulate.r;
			}
			if (pass.rgbgen == RGBGEN_IDENTITYLIGHTING) {
				d3ddev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
				modulate.r = 128;
				modulate.g = modulate.r;
				modulate.b = modulate.r;
			}
		}
		
		if (pass.rgbgen == RGBGEN_WAVE || pass.rgbgen == RGBGEN_IDENTITYLIGHTING || pass.alphagen == ALPHAGEN_WAVE)
			d3ddev->SetRenderState(D3DRS_TEXTUREFACTOR, modulate);

		matrix_t mat(matrix_t::identity);
		if (pass.num_tcmods) {
			for (int i = 0; i < pass.num_tcmods; ++i) {
				matrix_t temp(matrix_t::identity);
				float magnitude;
				matrix_t m1(matrix_t::identity), m2(matrix_t::identity), m3(matrix_t::identity);
				switch (pass.tcmods[i].type) {
				case TCMOD_ROTATE:
					// translate(-0.5, -0.5) * rotate(angle) * translate(0.5, 0.5)
					temp._00 = m_cos(m_deg2rad(pass.tcmods[i].angle * timer.time(TID_APP)));
					temp._01 = m_sin(m_deg2rad(pass.tcmods[i].angle * timer.time(TID_APP)));
					temp._10 = -temp._01;
					temp._11 = temp._00;
					temp._20 = 0.5f * (temp._01 - temp._00 + 1.0f);
					temp._21 = 0.5f * (temp._10 - temp._00 + 1.0f);
					break;
				case TCMOD_SCALE:
					temp._00 = pass.tcmods[i].x;
					temp._11 = pass.tcmods[i].y;
//					temp._20 = (pass.tcmods[i].x + 1.0f) * -0.5f;
//					temp._21 = (pass.tcmods[i].y + 1.0f) * -0.5f;
					break;
				case TCMOD_SCROLL:
					temp._20 = pass.tcmods[i].x * timer.time(TID_APP);
					temp._21 = pass.tcmods[i].y * timer.time(TID_APP);
					break;
				case TCMOD_STRETCH:
					magnitude = pass.tcmods[i].wave.value(timer.time(TID_APP));
					m1._20 = -0.5;
					m1._21 = -0.5;
					m2._00 = magnitude;
					m2._11 = magnitude;
					m3._20 = 0.5;
					m3._21 = 0.5;
					temp = m1 * m2 * m3;
					break;
				}
				mat *= temp;
			}
			d3ddev->SetTransform(D3DTS_TEXTURE0, reinterpret_cast<const D3DXMATRIX*>(&mat));
			d3ddev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
		} 
		if (pass.flags & PF_ENVIRONMENT) {
			mat._00 = 0.5f;
			mat._01 = 0.0f;
			mat._10 = 0.0f;
			mat._11 = 0.5f;
			mat[2] = vec4_t(0.0f, 0.0f, 1.0f, 0.0f);
			mat[3] = vec4_t(0.0f, 0.0f, 0.0f, 1.0f);
			d3ddev->SetTransform(D3DTS_TEXTURE0, reinterpret_cast<const D3DXMATRIX*>(&(mat)));
			d3ddev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
			d3ddev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
		} 

		// Set the texture
		htexture_t map = pass.map();
		ensure_loaded(map);
		if (map == HT_LIGHTMAP)
			d3ddev->SetTexture(0, shaders[lightmap].texture);
		else
			d3ddev->SetTexture(0, shaders[map].texture);

	} else {	// (shaders[shader].type == STYPE_TEXTURE)
		// The one and only pass for textures
		ensure_loaded(shader);
		d3ddev->SetTexture(0, shaders[shader].texture);
		if (lightmap) {
			d3ddev->SetTexture(1, shaders[lightmap].texture);
			d3ddev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
		}
	}
}

void
d3d_t::end_pass(hshader_t shader, htexture_t lightmap, int passno)
{
//	console.printf("    end_pass %s %s %d\n", shaders[shader].name.c_str(), shaders[lightmap].name.c_str(), passno);

	if (shaders[shader].type == STYPE_SHADER) {
		const shader_pass_t& pass = passes[shaders[shader].first_pass + passno];

		if (pass.flags & PF_ALPHABLEND)
			d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		if (pass.flags & PF_ALPHATEST)
			d3ddev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

		if (pass.flags & PF_CLAMP) {
			d3ddev->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
			d3ddev->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
		}

		if (pass.flags & PF_ENVIRONMENT) {
			d3ddev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
			d3ddev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
		}

		if (pass.flags & PF_NOZWRITE)
			d3ddev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

		if (pass.flags & PF_USETC1)
			d3ddev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);

		if (pass.alphagen != ALPHAGEN_IDENTITY) {
			d3ddev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
			if (pass.alphagen == ALPHAGEN_WAVE)
				d3ddev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		}

		if (pass.rgbgen != RGBGEN_IDENTITY) {
			d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
			if (pass.rgbgen == RGBGEN_WAVE || pass.rgbgen == RGBGEN_IDENTITYLIGHTING)
				d3ddev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		}

		if (pass.depth_func != D3DCMP_LESSEQUAL)
			d3ddev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);

		if (pass.num_tcmods)
			d3ddev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);

	} else { // (shaders[shader].type == STYPE_TEXTURE)
		if (lightmap)
			d3ddev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	}
}

void
d3d_t::end_shader(hshader_t shader, htexture_t lightmap)
{
//	console.printf("end_shader %s %s\n", shaders[shader].name.c_str(), shaders[lightmap].name.c_str());

	if ((shaders[shader].flags & SF_CULLFRONT) || !(shaders[shader].flags & SF_CULLBACK))
		d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}

//#include "entity.h"
//extern player_t player;

bool
d3d_t::generate_mipmaps(shader_t& texture)
{
	HRESULT hr = D3DXFilterTexture(
		texture.texture,
		NULL,
		D3DX_DEFAULT,
		D3DX_FILTER_TRIANGLE
	);
	if (FAILED(hr))
		console.printf("generate_mipmaps failed for %s\n", texture.name);
	return SUCCEEDED(hr);
}

int
d3d_t::resources_to_load()
{
	// Return a count of the unloaded STYPE_TEXTURE shaders
	int count = 0;
	for (int i = 4; i < num_shaders; ++i)
		if (shaders[i].type == STYPE_TEXTURE && (shaders[i].flags & SF_PRECACHE))
			++count;
	return count;
}

void
d3d_t::load_resource()
{
	// Find the first unloaded STYPE_TEXTURE
	htexture_t texture;
	for (texture = 4; texture < num_shaders; ++texture)
		if (shaders[texture].type == STYPE_TEXTURE && (shaders[texture].flags & SF_PRECACHE))
			break;
	u_assert(texture < num_shaders);

	// Now load that texture
	load_texture(texture);
}

void
d3d_t::free_resources()
{
	for (int i = 0; i < num_shaders; ++i) {
		if (!(shaders[i].flags & SF_RETAIN)) {
			shaders[i].texture = 0;
			shaders[i].flags &= ~(SF_CACHED | SF_PRECACHE);
		}
	}
	num_static_verts = 0;
	num_static_inds = 0;
}

void
d3d_t::load_texture(htexture_t texture)
	// Loads a texture and returns a reference to it. The newly loaded texture is added to
	// the textures map before returning
{
	u_assert(shaders[texture].texture == 0);

	console.printf("loading texture: %s ... ", shaders[texture].name.c_str());
	auto_ptr<file_t> file(pak.open_file(shaders[texture].name));
	if (file.get()) {
		HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(
			d3ddev, file->data(), file->size(), D3DX_DEFAULT, D3DX_DEFAULT,
			((shaders[texture].flags & SF_NOMIPMAPS) ? 1 : D3DX_DEFAULT), 0, back_buffer_format->back_buffer_format,
			D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &shaders[texture].texture);
		if (FAILED(hr))
			console.printf("failed, %s\n", get_error_string(hr));
		else
			console.print("ok\n");
		shaders[texture].flags &= ~SF_PRECACHE;	// Dont try to load this texture again
		shaders[texture].flags |= SF_CACHED;	// Texture is loaded
	} else {
		console.print("failed, file not found\n");
	}
}

///////////////////////////////////////////////////////////////////////////////
// RESOURCE UPLOADING FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

int
d3d_t::upload_dynamic_verts(const vertex_t* verts, int count)
	// Upload vertices into the dynamic vertex buffer
{
	static int next_vertex = 0;

	vertex_t* ver;
	if (next_vertex + count <= *max_dynamic_verts) {
		vbuf->Lock(
			next_vertex * sizeof(vertex_t),
			count * sizeof(vertex_t),
			reinterpret_cast<ubyte**>(&ver),
			D3DLOCK_NOOVERWRITE
		);
	} else {
		vbuf->Lock(0, 0, reinterpret_cast<ubyte**>(&ver), D3DLOCK_DISCARD);
		next_vertex = 0;
	}
	u_memcpy(ver, verts, count * sizeof(vertex_t));
	vbuf->Unlock();

	int vertex = next_vertex;
	next_vertex += count;

	return vertex;
}

int
d3d_t::upload_dynamic_inds(const index_t* inds, int count)
	// Copy data into the dynamic index buffer, discarding if nescessary
{
	static int next_index = 0;

	index_t* ind;
	if (next_index + count <= *max_dynamic_inds) {
		ibuf->Lock(
			next_index * sizeof(index_t),
			count * sizeof(index_t),
			reinterpret_cast<ubyte**>(&ind),
			D3DLOCK_NOOVERWRITE
		);
	} else {
		ibuf->Lock(0, 0, reinterpret_cast<ubyte**>(&ind), D3DLOCK_DISCARD);
		next_index = 0;
	}
	u_memcpy(ind, inds, count * sizeof(index_t));
	ibuf->Unlock();

	int index = next_index;
	next_index += count;

	return index;
}

int
d3d_t::upload_static_verts(const vertex_t* verts, int count)
	// Upload vertices into the static vertex buffer
{
	console.debugf("uploading %d static vertices for total %d static vertices\n", count, count + num_static_verts);

	vertex_t* ver;
	if (num_static_verts + count <= *max_static_verts) {
		svbuf->Lock(
			num_static_verts * sizeof(vertex_t),
			count * sizeof(vertex_t),
			reinterpret_cast<ubyte**>(&ver), 
			0
		);
		u_memcpy(ver, verts, count * sizeof(vertex_t));
		svbuf->Unlock();
		num_static_verts += count;
		return num_static_verts - count;
	}
	return -1;
}

int
d3d_t::upload_static_inds(const index_t* inds, int count)
	// Upload index data into the static index buffer
{
	console.debugf("uploading %d static indices for total %d static indices\n", count, count + num_static_inds);

	index_t* ind;
	if (num_static_inds + count <= *max_static_inds) {
		sibuf->Lock(
			num_static_inds * sizeof(index_t),
			count * sizeof(index_t),
			reinterpret_cast<ubyte**>(&ind), 
			0
		);
		for (int i = 0; i < count; ++i)
			ind[i] = inds[i];
		sibuf->Unlock();
		num_static_inds += count;
		return num_static_inds - count;
	}
	return -1;
}

htexture_t
d3d_t::upload_lightmap_rgb(const ubyte* data, const int width, const int height, const char* name)
	// Upload a lightmap, lightmaps are arrays of 128 x 128 x 3 unsigned bytes
{
	// Quake 3 uses overbright/gamma in fullscreen to brighten up the world, since this
	// is impossible in windowed mode, artificially inflate the lightmap values to
	// get a similar effect
	// lm_lookup[i] = floor(256 - (1.0 - (i / 255))^2 * 223
	const int lm_lookup[] = {
		 32,  34,  36,  38,  39,  41,  43,  45,  46,  48,  50,  51,  53,  55,  56,  58,
		 60,  61,  63,  64,  66,  68,  69,  71,  72,  74,  76,  77,  79,  80,  82,  83,
		 85,  86,  88,  90,  91,  93,  94,  95,  97,  98, 100, 101, 103, 104, 106, 107,
		109, 110, 111, 113, 114, 116, 117, 118, 120, 121, 122, 124, 125, 126, 128, 129,
		130, 132, 133, 134, 136, 137, 138, 139, 141, 142, 143, 144, 146, 147, 148, 149,
		150, 152, 153, 154, 155, 156, 158, 159, 160, 161, 162, 163, 164, 165, 167, 168,
		169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184,
		185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 198, 199,
		200, 201, 202, 203, 204, 204, 205, 206, 207, 208, 209, 209, 210, 211, 212, 212,
		213, 214, 215, 215, 216, 217, 218, 218, 219, 220, 221, 221, 222, 223, 223, 224,
		225, 225, 226, 226, 227, 228, 228, 229, 230, 230, 231, 231, 232, 232, 233, 234,
		234, 235, 235, 236, 236, 237, 237, 238, 238, 239, 239, 240, 240, 241, 241, 241,
		242, 242, 243, 243, 244, 244, 244, 245, 245, 245, 246, 246, 247, 247, 247, 248,
		248, 248, 249, 249, 249, 249, 250, 250, 250, 251, 251, 251, 251, 252, 252, 252,
		252, 252, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254, 254, 254, 254, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	};

	// Check if a texture with this name exists
	htexture_t handle = get_texture(name);
	if (handle == 0) {
		u_assert(num_shaders < *max_shaders);
		handle = num_shaders++;
	}

	// Set up the texture for use, same setup as a plain texture
	shader_t& texture = shaders[handle];
	texture.type = STYPE_TEXTURE;
	texture.name = name;
	texture.flags = SF_CULLBACK;
	texture.sort = SORT_OPAQUE;
	texture.texture = 0;
	texture.num_passes = 1;
	texture.first_pass = -1;

	HRESULT hr = d3ddev->CreateTexture(width, height, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture.texture);
	if (SUCCEEDED(hr)) {
		D3DSURFACE_DESC desc;

		if (SUCCEEDED(hr = texture.texture->GetLevelDesc(0, &desc))) {
			D3DLOCKED_RECT rect;
			if (SUCCEEDED(hr = texture.texture->LockRect(0, &rect, 0, 0))) {
				ubyte* dstbuffer = static_cast<ubyte*>(rect.pBits);
				for (int i = 0; i < width; ++i) {
					ulong* dest = reinterpret_cast<ulong*>(dstbuffer);
					for (int j = 0; j < height; ++j) {
						ubyte r = *data++;
						ubyte g = *data++;
						ubyte b = *data++;
						ubyte a;
						if (*display_fullscreen == 0 || !(back_buffer_format->format->device->caps.Caps2 & D3DCAPS2_FULLSCREENGAMMA)) {
							// Brighten the lightmaps if we cant control gamma
							r = lm_lookup[r];
							g = lm_lookup[g];
							b = lm_lookup[b];
							a = (r + g + b) / 3;
						}
						*dest++ = ((a << 24) + (r << 16) + (g << 8) + b);
					}
					dstbuffer += rect.Pitch;
				}
				console.printf("%s successfully uploaded\n", name);
				texture.texture->UnlockRect(0);
				generate_mipmaps(texture);
			} else {
				console.printf("IDirect3DTexture8->LockRect() failed, hr = %#x\n", hr);
			}
		} else {
			console.printf("IDirect3DTexture8->GetLevelDesc() failed, hr = %#x\n", hr);
		}
	} else {
		console.printf("IDirect3DDevice8->CreateTexture() falied: hr = %#x\n", hr);
	}

	return handle;
}

void
d3d_t::upload_shader(const token_t* tokens)
	// Convert a token stream into the internal representation of a shader
{
	u_assert(num_shaders != *max_shaders);

	// Save num shaders, if the parsing fails this will have to be restored
	int save_num_shaders = num_shaders;
	int save_num_passes = num_passes;

	if (*verbose_shader_parsing) {
		console.printf(DIVIDER);
		console.printf("upload_shader called for shader %s\n", tokens[0].string_value, num_shaders);
	}

	// Take the next shader from the list
	shader_t& shader = shaders[num_shaders++];

	// Set the defaults for STYPE_SHADER shaders
	shader.type = STYPE_SHADER;
	shader.name = tokens[0].string_value;	// First token is the shader name
	shader.flags = SF_CULLBACK;
	shader.sort = SORT_OPAQUE;
	shader.texture = 0;
	shader.num_passes = 0;
	shader.first_pass = num_passes;

	bool fail = false;

	// Skip token 2, its always SK_LEFT_BRACE
	for (const token_t* t = tokens + 2; t < (tokens + tokens->remaining) && !fail; t += t->remaining) {
		u_assert(num_passes + shader.num_passes < *max_shader_passes);

		shader_pass_t& pass = passes[num_passes + shader.num_passes];
		pass.rgbgen = RGBGEN_IDENTITY;
		pass.alphagen = ALPHAGEN_IDENTITY;
		pass.flags = 0;
		pass.num_maps = 0;
		pass.num_tcmods = 0;
		pass.anim_freq = 0.0f;
		pass.alpha_func = D3DCMP_ALWAYS;
		pass.alpha_ref = 0;
		pass.src_blend = D3DBLEND_ONE;
		pass.dest_blend = D3DBLEND_ZERO;
		pass.depth_func = D3DCMP_LESSEQUAL;

		// t now points to the next element in the shader body
		switch (t->type) {
		case SK_LEFT_BRACE:	// Start of a shader stage
			for (++t; t->type != SK_RIGHT_BRACE; t += t->remaining) {
				switch (t->type) {
				case SK_ALPHAFUNC:	// SE_ALPHAFUNC
					switch (t[1].type) {
					case SK_GE128:
						pass.alpha_func = D3DCMP_GREATEREQUAL;
						pass.alpha_ref  = 128;
						break;
					case SK_GT0:
						pass.alpha_func = D3DCMP_GREATER;
						pass.alpha_ref  = 0;
						break;
					case SK_LT128:
						pass.alpha_func = D3DCMP_LESS;
						pass.alpha_ref  = 128;
					}
					pass.rgbgen = RGBGEN_IDENTITY;
					pass.flags |= PF_ALPHATEST;
					pass.flags |= PF_NOZWRITE;
					if (shader.num_passes == 0)
						shader.sort = SORT_ADDITIVE;
					break;
				case SK_ALPHAGEN:	// SE_ALPHAGEN
					switch (t[1].type) {
					case SK_VERTEX:
						pass.alphagen = ALPHAGEN_VERTEX;
						break;
					case SK_ENTITY:
					case SK_LIGHTINGSPECULAR:
						break;
					case SK_WAVE:
						pass.alphagen = ALPHAGEN_WAVE;
						switch (t[2].type) {
						case SK_INVERSESAWTOOTH:
							pass.alphagen_wave.type = WAVE_INVERSESAWTOOTH;
							break;
						case SK_NOISE:
							pass.alphagen_wave.type = WAVE_NOISE;
							break;
						case SK_SAWTOOTH:
							pass.alphagen_wave.type = WAVE_SAWTOOTH;
							break;
						case SK_SIN:
							pass.alphagen_wave.type = WAVE_SIN;
							break;
						case SK_SQUARE:
							pass.alphagen_wave.type = WAVE_SQUARE;
							break;
						case SK_TRIANGLE:
							pass.alphagen_wave.type = WAVE_TRIANGLE;
						}
						pass.alphagen_wave.base		 = t[3].float_value;
						pass.alphagen_wave.amplitude = t[4].float_value;
						pass.alphagen_wave.phase	 = t[5].float_value;
						pass.alphagen_wave.frequency = t[6].float_value;
					}
					break;
				case SK_ANIMMAP:	// SL_FLOAT, SE_ANIMMAP
					pass.anim_freq = t[1].float_value;
					t += 2;
					// Yes, the -1 index here is correct and safe to use
					while (t[-1].remaining != 1 && pass.num_maps < MAX_PASS_MAPS) {
						pass.maps[pass.num_maps] = define_texture(t->string_value, true);
						if (pass.maps[pass.num_maps++] == 0)
							fail = true;
						++t;
					}
					--t;
					break;
				case SK_BLENDFUNC:	// SE_BLENDFUNC
					if (t->remaining == 3) {	// SE_SRCBLEND, SE_DESTBLEND
						switch (t[1].type) {
						case SK_GL_DST_COLOR:
							pass.src_blend = D3DBLEND_DESTCOLOR;
							break;
						case SK_GL_ONE:
							pass.src_blend = D3DBLEND_ONE;
							break;
						case SK_GL_ONE_MINUS_DST_COLOR:
							pass.src_blend = D3DBLEND_INVDESTCOLOR;
							break;
						case SK_GL_ONE_MINUS_SRC_ALPHA:
							pass.src_blend = D3DBLEND_INVSRCALPHA;
							break;
						case SK_GL_SRC_ALPHA:
							pass.src_blend = D3DBLEND_SRCALPHA;
							break;
						case SK_GL_ZERO:
							pass.src_blend = D3DBLEND_ZERO;
						}
						switch (t[2].type) {
						case SK_GL_SRC_COLOR:
							pass.dest_blend = D3DBLEND_SRCCOLOR;
							break;
						case SK_GL_ONE:
							pass.dest_blend = D3DBLEND_ONE;
							break;
						case SK_GL_ONE_MINUS_DST_ALPHA:
							pass.dest_blend = D3DBLEND_INVDESTALPHA;
							break;
						case SK_GL_ONE_MINUS_SRC_ALPHA:
							pass.dest_blend = D3DBLEND_INVSRCALPHA;
							break;
						case SK_GL_ONE_MINUS_SRC_COLOR:
							pass.dest_blend = D3DBLEND_INVSRCCOLOR;
							break;
						case SK_GL_SRC_ALPHA:
							pass.dest_blend = D3DBLEND_SRCALPHA;
							break;
						case SK_GL_ZERO:
							pass.dest_blend = D3DBLEND_ZERO;
						}
					} else {	// SE_BLENDFUNC
						switch (t[1].type) {
						case SK_ADD:
							pass.src_blend = D3DBLEND_ONE;
							pass.dest_blend = D3DBLEND_ONE;
							break;
						case SK_BLEND:
							pass.src_blend = D3DBLEND_SRCALPHA;
							pass.dest_blend = D3DBLEND_INVSRCALPHA;
							break;
						case SK_FILTER:
							pass.src_blend = D3DBLEND_DESTCOLOR;
							pass.dest_blend = D3DBLEND_ZERO;
						}
					}
					if ((pass.src_blend != D3DBLEND_ONE) || (pass.dest_blend != D3DBLEND_ZERO)) {
						pass.flags |= (PF_ALPHABLEND | PF_NOZWRITE);
						if (shader.num_passes == 0)
							shader.sort = SORT_ADDITIVE;
					}
					break;
				case SK_CLAMPMAP:	// SE_MAPNAME
					pass.maps[0] = define_texture(t[1].string_value, true);
					if (pass.maps[0] == 0)
						fail = true;
					pass.num_maps = 1;
					pass.flags |= PF_CLAMP;
					if (pass.maps[0] == HT_LIGHTMAP)
						pass.flags |= PF_USETC1;
					break;
				case SK_DEPTHFUNC:	// SE_DEPTHFUNC
					switch (t[1].type) {
					case SK_EQUAL:
						pass.depth_func = D3DCMP_EQUAL;
						break;
					case SK_LEQUAL:
						pass.depth_func = D3DCMP_LESSEQUAL;
						break;
					}
					break;
				case SK_DEPTHWRITE:	//
					pass.flags &= ~PF_NOZWRITE;
					break;
				case SK_MAP:		// SE_MAPNAME
					pass.maps[0] = define_texture(t[1].string_value, true);
					if (pass.maps[0] == 0)
						fail = true;
					pass.num_maps = 1;
					if (pass.maps[0] == HT_LIGHTMAP)
						pass.flags |= PF_USETC1;
					break;
				case SK_RGBGEN:		// SE_RGBGEN
					switch (t[1].type) {
					case SK_ENTITY:
						break;
					case SK_IDENTITY:
						pass.rgbgen = RGBGEN_IDENTITY;
						break;
					case SK_IDENTITYLIGHTING:
						pass.rgbgen = RGBGEN_IDENTITYLIGHTING;
						break;
					case SK_LIGHTINGDIFFUSE:
						break;
					case SK_EXACTVERTEX:
					case SK_VERTEX:
						pass.rgbgen = RGBGEN_VERTEX;
//						pass->tstates.add_activate_state(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
//						pass->tstates.add_deactivate_state(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
						break;
					case SK_WAVE:
						pass.rgbgen = RGBGEN_WAVE;
						switch (t[2].type) {
						case SK_INVERSESAWTOOTH:
							pass.rgbgen_wave.type = WAVE_INVERSESAWTOOTH;
							break;
						case SK_NOISE:
							pass.rgbgen_wave.type = WAVE_NOISE;
							break;
						case SK_SAWTOOTH:
							pass.rgbgen_wave.type = WAVE_SAWTOOTH;
							break;
						case SK_SIN:
							pass.rgbgen_wave.type = WAVE_SIN;
							break;
						case SK_SQUARE:
							pass.rgbgen_wave.type = WAVE_SQUARE;
							break;
						case SK_TRIANGLE:
							pass.rgbgen_wave.type = WAVE_TRIANGLE;
						}
						pass.rgbgen_wave.base		= t[3].float_value;
						pass.rgbgen_wave.amplitude	= t[4].float_value;
						pass.rgbgen_wave.phase		= t[5].float_value;
						pass.rgbgen_wave.frequency	= t[6].float_value;
						break;
					}
					break;
				case SK_TCGEN:		// SE_TCGEN
					switch (t[1].type) {
					case SK_BASE:
						pass.flags &= ~(PF_ENVIRONMENT);
						break;
					case SK_ENVIRONMENT:
						pass.flags |= PF_ENVIRONMENT;
						break;
					case SK_LIGHTMAP:
						pass.flags |= PF_USETC1;
					}
					break;
				case SK_TCMOD:		// SE_TCMOD
					switch (t[1].type) {
					case SK_ROTATE:		// SL_FLOAT
						u_assert(pass.num_tcmods != MAX_PASS_TCMODS);
						pass.tcmods[pass.num_tcmods].type = TCMOD_ROTATE;
						pass.tcmods[pass.num_tcmods].angle= t[2].float_value;
						++pass.num_tcmods;
						break;
					case SK_SCALE:		// SL_FLOAT, SL_FLOAT
						u_assert(pass.num_tcmods != MAX_PASS_TCMODS);
						pass.tcmods[pass.num_tcmods].type = TCMOD_SCALE;
						pass.tcmods[pass.num_tcmods].x	= t[2].float_value;
						pass.tcmods[pass.num_tcmods].y	= t[3].float_value;
						++pass.num_tcmods;
						break;
					case SK_SCROLL:		// SL_FLOAT, SL_FLOAT
						u_assert(pass.num_tcmods != MAX_PASS_TCMODS);
						pass.tcmods[pass.num_tcmods].type = TCMOD_SCROLL;
						pass.tcmods[pass.num_tcmods].x	= t[2].float_value;
						pass.tcmods[pass.num_tcmods].y	= t[3].float_value;
						++pass.num_tcmods;
						break;
					case SK_STRETCH:	// SE_WAVEFUNC
						u_assert(pass.num_tcmods != MAX_PASS_TCMODS);
						pass.tcmods[pass.num_tcmods].type = TCMOD_STRETCH;
						switch (t[2].type) {
						case SK_INVERSESAWTOOTH:
							pass.tcmods[pass.num_tcmods].wave.type = WAVE_INVERSESAWTOOTH;
							break;
						case SK_NOISE:
							pass.tcmods[pass.num_tcmods].wave.type = WAVE_NOISE;
							break;
						case SK_SAWTOOTH:
							pass.tcmods[pass.num_tcmods].wave.type = WAVE_SAWTOOTH;
							break;
						case SK_SIN:
							pass.tcmods[pass.num_tcmods].wave.type = WAVE_SIN;
							break;
						case SK_SQUARE:
							pass.tcmods[pass.num_tcmods].wave.type = WAVE_SQUARE;
							break;
						case SK_TRIANGLE:
							pass.tcmods[pass.num_tcmods].wave.type = WAVE_TRIANGLE;
						}
						pass.tcmods[pass.num_tcmods].wave.base	  = t[3].float_value;
						pass.tcmods[pass.num_tcmods].wave.amplitude = t[4].float_value;
						pass.tcmods[pass.num_tcmods].wave.phase	  = t[5].float_value;
						pass.tcmods[pass.num_tcmods].wave.frequency = t[6].float_value;
						++pass.num_tcmods;
						break;
					case SK_TRANSFORM:	// SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT
						break;
					case SK_TURB:		// SE_TURB
						break;
					}
					break;
				}
			}
			if (shader.num_passes == 0)
				shader.first_pass = num_passes;
			++shader.num_passes;
			break;
		case SK_CULL:					// SE_CULL
			switch (t[1].type) {
			case SK_BACK:
				shader.flags &= ~(SF_CULLFRONT);	// turn off front culling
				shader.flags |= SF_CULLBACK;
				break;
			case SK_DISABLE:
			case SK_NONE:
			case SK_TWOSIDED:
				shader.flags &= ~(SF_CULLBACK | SF_CULLFRONT);
				break;
			case SK_FRONT:
				shader.flags &= ~(SF_CULLBACK);	// turn off the default
				shader.flags |= SF_CULLFRONT;
				break;
			};
			break;
		case SK_DEFORMVERTEXES:			// SE_DEFORMVERTEXES
			break;
		case SK_FOGPARMS:				// SE_FOGPARMS
			break;
		case SK_LIGHT:					// SL_FLOAT
			break;
		case SK_NOMIPMAPS:
			shader.flags |= SF_NOMIPMAPS;
			break;
		case SK_NOPICMIP:
			shader.flags |= SF_NOPICMIP;
			break;
		case SK_Q3MAP_BACKSHADER:		// SL_STRING
			break;
		case SK_Q3MAP_GLOBALTEXTURE:	//
			break;
		case SK_Q3MAP_LIGHTIMAGE:		// SL_STRING
			break;
		case SK_Q3MAP_LIGHTSUBDIVIDE:	// SL_FLOAT
			break;
		case SK_Q3MAP_SUN:				// SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT
			break;
		case SK_Q3MAP_SURFACELIGHT:		// SL_FLOAT
			break;
		case SK_Q3MAP_TESSSIZE:			// SL_FLOAT
			break;
		case SK_QER_EDITORIMAGE:		// SL_STRING
			break;
		case SK_QER_NOCARVE:			//
			break;
		case SK_QER_TRANS:				// SL_FLOAT
			break;
		case SK_SKYPARMS:				// SE_SKYBOX, SE_CLOUDHEIGHT, SE_SKYBOX
			shader.flags &= ~(SF_CULLFRONT | SF_CULLBACK);
			break;
		case SK_SORT:					// SE_SORT
			switch (t[1].type) {
			case SK_PORTAL:
				shader.sort = SORT_PORTAL;
				break;
			case SK_SKY:
				shader.sort = SORT_SKY;
				break;
			case SK_OPAQUE:
				shader.sort = SORT_OPAQUE;
				break;
			case SK_BANNER:
				shader.sort = SORT_BANNER;
				break;
			case SK_UNDERWATER:
				shader.sort = SORT_UNDERWATER;
				break;
			case SK_ADDITIVE:
				shader.sort = SORT_ADDITIVE;
				break;
			case SK_NEAREST:
				shader.sort = SORT_NEAREST;
				break;
			case SL_FLOAT:
				shader.sort = m_floor(t[1].float_value + 0.5f);
				break;
			}
			break;
		case SK_SURFACEPARM:			// SE_SURFACEPARM
			break;
		}
	}

	if (fail) {
		num_shaders = save_num_shaders;
		num_passes = save_num_passes;
	} else {
		num_passes += shader.num_passes;
	}

	if (*verbose_shader_parsing) {
		if (!fail)
			list_shaders(save_num_shaders, num_shaders - save_num_shaders);
		console.printf("upload_shader %s - %d shaders exist:\n", fail ? "failed" : "succeeded", num_shaders);
	}
}


void
d3d_t::list_shaders(uint first, uint num)
	// list num shaders in the list, starting at first, if num is 0, then list all
{
	if (first >= num_shaders)
		console.printf("d3d_t::list_shaders: first shader index too big");

	uint last = num ? first + num : num_shaders;
	if (last > num_shaders) {
		console.print("d3d_t::list_shaders: number to list too big - truncating\n");
		last = num_shaders;
	}
	for (uint i = first; i < last; ++i)
		console.printf("%4d %c %s\n", i,
			shaders[i].type == STYPE_SHADER ? 's' : 't',
			shaders[i].name.c_str());
	console.printf("Total: %d shaders, Listed: %d shaders\n", num_shaders, last - first);
}

result_t
d3d_t::choose_present_params()
	// Choose which adapter will be used
{
	// Select an adapter (select whatever adapter c_d3d_adapter wants)
	d3dinfo_t::adapter_t* adapter = 0;
	if (info.num_adapters > *d3d_adapter)
		adapter = &info.adapters[*d3d_adapter];
	else 
		return "Adapter specified in d3d_use_adapter not found";
	console.printf(
		"Direct3D: Using adapter%d - %s (%s)\n",
		*d3d_adapter,
		adapter->identifier.Description,
		adapter->identifier.Driver
	);

	// Select a device (depending of the value of c_d3d_device)
	d3dinfo_t::device_t* device;
	if (*d3d_device == 0 && adapter->hal_device.supported)
		device = &adapter->hal_device;
	else if (*d3d_device == 1 && adapter->ref_device.supported)
		device = &adapter->ref_device;
	else
		return "Requested d3d device unavailable";
	if (device == &adapter->hal_device)
		console.printf("Direct3D: Using HAL device\n");
	else
		console.printf("Direct3D: Using Reference Rasterizer\n");

	// Chech any required capabilities are supported
	if (!confirm_device(*device))
		return result_t::last;

	// Select the buffer format we want
	d3dinfo_t::format_t* format;
	if (*display_fullscreen) {
		if (*display_color_depth == 32) {
			if (device->format_A8R8G8B8_fullscreen.supported)
				format = &device->format_A8R8G8B8_fullscreen;
			else if (device->format_X8R8G8B8_fullscreen.supported)
				format = &device->format_X8R8G8B8_fullscreen;
			else
				return "No suitable 32 bit display modes available";
		} else if (*display_color_depth == 16) {
			if (device->format_X1R5G5B5_fullscreen.supported)
				format = &device->format_X1R5G5B5_fullscreen;
			else if (device->format_R5G6B5_fullscreen.supported)
				format = &device->format_R5G6B5_fullscreen;
			else
				return "No suitable 16 bit display modes available";
		}
	} else {
		if (*display_color_depth == 32) {
			if (adapter->display_mode.format == D3DFMT_X8R8G8B8 && device->format_X8R8G8B8_windowed.supported)
				format = &device->format_X8R8G8B8_windowed;
			else if (adapter->display_mode.format == D3DFMT_A8R8G8B8 && device->format_A8R8G8B8_windowed.supported)
				format = &device->format_A8R8G8B8_windowed;
			else
				return "Cannot support 32 bit color windowed mode with current display settings";
		} else if (*display_color_depth == 16) {
			if (adapter->display_mode.format == D3DFMT_X1R5G5B5 && device->format_X1R5G5B5_windowed.supported)
				format = &device->format_X1R5G5B5_windowed;
			else if (adapter->display_mode.format == D3DFMT_R5G6B5 && device->format_R5G6B5_windowed.supported)
				format = &device->format_R5G6B5_windowed;
			else
				return "Cannot support 16 bit color windowed mode with current display settings";
		}
	}

	// Select the adapter mode
	if (*display_fullscreen) {
		for (int i = 0; i < adapter->num_modes; ++i)
			if (adapter->modes[i].width == *display_width
			 && adapter->modes[i].height == *display_height
			 && adapter->modes[i].format == format->format)
				display_mode = &adapter->modes[i];
		if (i == adapter->num_modes)
			return "No matching display mode for requested display_width and display_height";
	} else {
		display_mode = &adapter->display_mode;
	}

	// Select the back buffer format
	if (format->alpha_format.supported)
		back_buffer_format = &format->alpha_format;
	else if (format->back_buffer_format.supported)
		back_buffer_format = &format->back_buffer_format;
	else
		return "No usable back buffer format found";

	// Finally choose the depth/stencil buffer format
	D3DFORMAT depth_stencil_format = D3DFMT_UNKNOWN;
	for (int i = 0; i < back_buffer_format->num_depth_stencil_formats; ++i) {
		if (back_buffer_format->depth_stencil_formats[i] == D3DFMT_D32)
			if (*display_z_depth == 32 && *display_stencil_depth == 0)
				depth_stencil_format = D3DFMT_D32;
		if (back_buffer_format->depth_stencil_formats[i] == D3DFMT_D15S1)
			if (*display_z_depth == 15 && *display_stencil_depth == 1)
				depth_stencil_format = D3DFMT_D15S1;
		if (back_buffer_format->depth_stencil_formats[i] == D3DFMT_D24S8)
			if (*display_z_depth == 24 && *display_stencil_depth == 8)
				depth_stencil_format = D3DFMT_D24S8;
		if (back_buffer_format->depth_stencil_formats[i] == D3DFMT_D16)
			if (*display_z_depth == 16 && *display_stencil_depth == 0)
				depth_stencil_format = D3DFMT_D16;
		if (back_buffer_format->depth_stencil_formats[i] == D3DFMT_D24X8)
			if (*display_z_depth == 24 && *display_stencil_depth == 0)
				depth_stencil_format = D3DFMT_D24X8;
		if (back_buffer_format->depth_stencil_formats[i] == D3DFMT_D24X4S4)
			if (*display_z_depth == 24 && *display_stencil_depth == 0)
				depth_stencil_format = D3DFMT_D24X4S4;
	}
	if (depth_stencil_format == D3DFMT_UNKNOWN)
		return "No acceptable depth stencil buffer format found";

	d3dpp.BackBufferFormat = back_buffer_format->back_buffer_format;
	d3dpp.BackBufferCount = *display_back_buffers;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow	= hwnd;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = depth_stencil_format;
	d3dpp.Flags = 0;
	d3dpp.FullScreen_RefreshRateInHz = 0;

	if (*display_fullscreen) {
		d3dpp.Windowed = FALSE;
		d3dpp.BackBufferWidth = display_mode->width;
		d3dpp.BackBufferHeight = display_mode->height;
		if (*display_vsync)
			d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		else
			d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	} else {	// Windowed
		d3dpp.Windowed = TRUE;
		d3dpp.BackBufferWidth = 0;
		d3dpp.BackBufferHeight = 0;
		d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	}

	return true;
}

result_t
d3d_t::confirm_device(const d3dinfo_t::device_t& device)
	// Confirm that any required device capabilities are present
{
	if (*display_fullscreen) {
		if (*display_vsync) {	// Make sure vsync is supported
			if (!(device.caps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE))
				return "Device does not support vertical sync";
		} else {
			if (!(device.caps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE))
				return "Device requires vertical sync to be enabled";
		}
	} else {	// Windowed
		if (!(device.caps.Caps2 & D3DCAPS2_CANRENDERWINDOWED))
			return "Device unable to render in windowed mode";
	}
	if (!(device.caps.PrimitiveMiscCaps & D3DPMISCCAPS_BLENDOP))
		return "Device does not support alpha blending";
	if (!(device.caps.PrimitiveMiscCaps & D3DPMISCCAPS_CULLCW))
		return "Device does not support clockwise culling";
	if (!(device.caps.PrimitiveMiscCaps & D3DPMISCCAPS_CULLCCW))
		return "Device does not support counter-clockwise culling";
	if (!(device.caps.PrimitiveMiscCaps & D3DPMISCCAPS_MASKZ))
		return "Device cannot disable zbuffer modification for pixel operations";
	if (device.caps.MaxVertexIndex < MAX_INDICES)
		return "Device cannot support sufficiently large index buffers";
	if (!(device.caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT))
		return "Device is not capable of hardware vertex processing";

	return true;
}
