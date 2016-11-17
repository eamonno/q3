//-----------------------------------------------------------------------------
// File: d3ddev.cpp
//
// Desc: Implementation of d3ddev_t
//-----------------------------------------------------------------------------

#include "d3ddev.h"
#include "console.h"
#include "pak.h"
#include "util.h"
#include "timer.h"
#include "ui.h"
#include <d3dx8.h>
#include <memory>
#include "win.h"
#include "exec.h"
#include "mem.h"

#define new mem_new

using std::auto_ptr;

#undef d3d
#undef d3ddev

// General Purpose cvars
cvar_int_t showtris("showtris", 0, 0, 2);
cvar_int_t c_display_width("c_display_width", 640);
cvar_int_t c_display_height("c_display_height", 480);
cvar_int_t c_color_depth("c_color_depth", 32);
cvar_int_t c_z_depth("c_z_depth", 24);
cvar_int_t c_stencil_depth("c_stencil_depth", 8);
cvar_int_t c_back_buffers("c_back_buffers", 2, 2, 3);
cvar_int_t c_fullscreen("c_fullscreen", 0, 0, 1);
cvar_int_t c_vsync("c_vsync", 0, 0, 1);
cvar_int_t c_d3d_use_adapter("c_d3d_use_adapter", 0);
cvar_int_t c_d3d_use_reference_device("c_d3d_use_reference_device", 1, 0, 2);
	// 0 = only use a hardware device
	// 1 = only use the reference device if no hardware device is present
	// 2 = only use the reference device

// Configuration cvars, highly unlikely to change, changes to these may cause
// some nasty behaviour
cvar_int_t c_max_textures("c_max_textures", 2048);
cvar_int_t c_max_shaders("c_max_shaders", 2048);
cvar_int_t c_max_shader_passes("c_max_shader_passes", 4096);
cvar_int_t c_max_static_verts("c_max_static_verts", 60000);
cvar_int_t c_max_static_inds("c_max_static_inds", 120000);
cvar_int_t c_max_dynamic_verts("c_max_dynamic_verts", 20000);
cvar_int_t c_max_dynamic_inds("c_max_dynamic_inds", 20000);

// Surface flags
const uint SF_ACTIVE	= 0x01;	// Shader is currently in use
const uint SF_RETAIN	= 0x02;	// Never unload this shader
const uint SF_NOMIPMAPS	= 0x04;	// No mipmaps for this shader
const uint SF_NOPICMIP	= 0x08;	// Never picmip this shader
const uint SF_CULLFRONT	= 0x10;	// Cull front faces
const uint SF_CULLBACK	= 0x20;	// Cull back faces (default)

namespace {
//	// Most textures that can be loaded at once
//	const int MAX_TEXTURES = 2048;
//	// Most shaders that can be loaded at once
//	const int MAX_SHADERS = 2048;
//	// Most total shader passes
//	const int MAX_SHADER_PASSES = 4096;
	// Most textures for an anim map
	const int MAX_PASS_MAPS = 8;
	// Most texture coordinate mods for a pass
	const int MAX_PASS_TCMODS = 3;

//	const int MAX_STATIC_VERTS = 60000;
//	const int MAX_STATIC_INDS = 120000;

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

	const htexture_t HT_WHITEIMAGE = 1;	// Texture handle to refer to the whiteimage
	const htexture_t HT_LIGHTMAP = 2;	// Texture handle to refer to the current face lightmap

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


struct d3ddev_t::texture_t {
	texture_t()	:
		flags(0)
	{}

	str_t<64>	name;
	int			flags;				// Shader flags (textures are also shaders)
	com_ptr_t<IDirect3DTexture8> texture;
};

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

struct d3ddev_t::shader_t {
	shader_t() : 
		first_pass(-1),
		num_passes(0),
		flags(SF_CULLBACK),
		sort(SORT_OPAQUE)
	{}

	str_t<64>	name;
	int			flags;			// Shader flags
	int			sort;
	int			first_pass;
	int			num_passes;
};

struct d3ddev_t::shader_pass_t {
	shader_pass_t() :
		rgbgen(RGBGEN_IDENTITY),
		alphagen(ALPHAGEN_IDENTITY),
		flags(0),
		num_maps(0),
		num_tcmods(0),
		anim_freq(0.0f),
		alpha_func(D3DCMP_ALWAYS),
		alpha_ref(0),
		src_blend(D3DBLEND_ONE),
		dest_blend(D3DBLEND_ZERO),
		depth_func(D3DCMP_LESSEQUAL)
	{}
	
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

d3ddev_t::d3ddev_t() : 
	adapters(0),
	num_adapters(0), 
	chosen_adapter(0),
	chosen_device(0),
	chosen_format(0), 
	chosen_mode(0), 
	chosen_back_buffer_format(0),
	chosen_depth_stencil_format(D3DFMT_UNKNOWN),
	num_shaders(0),
	num_textures(0),
	num_passes(0),
	shaders(0),
	textures(0),
	passes(0),
	textures_to_load(0),
	num_static_verts(0),
	num_static_inds(0)
{ 
	u_zeromem(&d3dpp, sizeof(d3dpp)); 
}

d3ddev_t&
d3ddev_t::get_instance()
{
	static std::auto_ptr<d3d_t> instance(new d3d_t());
	return *instance;
}

uint
d3ddev_t::get_surface_flags(hshader_t shader)
{
	if (shader < *c_max_textures)
		return SF_CULLBACK;
	else
		return shaders[shader - *c_max_textures].flags;
}

result_t
d3ddev_t::init()
	// Initialises the device, scans for available display modes etc, does not
	// actually set a display mode
{
	shaders = new shader_t[*c_max_shaders];
	textures = new texture_t[*c_max_textures];
	passes = new shader_pass_t[*c_max_shader_passes];

	textures[0].name = "noshader";
	textures[1].name = "$whiteimage";
	textures[2].name = "$lightmap";

	textures[0].flags = SF_RETAIN | SF_ACTIVE;
	textures[1].flags = SF_RETAIN | SF_ACTIVE;
	textures[2].flags = SF_RETAIN | SF_ACTIVE;

	num_textures += 3;

	return result_t::last;
}

int
compare_shaders(const void* v1, const void* v2)
{
	const d3ddev_t::shader_t* s1 = static_cast<const d3d_t::shader_t*>(v1);
	const d3ddev_t::shader_t* s2 = static_cast<const d3d_t::shader_t*>(v2);

	// First sort by sort
	if (s1->sort < s2->sort)
		return -1;
	if (s1->sort > s2->sort)
		return 1;

	// Then sort in descenting number of passes
	if (s1->num_passes < s2->num_passes)
		return -1;
	if (s1->num_passes > s2->num_passes)
		return 1;

	// Just sort alphabetically after that
	return s1->name.cmpfn(s2->name);
}

result_t
d3ddev_t::create()
	// Create the Direct 3d device, returns success or failure
{
	if (!choose_adapter())
		return result_t::last;
	if (!choose_adapter_device())
		return result_t::last;
	if (!choose_adapter_format())
		return result_t::last;
	if (!choose_adapter_mode())
		return result_t::last;
	if (!choose_adapter_back_buffer_format())
		return result_t::last;
	if (!choose_adapter_depth_stencil_format())
		return result_t::last;

	// Sort the shaders
	qsort(shaders, num_shaders, sizeof(shader_t), compare_shaders);

// TO-DO - try controlling vsync with the SwapEffect being set to
//			D3DSWAPEFFET_COPY if in windowed mode, though it may
//          have performance implications

	d3dpp.BackBufferFormat = chosen_back_buffer_format->format;
	d3dpp.BackBufferCount = *c_back_buffers;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow	= hwnd;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = chosen_depth_stencil_format;
	d3dpp.Flags = 0;
	d3dpp.FullScreen_RefreshRateInHz = 0;

	if (*c_fullscreen) {
		d3dpp.Windowed = FALSE;
		d3dpp.BackBufferWidth = *c_display_width;
		d3dpp.BackBufferHeight = *c_display_height;
		if (*c_vsync)
			d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		else
			d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	} else {	// Windowed
		d3dpp.Windowed = TRUE;
		d3dpp.BackBufferWidth = 0;
		d3dpp.BackBufferHeight = 0;
		d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	}

	uint behaviour = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	if (chosen_device->caps.DevCaps & D3DDEVCAPS_PUREDEVICE)
		behaviour |= D3DCREATE_PUREDEVICE;	// Create a pure device if available

//	if (chosen_device->caps.VertexShaderVersion < D3DVS_VERSION(1, 1))
//		return "Vertex Shader version 1.1 not supported";

	HRESULT hr;

	hr = d3d->CreateDevice(
		chosen_adapter->ordinal,
		chosen_device->type,
		hwnd, 
		behaviour,
		&d3dpp,
		&d3ddev
	);
	if (FAILED(hr))
		return "IDirect3D8->CreateDevice() failed";

	// Create the dynamic usage index buffer
	hr = d3ddev->CreateIndexBuffer(
		*c_max_dynamic_inds * sizeof(index_t),
		D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_DEFAULT,
		&ibuf
	);
	if (FAILED(hr))
		return "IDirect3D8->CreateIndexBuffer() failed for dynamic index buffer";

	// Create the dynamic usage vertex buffer
	hr = d3ddev->CreateVertexBuffer(
		*c_max_dynamic_verts * sizeof(vertex_t),
		D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		FIXED_VERTEX_FORMAT,
		D3DPOOL_DEFAULT,
		&vbuf
	);
	if (FAILED(hr))
		return "IDirect3D8->CreateVertexBuffer() failed for dynamic vertex buffer";

	// Create the static usage index buffer
	d3ddev->CreateIndexBuffer(
		*c_max_static_inds * sizeof(index_t),
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_DEFAULT,
		&sibuf
	);
	if (FAILED(hr))
		return "IDirect3D8->CreateIndexBuffer() failed for static index buffer";

	// Create the static usage vertex buffer
	d3ddev->CreateVertexBuffer(
		*c_max_static_verts * sizeof(vertex_t),
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
	d3ddev->SetGammaRamp(D3DSGR_CALIBRATE, &ramp);

	return true;
}

const char*
d3ddev_t::get_error_string(HRESULT hr)
{
	const int MAX_ERROR_STRING_LENGTH = 1024;

	static char str[MAX_ERROR_STRING_LENGTH];

	HRESULT hre = D3DXGetErrorString(hr, str, MAX_ERROR_STRING_LENGTH);
	if (FAILED(hre))
		u_snprintf(str, MAX_ERROR_STRING_LENGTH, "D3DXGetErrorString() failed for hresult %#x", hr);
	return str;
}

int
d3ddev_t::upload_static_verts(const vertex_t* verts, int count)
{
	console.debugf("uploading %d static vertices for total %d static vertices\n", count, count + num_static_verts);

	vertex_t* ver;
	if (num_static_verts + count < *c_max_static_verts) {
		svbuf->Lock(
			num_static_verts * sizeof(vertex_t),
			count * sizeof(vertex_t),
			reinterpret_cast<ubyte**>(&ver), 
			D3DLOCK_NOOVERWRITE
		);
		u_memcpy(ver, verts, count * sizeof(vertex_t));
		svbuf->Unlock();
		num_static_verts += count;
		return num_static_verts - count;
	}
	return -1;
}

int
d3ddev_t::upload_static_inds(const index_t* inds, int count)
{
	console.debugf("uploading %d static indices for total %d static indices\n", count, count + num_static_inds);

	index_t* ind;
	if (num_static_inds + count < *c_max_static_inds) {
		sibuf->Lock(
			num_static_inds * sizeof(index_t),
			count * sizeof(index_t),
			reinterpret_cast<ubyte**>(&ind), 
			D3DLOCK_NOOVERWRITE
		);
		for (int i = 0; i < count; ++i)
			ind[i] = inds[i];
		sibuf->Unlock();
		num_static_inds += count;
		return num_static_inds - count;
	}
	return -1;
}

void
d3ddev_t::destroy()
	// Free any resources used by the device
{
	delete [] shaders;
	num_shaders = 0;
	shaders = 0;

	delete [] passes;
	num_passes = 0;
	passes = 0;

	delete [] textures;
	num_textures = 0;
	textures = 0;

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
	d3d = 0;

	for (int a = 0; a < num_adapters; ++a) {
		delete [] adapters[a].name;
		delete [] adapters[a].modes;
		if (adapters[a].devices) {
			for (int d = 0; d < adapters[a].num_devices; ++d) {
				for (int f = 0; f < adapters[a].devices[d].num_formats; ++f) {
					if (adapters[a].devices[d].formats[f].back_buffer_format) {
						delete [] adapters[a].devices[d].formats[f].back_buffer_format->depth_stencil_formats;
						delete [] adapters[a].devices[d].formats[f].back_buffer_format->multisample_types;
						delete adapters[a].devices[d].formats[f].back_buffer_format;
					}
					if (adapters[a].devices[d].formats[f].alpha_format) {
						delete [] adapters[a].devices[d].formats[f].alpha_format->depth_stencil_formats;
						delete [] adapters[a].devices[d].formats[f].alpha_format->multisample_types;
						delete adapters[a].devices[d].formats[f].alpha_format;
					}
					delete [] adapters[a].devices[d].formats[f].texture_formats;
					delete [] adapters[a].devices[d].formats[f].render_formats;
					delete [] adapters[a].devices[d].formats[f].volume_formats;
					delete [] adapters[a].devices[d].formats[f].cube_formats;
				}
				delete [] adapters[a].devices[d].formats;
			}
		}
		delete [] adapters[a].devices;
	}
	delete [] adapters;
	num_adapters = 0;
	adapters = 0;
}

bool
d3ddev_t::lost()
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
d3ddev_t::begin()
{
//	d3ddev->SetTransform(D3DTS_WORLD, NULL);
//	d3ddev->SetTransform(D3DTS_VIEW, NULL);
//	d3ddev->SetTransform(D3DTS_PROJECTION, NULL);
	next_vertex = 0;
	next_index = 0;
	return SUCCEEDED(d3ddev->BeginScene());
}

void
d3ddev_t::end()
{
	d3ddev->EndScene();
	d3ddev->Present(NULL, NULL, NULL, NULL); 
}

matrix_t world_matrix;

void
d3ddev_t::set_camera(const camera_t& camera)
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
	// TO-DO: THIS DOES NOT SORT CORRECTLY, IT SORTS TEXTURES BEFORE ALL SHADERS
	const face_t* face1 = static_cast<const face_t*>(f1);
	const face_t* face2 = static_cast<const face_t*>(f2);

	if (face1->shader != face2->shader)
		return face1->shader - face2->shader;
	return face1->lightmap - face2->lightmap;
}

void
d3ddev_t::render_list(display_list_t& dl)
	// Render the face in question
{
	qsort(dl.face_buffer(), dl.num_faces(), sizeof(face_t), compare_faces);

	int active_vbuf = 0;	// 0 = none, 1 = static, 2 = dynamic

	const face_t* face;
	for (const face_t* f = dl.face_buffer(); f != dl.face_buffer_end(); f = (face == f ? f + 1 : face)) {
		face = f;
		begin_shader(f->shader, f->lightmap);
		for (int pass = 0; pass < current_shader_passes; ++pass) {
			begin_pass(pass);
			for (face = f; face->shader == f->shader && face->lightmap == f->lightmap && face != dl.face_buffer_end(); ++face) {
				int use_vbuf = 0;

				// Copy over the index information, since the same index information is
				// valid for multiple passes it can all be copied at once
				if (face->inds) {
					index_t* ind;
					if (next_index + face->num_inds < *c_max_dynamic_inds) {
						ibuf->Lock(
							next_index * sizeof(index_t),
							face->num_inds * sizeof(index_t),
							reinterpret_cast<ubyte**>(&ind),
							D3DLOCK_NOOVERWRITE
						);
					} else {
						ibuf->Lock(0, 0, reinterpret_cast<ubyte**>(&ind), D3DLOCK_DISCARD);
						next_index = 0;
					}
					u_memcpy(ind, face->inds, face->num_inds * sizeof(index_t));
					ibuf->Unlock();
					d3ddev->SetIndices(ibuf, face->verts ? next_vertex : face->base_vert);
				} else {
					d3ddev->SetIndices(sibuf, face->verts ? next_vertex : face->base_vert);
				}

				// Copy over the vertex information if required
				if (face->verts) {
					vertex_t* ver;
					if (next_vertex + face->num_verts < *c_max_dynamic_verts) {
						vbuf->Lock(
							next_vertex * sizeof(vertex_t),
							face->num_verts * sizeof(vertex_t),
							reinterpret_cast<ubyte**>(&ver),
							D3DLOCK_NOOVERWRITE
						);
					} else {
						vbuf->Lock(0, 0, reinterpret_cast<ubyte**>(&ver), D3DLOCK_DISCARD);
						next_vertex = 0;
					}
					u_memcpy(ver, face->verts, face->num_verts * sizeof(vertex_t));
					vbuf->Unlock();
					use_vbuf = 2;
				} else {
					use_vbuf = 1;
				}

				// Change the active vertex buffer if nescessary
				if (use_vbuf != active_vbuf) {
					if (use_vbuf == 1)
						d3ddev->SetStreamSource(0, svbuf, sizeof(vertex_t));
					else
						d3ddev->SetStreamSource(0, vbuf, sizeof(vertex_t));
					active_vbuf = use_vbuf;
				}

				// Now do the drawing
				HRESULT hr = d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, face->num_verts, face->inds ? next_index : face->base_ind, face->num_inds / 3);
				u_assert(SUCCEEDED(hr));

				// Update the dynamic vertex and index buffer positions
				if (face->verts)
					next_vertex += face->num_verts;
				if (face->inds)
					next_index += face->num_inds;
			}
			end_pass();
		}
		end_shader();
	}
	if (*showtris)
		show_tris(dl);
}

void
d3ddev_t::show_tris(display_list_t& dl)
	// Render the face in question
{
	d3ddev->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	d3ddev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	d3ddev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	if (*showtris == 1) {
		d3ddev->SetRenderState(D3DRS_TEXTUREFACTOR, color_t::white);
	} else {
		// Blend alpha with white tfactor, less glaring, the white makes any
		// tris with black diffuse visible
		d3ddev->SetRenderState(D3DRS_TEXTUREFACTOR, 0x1fffffff);
		d3ddev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
		d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_BLENDFACTORALPHA);	// use diffuse color
	}
	for (const face_t* face = dl.face_buffer(); face != dl.face_buffer_end(); ++face)
		render_no_shader(face);
	if (*showtris == 2)
		d3ddev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	d3ddev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	d3ddev->SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff);
	d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	d3ddev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	d3ddev->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
}

hshader_t 
d3ddev_t::get_shader(const char* name, bool retain)
	// Return the handle to the named shader
{
	// Search for a shader with that name
	for (int i = 0; i < num_shaders; ++i)
		if (shader_name_cmp(name, shaders[i].name)) {
			shaders[i].flags |= SF_ACTIVE;
			for (int p = 0; p < shaders[i].num_passes; ++p) {
				const shader_pass_t& pass = passes[shaders[i].first_pass + p];
				for (int map = 0; map < pass.num_maps; ++map) {
					htexture_t ht = pass.maps[map];
					if ((textures[ht].flags & SF_ACTIVE) == 0) {
						textures[ht].flags |= SF_ACTIVE;
						++textures_to_load;
					}
				}
			}
			console.printf("get_shader: returned shader %s\n", shaders[i].name.str());
			return i + *c_max_textures;
		}

	// Search for a texture with that name
	int texture = define_texture(name);

	if (texture == 0)
		console.warnf("requested unknown shader %s\n", name);
	else
		console.printf("get_shader: returned textur %s\n", textures[texture].name.str());

	if ((textures[texture].flags & SF_ACTIVE) == 0) {
		textures[texture].flags |= SF_ACTIVE;
		++textures_to_load;
	}
		
	return texture;
}

void
d3ddev_t::begin_shader(hshader_t shader, htexture_t lightmap)
{
	current_shader = shader;
	current_lightmap = lightmap;

	if (current_shader >= *c_max_textures) {
		// This is a proper shader
		const shader_t& shader = shaders[current_shader - *c_max_textures];
		current_shader_passes = shader.num_passes;

		if (shader.flags & SF_CULLFRONT)
			d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		else if (!(shader.flags & SF_CULLBACK))
			d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		
	} else {
		current_shader_passes = 1;
	}
}

void
d3ddev_t::end_shader()
{
	if (current_shader >= *c_max_textures) {
		// This is a proper shader
		const shader_t& shader = shaders[current_shader - *c_max_textures];

		if ((shader.flags & SF_CULLFRONT) || !(shader.flags & SF_CULLBACK))
			d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	}
}

#include "entity.h"
extern player_t player;

void
d3ddev_t::begin_pass(int num)
{
	current_pass = num;

	color_t modulate(color_t::identity);

	if (current_shader >= *c_max_textures) {
		const shader_pass_t& pass = passes[shaders[current_shader - *c_max_textures].first_pass + current_pass];

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
		if (textures[map].texture == 0 && map > HT_LIGHTMAP)
			load_texture(map, 0);	// Load the texture if nescessary
		if (map == HT_LIGHTMAP)
			d3ddev->SetTexture(0, textures[current_lightmap].texture);
		else
			d3ddev->SetTexture(0, textures[map].texture);

	} else {
		if (textures[current_shader].texture == 0 && current_shader > HT_LIGHTMAP)
			load_texture(current_shader, 0);	// Load the texture if nescessary
		d3ddev->SetTexture(0, textures[current_shader].texture);
		if (current_lightmap) {
			d3ddev->SetTexture(1, textures[current_lightmap].texture);
			d3ddev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
		}
	}
}

void
d3ddev_t::end_pass()
{
	if (current_shader >= *c_max_textures) {
		const shader_pass_t& pass = passes[shaders[current_shader - *c_max_textures].first_pass + current_pass];

		if (pass.flags & PF_ALPHABLEND) {
			d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		}

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

	} else {
		if (current_lightmap)
			d3ddev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	}
}

void
d3ddev_t::render_no_shader(const face_t* face)
{
	// Copy over the index information, since the same index information is
	// valid for multiple passes it can all be copied at once
	if (face->inds) {
		index_t* ind;
		if (next_index + face->num_inds < *c_max_dynamic_inds) {
			ibuf->Lock(
				next_index * sizeof(index_t),
				face->num_inds * sizeof(index_t),
				reinterpret_cast<ubyte**>(&ind),
				D3DLOCK_NOOVERWRITE
			);
		} else {
			ibuf->Lock(0, face->num_inds * sizeof(index_t), reinterpret_cast<ubyte**>(&ind), D3DLOCK_DISCARD);
			next_index = 0;
		}
		u_memcpy(ind, face->inds, face->num_inds * sizeof(index_t));
		ibuf->Unlock();
		d3ddev->SetIndices(ibuf, face->verts ? next_vertex : face->base_vert);
	} else {
		d3ddev->SetIndices(sibuf, face->verts ? next_vertex : face->base_vert);
	}

	if (face->verts) {
		vertex_t* ver;
		if (next_vertex + face->num_verts < *c_max_dynamic_verts) {
			vbuf->Lock(
				next_vertex * sizeof(vertex_t),
				face->num_verts * sizeof(vertex_t),
				reinterpret_cast<ubyte**>(&ver),
				D3DLOCK_NOOVERWRITE
			);
		} else {
			vbuf->Lock(0, face->num_verts * sizeof(vertex_t), reinterpret_cast<ubyte**>(&ver), D3DLOCK_DISCARD);
			next_vertex = 0;
		}
		u_memcpy(ver, face->verts, face->num_verts * sizeof(vertex_t));
		vbuf->Unlock();
		d3ddev->SetStreamSource(0, vbuf, sizeof(vertex_t));
	} else {
		d3ddev->SetStreamSource(0, svbuf, sizeof(vertex_t));
	}

	HRESULT hr = d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, face->num_verts, face->inds ? next_index : face->base_ind, face->num_inds / 3);
	u_assert(SUCCEEDED(hr));

	if (face->verts)
		next_vertex += face->num_verts;
	if (face->inds)
		next_index += face->num_inds;
}

bool
d3ddev_t::generate_mipmaps(texture_t& texture)
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

void
d3ddev_t::load_resource()
{
	for (int i = HT_LIGHTMAP + 1; i < num_textures; ++i) {
		if ((textures[i].flags & SF_ACTIVE) && textures[i].texture == 0) {
			load_texture(i, 0);
			return;
		}
	}
	// HACK HACK HACK
	if (i == num_textures)
		textures_to_load = 0;
}

void
d3ddev_t::free_resources()
{
	for (int i = HT_LIGHTMAP + 1; i < num_textures; ++i) {
		textures[i].texture = 0;
		textures[i].flags &= ~SF_ACTIVE;
	}
	num_static_verts = 0;
	num_static_inds = 0;
}

void
d3ddev_t::load_texture(htexture_t texture, int flags)
	// Loads a texture and returns a reference to it. The newly loaded texture is added to
	// the textures map before returning
{
	// TO-DO: Change to using D3DPOOL_DEFAULT to save having duplicates of every
	// texture in system memory, to do this they will all have to be re-loaded if
	// there is a device change
	u_assert(textures[texture].texture == 0);

	console.printf("loading texture: %s ... ", textures[texture].name.str());
	auto_ptr<file_t> file(pak.open_file(textures[texture].name));
	if (file.get()) {
		HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(
			d3ddev, file->data(), file->size(), D3DX_DEFAULT, D3DX_DEFAULT,
			((flags & SF_NOMIPMAPS) ? 1 : D3DX_DEFAULT), 0, chosen_format->alpha_format->format,
			D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &textures[texture].texture);
		if (FAILED(hr))
			console.printf("failed, %s\n", get_error_string(hr));
		else
			console.print("ok\n");
		if (textures[texture].flags & SF_ACTIVE)
			--textures_to_load;
		textures[texture].flags |= SF_ACTIVE;	// either way its loaded
	} else {
		console.print("failed, file not found\n");
	}
}

htexture_t
d3ddev_t::upload_lightmap_rgb(const ubyte* data, const int width, const int height, const char* name)
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

	u_assert(num_textures != *c_max_textures);

	texture_t& texture = textures[num_textures];

	HRESULT hr = d3ddev->CreateTexture(128, 128, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture.texture);
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
						if (*c_fullscreen == 0 || !(chosen_device->caps.Caps2 & D3DCAPS2_FULLSCREENGAMMA)) {
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
				texture.name = name;
				console.printf("%s successfully uploaded\n", name);
				texture.texture->UnlockRect(0);
				generate_mipmaps(texture);
				return num_textures++;

			} else {
				console.printf("IDirect3DTexture8->LockRect() failed, hr = %#x\n", hr);
			}
		} else {
			console.printf("IDirect3DTexture8->GetLevelDesc() failed, hr = %#x\n", hr);
		}
	} else {
		console.printf("IDirect3DDevice8->CreateTexture() falied: hr = %#x\n", hr);
	}
	return 0;
}

void
d3ddev_t::upload_shader(const token_t* tokens)
	// Convert a token stream into the internal representation of a shader
{
	u_assert(num_shaders != *c_max_shaders);
	shader_t* shader = &shaders[num_shaders];

	// First token is always the shaders name
	shader->name = tokens[0].string_value;

//	if (shader->name.cmpfn("textures/sfx/x_conduit") == 0)
//		u_break();

	bool fail = false;
	// Skip token 2, its always SK_LEFT_BRACE
	for (const token_t* t = tokens + 2; t < (tokens + tokens->remaining) && !fail; t += t->remaining) {
		shader_pass_t* pass = &passes[num_passes + shader->num_passes];
		u_assert(num_passes + shader->num_passes < *c_max_shader_passes);
		// t now points to the next element in the shader body
		switch (t->type) {
		case SK_LEFT_BRACE:	// Start of a shader stage
			for (++t; t->type != SK_RIGHT_BRACE; t += t->remaining) {
				switch (t->type) {
				case SK_ALPHAFUNC:	// SE_ALPHAFUNC
					switch (t[1].type) {
					case SK_GE128:
						pass->alpha_func = D3DCMP_GREATEREQUAL;
						pass->alpha_ref  = 128;
						break;
					case SK_GT0:
						pass->alpha_func = D3DCMP_GREATER;
						pass->alpha_ref  = 0;
						break;
					case SK_LT128:
						pass->alpha_func = D3DCMP_LESS;
						pass->alpha_ref  = 128;
					}
					pass->rgbgen = RGBGEN_IDENTITY;
					pass->flags |= PF_ALPHATEST;
					pass->flags |= PF_NOZWRITE;
					if (shader->num_passes == 0)
						shader->sort = SORT_ADDITIVE;
					break;
				case SK_ALPHAGEN:	// SE_ALPHAGEN
					switch (t[1].type) {
					case SK_VERTEX:
						pass->alphagen = ALPHAGEN_VERTEX;
						break;
					case SK_ENTITY:
					case SK_LIGHTINGSPECULAR:
						break;
					case SK_WAVE:
						pass->alphagen = ALPHAGEN_WAVE;
						switch (t[2].type) {
						case SK_INVERSESAWTOOTH:
							pass->alphagen_wave.type = WAVE_INVERSESAWTOOTH;
							break;
						case SK_NOISE:
							pass->alphagen_wave.type = WAVE_NOISE;
							break;
						case SK_SAWTOOTH:
							pass->alphagen_wave.type = WAVE_SAWTOOTH;
							break;
						case SK_SIN:
							pass->alphagen_wave.type = WAVE_SIN;
							break;
						case SK_SQUARE:
							pass->alphagen_wave.type = WAVE_SQUARE;
							break;
						case SK_TRIANGLE:
							pass->alphagen_wave.type = WAVE_TRIANGLE;
						}
						pass->alphagen_wave.base	  = t[3].float_value;
						pass->alphagen_wave.amplitude = t[4].float_value;
						pass->alphagen_wave.phase	  = t[5].float_value;
						pass->alphagen_wave.frequency = t[6].float_value;
					}
					break;
				case SK_ANIMMAP:	// SL_FLOAT, SE_ANIMMAP
					pass->anim_freq = t[1].float_value;
					t += 2;
					// Yes, the -1 index here is correct and safe to use
					while (t[-1].remaining != 1 && pass->num_maps < MAX_PASS_MAPS) {
						pass->maps[pass->num_maps] = define_texture(t->string_value);
						if (pass->maps[pass->num_maps++] == 0)
							fail = true;
						++t;
					}
					--t;
					break;
				case SK_BLENDFUNC:	// SE_BLENDFUNC
					if (t->remaining == 3) {	// SE_SRCBLEND, SE_DESTBLEND
						switch (t[1].type) {
						case SK_GL_DST_COLOR:
							pass->src_blend = D3DBLEND_DESTCOLOR;
							break;
						case SK_GL_ONE:
							pass->src_blend = D3DBLEND_ONE;
							break;
						case SK_GL_ONE_MINUS_DST_COLOR:
							pass->src_blend = D3DBLEND_INVDESTCOLOR;
							break;
						case SK_GL_ONE_MINUS_SRC_ALPHA:
							pass->src_blend = D3DBLEND_INVSRCALPHA;
							break;
						case SK_GL_SRC_ALPHA:
							pass->src_blend = D3DBLEND_SRCALPHA;
							break;
						case SK_GL_ZERO:
							pass->src_blend = D3DBLEND_ZERO;
						}
						switch (t[2].type) {
						case SK_GL_SRC_COLOR:
							pass->dest_blend = D3DBLEND_SRCCOLOR;
							break;
						case SK_GL_ONE:
							pass->dest_blend = D3DBLEND_ONE;
							break;
						case SK_GL_ONE_MINUS_DST_ALPHA:
							pass->dest_blend = D3DBLEND_INVDESTALPHA;
							break;
						case SK_GL_ONE_MINUS_SRC_ALPHA:
							pass->dest_blend = D3DBLEND_INVSRCALPHA;
							break;
						case SK_GL_ONE_MINUS_SRC_COLOR:
							pass->dest_blend = D3DBLEND_INVSRCCOLOR;
							break;
						case SK_GL_SRC_ALPHA:
							pass->dest_blend = D3DBLEND_SRCALPHA;
							break;
						case SK_GL_ZERO:
							pass->dest_blend = D3DBLEND_ZERO;
						}
					} else {	// SE_BLENDFUNC
						switch (t[1].type) {
						case SK_ADD:
							pass->src_blend = D3DBLEND_ONE;
							pass->dest_blend = D3DBLEND_ONE;
							break;
						case SK_BLEND:
							pass->src_blend = D3DBLEND_SRCALPHA;
							pass->dest_blend = D3DBLEND_INVSRCALPHA;
							break;
						case SK_FILTER:
							pass->src_blend = D3DBLEND_DESTCOLOR;
							pass->dest_blend = D3DBLEND_ZERO;
						}
					}
					if ((pass->src_blend != D3DBLEND_ONE) || (pass->dest_blend != D3DBLEND_ZERO)) {
						pass->flags |= PF_ALPHABLEND;
						pass->flags |= PF_NOZWRITE;
						if (shader->num_passes == 0)
							shader->sort = SORT_ADDITIVE;
					}
					break;
				case SK_CLAMPMAP:	// SE_MAPNAME
					pass->maps[0] = define_texture(t[1].string_value);
					if (pass->maps[0] == 0)
						fail = true;
					pass->num_maps = 1;
					pass->flags |= PF_CLAMP;
					if (pass->maps[0] == HT_LIGHTMAP)
						pass->flags |= PF_USETC1;
					break;
				case SK_DEPTHFUNC:	// SE_DEPTHFUNC
					switch (t[1].type) {
					case SK_EQUAL:
						pass->depth_func = D3DCMP_EQUAL;
						break;
					case SK_LEQUAL:
						pass->depth_func = D3DCMP_LESSEQUAL;
						break;
					}
					break;
				case SK_DEPTHWRITE:	//
					pass->flags &= ~PF_NOZWRITE;
					break;
				case SK_MAP:		// SE_MAPNAME
					pass->maps[0] = define_texture(t[1].string_value);
					if (pass->maps[0] == 0)
						fail = true;
					pass->num_maps = 1;
					if (pass->maps[0] == HT_LIGHTMAP)
						pass->flags |= PF_USETC1;
					break;
				case SK_RGBGEN:		// SE_RGBGEN
					switch (t[1].type) {
					case SK_ENTITY:
						break;
					case SK_IDENTITY:
						pass->rgbgen = RGBGEN_IDENTITY;
						break;
					case SK_IDENTITYLIGHTING:
						pass->rgbgen = RGBGEN_IDENTITYLIGHTING;
						break;
					case SK_LIGHTINGDIFFUSE:
						break;
					case SK_EXACTVERTEX:
					case SK_VERTEX:
						pass->rgbgen = RGBGEN_VERTEX;
//						pass->tstates.add_activate_state(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
//						pass->tstates.add_deactivate_state(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
						break;
					case SK_WAVE:
						pass->rgbgen = RGBGEN_WAVE;
						switch (t[2].type) {
						case SK_INVERSESAWTOOTH:
							pass->rgbgen_wave.type = WAVE_INVERSESAWTOOTH;
							break;
						case SK_NOISE:
							pass->rgbgen_wave.type = WAVE_NOISE;
							break;
						case SK_SAWTOOTH:
							pass->rgbgen_wave.type = WAVE_SAWTOOTH;
							break;
						case SK_SIN:
							pass->rgbgen_wave.type = WAVE_SIN;
							break;
						case SK_SQUARE:
							pass->rgbgen_wave.type = WAVE_SQUARE;
							break;
						case SK_TRIANGLE:
							pass->rgbgen_wave.type = WAVE_TRIANGLE;
						}
						pass->rgbgen_wave.base		= t[3].float_value;
						pass->rgbgen_wave.amplitude	= t[4].float_value;
						pass->rgbgen_wave.phase		= t[5].float_value;
						pass->rgbgen_wave.frequency	= t[6].float_value;
						break;
					}
					break;
				case SK_TCGEN:		// SE_TCGEN
					switch (t[1].type) {
					case SK_BASE:
						pass->flags &= ~(PF_ENVIRONMENT);
						break;
					case SK_ENVIRONMENT:
						pass->flags |= PF_ENVIRONMENT;
						break;
					case SK_LIGHTMAP:
						pass->flags |= PF_USETC1;
					}
					break;
				case SK_TCMOD:		// SE_TCMOD
					switch (t[1].type) {
					case SK_ROTATE:		// SL_FLOAT
						u_assert(pass->num_tcmods != MAX_PASS_TCMODS);
						pass->tcmods[pass->num_tcmods].type = TCMOD_ROTATE;
						pass->tcmods[pass->num_tcmods].angle= t[2].float_value;
						++pass->num_tcmods;
						break;
					case SK_SCALE:		// SL_FLOAT, SL_FLOAT
						u_assert(pass->num_tcmods != MAX_PASS_TCMODS);
						pass->tcmods[pass->num_tcmods].type = TCMOD_SCALE;
						pass->tcmods[pass->num_tcmods].x	= t[2].float_value;
						pass->tcmods[pass->num_tcmods].y	= t[3].float_value;
						++pass->num_tcmods;
						break;
					case SK_SCROLL:		// SL_FLOAT, SL_FLOAT
						u_assert(pass->num_tcmods != MAX_PASS_TCMODS);
						pass->tcmods[pass->num_tcmods].type = TCMOD_SCROLL;
						pass->tcmods[pass->num_tcmods].x	= t[2].float_value;
						pass->tcmods[pass->num_tcmods].y	= t[3].float_value;
						++pass->num_tcmods;
						break;
					case SK_STRETCH:	// SE_WAVEFUNC
						u_assert(pass->num_tcmods != MAX_PASS_TCMODS);
						pass->tcmods[pass->num_tcmods].type = TCMOD_STRETCH;
						switch (t[2].type) {
						case SK_INVERSESAWTOOTH:
							pass->tcmods[pass->num_tcmods].wave.type = WAVE_INVERSESAWTOOTH;
							break;
						case SK_NOISE:
							pass->tcmods[pass->num_tcmods].wave.type = WAVE_NOISE;
							break;
						case SK_SAWTOOTH:
							pass->tcmods[pass->num_tcmods].wave.type = WAVE_SAWTOOTH;
							break;
						case SK_SIN:
							pass->tcmods[pass->num_tcmods].wave.type = WAVE_SIN;
							break;
						case SK_SQUARE:
							pass->tcmods[pass->num_tcmods].wave.type = WAVE_SQUARE;
							break;
						case SK_TRIANGLE:
							pass->tcmods[pass->num_tcmods].wave.type = WAVE_TRIANGLE;
						}
						pass->tcmods[pass->num_tcmods].wave.base	  = t[3].float_value;
						pass->tcmods[pass->num_tcmods].wave.amplitude = t[4].float_value;
						pass->tcmods[pass->num_tcmods].wave.phase	  = t[5].float_value;
						pass->tcmods[pass->num_tcmods].wave.frequency = t[6].float_value;
						++pass->num_tcmods;
						break;
					case SK_TRANSFORM:	// SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT
						break;
					case SK_TURB:		// SE_TURB
						break;
					}
					break;
				}
			}
			if (shader->num_passes == 0)
				shader->first_pass = num_passes;
			++shader->num_passes;
			break;
		case SK_CULL:					// SE_CULL
			switch (t[1].type) {
			case SK_BACK:
				shader->flags |= SF_CULLBACK;
				break;
			case SK_DISABLE:
			case SK_NONE:
			case SK_TWOSIDED:
				shader->flags &= ~(SF_CULLBACK | SF_CULLFRONT);
				break;
			case SK_FRONT:
				shader->flags &= ~(SF_CULLBACK);	// turn off the default
				shader->flags |= SF_CULLFRONT;
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
			shader->flags |= SF_NOMIPMAPS;
			break;
		case SK_NOPICMIP:
			shader->flags |= SF_NOPICMIP;
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
			shader->flags &= ~(SF_CULLFRONT | SF_CULLBACK);
			break;
		case SK_SORT:					// SE_SORT
			switch (t[1].type) {
			case SK_PORTAL:
				shader->sort = SORT_PORTAL;
				break;
			case SK_SKY:
				shader->sort = SORT_SKY;
				break;
			case SK_OPAQUE:
				shader->sort = SORT_OPAQUE;
				break;
			case SK_BANNER:
				shader->sort = SORT_BANNER;
				break;
			case SK_UNDERWATER:
				shader->sort = SORT_UNDERWATER;
				break;
			case SK_ADDITIVE:
				shader->sort = SORT_ADDITIVE;
				break;
			case SK_NEAREST:
				shader->sort = SORT_NEAREST;
				break;
			case SL_FLOAT:
				shader->sort = m_floor(t[1].float_value + 0.5f);
				break;
			}
			break;
		case SK_SURFACEPARM:			// SE_SURFACEPARM
			break;
		}
	}

	if (fail) {
		// Delete any information saved so its gone before the next shader is loaded
		for (int i = 0; i < shader->num_passes; ++i)
			passes[shader->first_pass + i] = shader_pass_t();
		shaders[num_shaders] = shader_t();
	} else {
		++num_shaders;
		num_passes += shader->num_passes;
	}
}


htexture_t
d3ddev_t::define_texture(const char* name)
	// Stores a texture name, makes sure the file exists on disk but does NOT
	// load the texture into memory
{
	u_assert(num_textures != *c_max_textures);

	htexture_t ht = get_texture(name);
	if (ht != 0)
		return ht;

	if (pak.file_exists(name)) {
		textures[num_textures].name = name;
		return num_textures++;
	}
	textures[num_textures].name = name;
	int length = textures[num_textures].name.length();

	if (length < 4)
		return 0;
	// If name ends in ".tga" try with a ".jpg" extension and vice versa
	if (textures[num_textures].name[length - 4] == '.') {
		if (u_tolower(textures[num_textures].name[length - 3]) == 't'
		 && u_tolower(textures[num_textures].name[length - 2]) == 'g'
		 && u_tolower(textures[num_textures].name[length - 1]) == 'a') {
			textures[num_textures].name[length - 3] = 'j';
			textures[num_textures].name[length - 2] = 'p';
			textures[num_textures].name[length - 1] = 'g';
		} else if (u_tolower(textures[num_textures].name[length - 3]) == 'j'
		 && u_tolower(textures[num_textures].name[length - 2]) == 'p'
		 && u_tolower(textures[num_textures].name[length - 1]) == 'g') {
			textures[num_textures].name[length - 3] = 't';
			textures[num_textures].name[length - 2] = 'g';
			textures[num_textures].name[length - 1] = 'a';
		}
		if (pak.file_exists(textures[num_textures].name))
			return num_textures++;
	}
	// That didnt work, try just adding the tga, then the jpg extension
	textures[num_textures].name += ".tga";
	if (pak.file_exists(textures[num_textures].name))
		return num_textures++;
	textures[num_textures].name[length] = '\0';
	textures[num_textures].name += ".jpg";
	if (pak.file_exists(textures[num_textures].name))
		return num_textures++;

	console.printf("Texture not found: %s\n", name);
	return 0;
}

htexture_t
d3ddev_t::get_texture(const char* name)
	// Return the texture handle of 0 if the texture has not yet been defined
	// This can do a LOT of work if the texture name does not exist, it should
	// be possible to write a better version of this
	// TO-DO: Speed this up to reduce load times
{
	// Check the exact name
	for (int i = 0; i < num_textures; ++i)
		if (shader_name_cmp(name, textures[i].name))
			return i;
	// No matching texture
	return 0;
}

result_t
d3ddev_t::choose_adapter()
	// Choose which adapter will be used
{
	if (num_adapters > 0)
		chosen_adapter = &adapters[0];
	if (chosen_adapter == NULL)
		return "No suitable adapter found";
	return true;
}

result_t
d3ddev_t::choose_adapter_device()
	// Choose which device will be used from the chosen adapter
{
	for (int i = 0; i < chosen_adapter->num_devices; i++)
		if (chosen_adapter->devices[i].type == D3DDEVTYPE_HAL 
		  && confirm_device(chosen_adapter->devices[i]) == true)
			break;
	if (i == chosen_adapter->num_devices)
		return "No acceptable device found";
	chosen_device = &chosen_adapter->devices[i];
	return true;
}

result_t
d3ddev_t::choose_adapter_format()
	// Choose a back buffer format
{
	if (*c_color_depth != 16 && *c_color_depth != 32)
		return "c_color_depth must be either 16 or 32 bits";

	for (int i = 0; i < chosen_device->num_formats; i++) {
		if (*c_color_depth == 32 && chosen_device->formats[i].format == D3DFMT_X8R8G8B8) {
			chosen_format = &chosen_device->formats[i];
			break;		// This mode is always acceptable
		} else if (*c_color_depth == 16 && chosen_device->formats[i].format == D3DFMT_X1R5G5B5) {
			chosen_format = &chosen_device->formats[i];
			break;		// This mode is always acceptable
		} else if (*c_color_depth == 16 && chosen_device->formats[i].format == D3DFMT_X4R4G4B4) {
			chosen_format = &chosen_device->formats[i];	// Dont break, try for a better mode
		} else if (*c_color_depth == 16 && chosen_device->formats[i].format == D3DFMT_R5G6B5) {
			chosen_format = &chosen_device->formats[i];	// Dont break, try for a better mode
		}
	}
	if (chosen_format == NULL)
		return "No suitable display format available";
	return true;
}

result_t
d3ddev_t::choose_adapter_mode()
	// Select an appropriate display mode
{

	if (*c_fullscreen) {
		for (int i = 0; i < chosen_adapter->num_modes; i++)
			if (*c_display_width == chosen_adapter->modes[i].width 
			 && *c_display_height == chosen_adapter->modes[i].height) {
				chosen_mode = &chosen_adapter->modes[i];
				break;
			}
	} else if (chosen_adapter->desktop_mode.format == chosen_format->format)
		chosen_mode = &chosen_adapter->desktop_mode;

	if (chosen_mode == NULL)
		return "No suitable screen mode found";
	return true;
}

result_t
d3ddev_t::choose_adapter_back_buffer_format()
	// Choose the format for the back buffer from those available for the
	// chosen device format
{
	if (chosen_format->alpha_format)
		chosen_back_buffer_format = chosen_format->alpha_format;
	else
		chosen_back_buffer_format = chosen_format->back_buffer_format;
	return true;
}

result_t
d3ddev_t::choose_adapter_depth_stencil_format()
	// Choose an appropriate depth/stencil buffer format to use
{
	for (int i = 0; i < chosen_back_buffer_format->num_depth_stencil_formats; i++) {
		if (chosen_back_buffer_format->depth_stencil_formats[i] == D3DFMT_D32)
			if (*c_z_depth == 32 && *c_stencil_depth == 0)
				chosen_depth_stencil_format = D3DFMT_D32;
		if (chosen_back_buffer_format->depth_stencil_formats[i] == D3DFMT_D15S1)
			if (*c_z_depth == 15 && *c_stencil_depth == 1)
				chosen_depth_stencil_format = D3DFMT_D15S1;
		if (chosen_back_buffer_format->depth_stencil_formats[i] == D3DFMT_D24S8)
			if (*c_z_depth == 24 && *c_stencil_depth == 8)
				chosen_depth_stencil_format = D3DFMT_D24S8;
		if (chosen_back_buffer_format->depth_stencil_formats[i] == D3DFMT_D16)
			if (*c_z_depth == 16 && *c_stencil_depth == 0)
				chosen_depth_stencil_format = D3DFMT_D16;
		if (chosen_back_buffer_format->depth_stencil_formats[i] == D3DFMT_D24X8)
			if (*c_z_depth == 24 && *c_stencil_depth == 0)
				chosen_depth_stencil_format = D3DFMT_D24X8;
		if (chosen_back_buffer_format->depth_stencil_formats[i] == D3DFMT_D24X4S4)
			if (*c_z_depth == 24 && *c_stencil_depth == 0)
				chosen_depth_stencil_format = D3DFMT_D24X4S4;
	}
	if (chosen_depth_stencil_format == D3DFMT_UNKNOWN)
		return "No acceptable depth stencil buffer format found";
	return true;
}

result_t
d3ddev_t::confirm_device(const d3d_adapter_device_t& device)
	// Confirm that any required device capabilities are present
{
	if (*c_fullscreen) {
		if (*c_vsync) {	// Make sure vsync is supported
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
