//-----------------------------------------------------------------------------
// File: d3dinfo.h
//
// Wrapper classes for the IDirect3D8 interface, basically just exists to keep
// all of the DirectX initialization code out of d3d.h
//-----------------------------------------------------------------------------

#ifndef D3DINFO_H
#define D3DINFO_H

#include "camera.h"
#include "dxcommon.h"
#include "util.h"
#include "displaylist.h"
#include "shader_parser.h"
#include "str.h"
#include "types.h"
#include <d3d8.h>

class d3dinfo_t {
	// Stores the Direct3D interface pointer and information about the 
	// adapters that are available
public:
	struct format_t;
	struct device_t;
	struct mode_t;
	struct adapter_t;

	struct back_buffer_format_t {
		enum { MAX_MULTISAMPLE_TYPES = 16 };
		enum { MAX_DEPTH_STENCIL_FORMATS = 6 };

		// Back buffer format available to a given format
		format_t* format;
		D3DFORMAT back_buffer_format;
		bool supported;

		int num_multisample_types;
		D3DMULTISAMPLE_TYPE multisample_types[MAX_MULTISAMPLE_TYPES];
		int num_depth_stencil_formats;
		D3DFORMAT depth_stencil_formats[MAX_DEPTH_STENCIL_FORMATS];

		result_t create(format_t* fmt, D3DFORMAT bbfmt);
	};

	struct format_t {
		enum { MAX_TEXTURE_FORMATS = 18 };
		enum { MAX_RENDER_FORMATS = 18 };
		enum { MAX_CUBE_FORMATS = 18 };
		enum { MAX_VOLUME_FORMATS = 18 };

		bool supported;
		bool fullscreen;

		device_t* device;
		D3DFORMAT format;

		int num_texture_formats;
		int num_render_formats;
		int num_cube_formats;
		int num_volume_formats;

		D3DFORMAT texture_formats[MAX_TEXTURE_FORMATS];
		D3DFORMAT render_formats[MAX_RENDER_FORMATS];
		D3DFORMAT cube_formats[MAX_CUBE_FORMATS];
		D3DFORMAT volume_formats[MAX_VOLUME_FORMATS];

		// There are at most 2 back buffer formats per adapter format
		back_buffer_format_t back_buffer_format;
		back_buffer_format_t alpha_format;

		result_t create(device_t* dev, D3DFORMAT fmt, bool fullscr);
	};

	struct device_t {
		bool supported;
		adapter_t* adapter;
		D3DDEVTYPE type;
		D3DCAPS8 caps;

		// There are 8 possible render target formats, 4 buffer formats, and
		// each exists fullscreen and windowed
		format_t format_X1R5G5B5_fullscreen;
		format_t format_X1R5G5B5_windowed;
		format_t format_R5G6B5_fullscreen;
		format_t format_R5G6B5_windowed;
		format_t format_X8R8G8B8_fullscreen;
		format_t format_X8R8G8B8_windowed;
		format_t format_A8R8G8B8_fullscreen;
		format_t format_A8R8G8B8_windowed;

		result_t create(adapter_t* adap, D3DDEVTYPE devtype);
	};

	struct mode_t {
		D3DFORMAT format;
		int width;
		int height;
	};

	struct adapter_t {
		enum { MAX_MODES = 128 };

		d3dinfo_t* info;
		int	ordinal;
		int num_modes;
		D3DADAPTER_IDENTIFIER8 identifier;

		mode_t display_mode;
		mode_t modes[MAX_MODES];

		device_t hal_device;
		device_t ref_device;

		result_t create(d3dinfo_t* info, int ord);
	};

	d3dinfo_t() : num_adapters(0), adapters(0) {}
	~d3dinfo_t()	{ destroy(); }

	IDirect3D8*	operator->() { return d3d.get(); }

	result_t create();
	void destroy() { delete [] adapters; adapters = 0; d3d = 0; }

	com_ptr_t<IDirect3D8> d3d;
	int	num_adapters;
	adapter_t* adapters;
};

#endif
