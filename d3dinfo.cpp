//-----------------------------------------------------------------------------
// File: d3dinfo.cpp
//
// Wrapper class for the IDirect3D8 interface, basically just exists to keep 
// all the direct 3d initialization code seperate from the rendering code
//-----------------------------------------------------------------------------

#include "d3dinfo.h"
#include <memory>
#include "exec.h"
#include "mem.h"
#include <d3dx8.h>
#include <stdlib.h>
#include "console.h"
#include "d3dlookup.h"

#define new mem_new

using std::auto_ptr;

#undef d3d

cvar_int_t c_d3d_sdk_version("d3d_sdk_version", D3D_SDK_VERSION, CVF_CONST | CVF_HIDDEN);

namespace {
	int compare_modes(const void* a, const void* b) 
		// Compare modes for sorting, sort by format, width, height in that order
	{
		const d3dinfo_t::mode_t* m1 = static_cast<const d3dinfo_t::mode_t*>(a);
		const d3dinfo_t::mode_t* m2 = static_cast<const d3dinfo_t::mode_t*>(b);
		if (m1->format < m2->format)	return -1;
		if (m1->format > m2->format)	return  1;
		if (m1->width < m2->width)		return -1;
		if (m1->width > m2->width)		return  1;
		if (m1->height < m2->height)	return -1;
		if (m1->height > m2->height)	return  1;
		return 0;	// Should never happen
	}
}

result_t
d3dinfo_t::create()
{
	d3d.set(Direct3DCreate8(*c_d3d_sdk_version));
	if (d3d == 0)
		return "Direct3DCreate8() failed";
	
	num_adapters = d3d->GetAdapterCount();
	if (num_adapters == 0)
		return "No adapters present";
	adapters = new adapter_t[num_adapters];

	u_zeromem(adapters, num_adapters * sizeof(adapter_t));

	for (int i = 0; i < num_adapters; ++i)
		if (!adapters[i].create(this, i))
			return result_t::last;
	return true;
}

result_t
d3dinfo_t::adapter_t::create(d3dinfo_t* d3dinfo, int ord)
{
	info = d3dinfo;
	ordinal = ord;
	
	if (FAILED(info->d3d->GetAdapterIdentifier(ordinal, D3DENUM_NO_WHQL_LEVEL, &identifier)))
		return "IDirect3D8->GetAdapterIdentifier() failed";

	D3DDISPLAYMODE mode;
	if (FAILED(info->d3d->GetAdapterDisplayMode(ordinal, &mode)))
		return "IDirect3D8->GetAdapterDisplayMode() failed";

	display_mode.format = mode.Format;
	display_mode.width = mode.Width;
	display_mode.height = mode.Height;

	num_modes = 0;
	int total_modes = info->d3d->GetAdapterModeCount(ordinal);

	for (int i = 0; i < total_modes; ++i) {
		if (FAILED(info->d3d->EnumAdapterModes(ordinal, i, &mode)))
			return "IDirect3D8->EnumAdapterModes() failed";
		for (int j = 0; j < num_modes; ++j) {
			if (modes[j].format == mode.Format
			 && modes[j].width == mode.Width
			 && modes[j].height == mode.Height)
				break;
		}
		if (j == MAX_MODES)	// Should never happen, if it does just forget that mode
			console.printf("adapter_t::MAX_MODES exceeded: mode %dx%d, format %s unavailable\n",
				mode.Width, mode.Height, D3DFORMAT_to_str(mode.Format));
		else if (j == num_modes) {
			// Ignore refresh rates
			modes[j].format = mode.Format;
			modes[j].width = mode.Width;
			modes[j].height = mode.Height;
			++num_modes;
		}
	}
	// Sort the modes array
	qsort(modes, num_modes, sizeof(d3dinfo_t::mode_t), compare_modes);

	hal_device.create(this, D3DDEVTYPE_HAL);
	ref_device.create(this, D3DDEVTYPE_REF);

	if (!hal_device.supported && !ref_device.supported)
		return "No acceptable devices found for the adapter";

	return true;
}

result_t
d3dinfo_t::device_t::create(adapter_t* adap, D3DDEVTYPE devtype)
{
	adapter = adap;
	type = devtype;

	if (SUCCEEDED(adapter->info->d3d->GetDeviceCaps(adapter->ordinal, type, &caps))) {
		supported = true;
		// In order to be supported at least one mode must use this format
		for (int m = 0; m < adapter->num_modes; ++m) {
			if (m == 0 || adapter->modes[m].format != adapter->modes[m - 1].format) {
				if (adapter->modes[m].format == D3DFMT_X1R5G5B5) {
					format_X1R5G5B5_fullscreen.create(this, D3DFMT_X1R5G5B5, true);
					format_X1R5G5B5_windowed.create(this, D3DFMT_X1R5G5B5, false);
				} else if (adapter->modes[m].format == D3DFMT_R5G6B5) {
					format_R5G6B5_fullscreen.create(this, D3DFMT_R5G6B5, true);
					format_R5G6B5_windowed.create(this, D3DFMT_R5G6B5, false);
				} else if (adapter->modes[m].format == D3DFMT_X8R8G8B8) {
					format_X8R8G8B8_fullscreen.create(this, D3DFMT_X8R8G8B8, true);
					format_X8R8G8B8_windowed.create(this, D3DFMT_X8R8G8B8, false);
				} else if (adapter->modes[m].format == D3DFMT_A8R8G8B8) {
					format_A8R8G8B8_fullscreen.create(this, D3DFMT_A8R8G8B8, true);
					format_A8R8G8B8_windowed.create(this, D3DFMT_A8R8G8B8, false);
				}
			}
		}
	} else {
		supported = false;
		return "IDirect3D8->GetDeviceCaps() failed";
	}
	return true;
}

result_t
d3dinfo_t::format_t::create(device_t* dev, D3DFORMAT fmt, bool fullscr)
{
	device = dev;
	format = fmt;
	fullscreen = fullscr;
	back_buffer_format.back_buffer_format = format;

	D3DFORMAT alpha_fmt = D3DFMT_UNKNOWN;
	if (format == D3DFMT_X1R5G5B5)
		alpha_fmt = D3DFMT_A1R5G5B5;
	else if (format == D3DFMT_X8R8G8B8)
		alpha_fmt = D3DFMT_A8R8G8B8;

	if (back_buffer_format.create(this, format))
		supported = true;
	if (alpha_format.create(this, alpha_fmt))
		supported = true;
	if (!supported)
		return "Format not supported";

	// Check the texture formats
	const D3DFORMAT formats[] = {
		D3DFMT_A1R5G5B5,	D3DFMT_A2B10G10R10,	D3DFMT_A4L4,	D3DFMT_A4R4G4B4,
		D3DFMT_A8,			D3DFMT_A8L8,		D3DFMT_A8P8,	D3DFMT_A8R3G3B2,
		D3DFMT_A8R8G8B8,	D3DFMT_G16R16,		D3DFMT_L8,		D3DFMT_P8,
		D3DFMT_R3G3B2,		D3DFMT_R5G6B5,		D3DFMT_R8G8B8,	D3DFMT_X1R5G5B5,
		D3DFMT_X4R4G4B4,	D3DFMT_X8R8G8B8,
	};
	u_assert(MAX_TEXTURE_FORMATS == sizeof(formats) / sizeof(*formats));
	u_assert(MAX_RENDER_FORMATS == sizeof(formats) / sizeof(*formats));
	u_assert(MAX_CUBE_FORMATS == sizeof(formats) / sizeof(*formats));
	u_assert(MAX_VOLUME_FORMATS == sizeof(formats) / sizeof(*formats));

	num_texture_formats = 0;
	num_render_formats = 0;
	num_cube_formats = 0;
	num_volume_formats = 0;

	for (int i = 0; i < MAX_TEXTURE_FORMATS; ++i) {
		HRESULT hr;

		// Check this format for textures
		hr = device->adapter->info->d3d->CheckDeviceFormat(
			device->adapter->ordinal, 
			device->type,
			format,
			0,
			D3DRTYPE_TEXTURE,
			formats[i]
		);
		if (SUCCEEDED(hr))
			texture_formats[num_texture_formats++] = formats[i];
		
		// Check this format for render targets
		hr = device->adapter->info->d3d->CheckDeviceFormat(
			device->adapter->ordinal, 
			device->type,
			format,
			D3DUSAGE_RENDERTARGET,
			D3DRTYPE_TEXTURE,
			formats[i]
		);
		if (SUCCEEDED(hr))
			render_formats[num_render_formats++] = formats[i];
		
		// Check this format for cube textures
		hr = device->adapter->info->d3d->CheckDeviceFormat(
			device->adapter->ordinal, 
			device->type,
			format,
			0,
			D3DRTYPE_CUBETEXTURE,
			formats[i]
		);
		if (SUCCEEDED(hr))
			cube_formats[num_cube_formats++] = formats[i];
		
		// Check this format for volume textures
		hr = device->adapter->info->d3d->CheckDeviceFormat(
			device->adapter->ordinal, 
			device->type,
			format,
			0,
			D3DRTYPE_VOLUMETEXTURE,
			formats[i]
		);
		if (SUCCEEDED(hr))
			volume_formats[num_volume_formats++] = formats[i];
	}

	if (num_texture_formats == 0)
		return "No supported texture formats found";

	return true;
}

result_t
d3dinfo_t::back_buffer_format_t::create(format_t* fmt, D3DFORMAT bbfmt)
{
	format = fmt;
	back_buffer_format = bbfmt;

	if (back_buffer_format == D3DFMT_UNKNOWN) {
		supported = false;
		return "Invalid back buffer format";
	}
	HRESULT hr = format->device->adapter->info->d3d->CheckDeviceType(
		format->device->adapter->ordinal,
		format->device->type,
		format->format,
		back_buffer_format,
		format->fullscreen ? TRUE : FALSE
	);
	supported = SUCCEEDED(hr);

	// Check for supported depth/stencil formats
	const D3DFORMAT dsfmts[] = {
		D3DFMT_D32, D3DFMT_D15S1, D3DFMT_D24S8,
		D3DFMT_D16, D3DFMT_D24X8, D3DFMT_D24X4S4,
	};
	u_assert(MAX_DEPTH_STENCIL_FORMATS == sizeof(dsfmts) / sizeof(*dsfmts));

	num_depth_stencil_formats = 0;

	for (int i = 0; i < MAX_DEPTH_STENCIL_FORMATS; ++i) {
	    HRESULT hr;
		hr = format->device->adapter->info->d3d->CheckDeviceFormat(
			format->device->adapter->ordinal,
			format->device->type, 
			format->format,
			D3DUSAGE_DEPTHSTENCIL,
			D3DRTYPE_SURFACE,
			dsfmts[i]
		);
		if (FAILED(hr))
			continue;	// This depth stencil format is not supported
		hr = format->device->adapter->info->d3d->CheckDepthStencilMatch(
			format->device->adapter->ordinal,
			format->device->type, 
			format->format,
			back_buffer_format,
			dsfmts[i]
		);
		if (SUCCEEDED(hr))
			depth_stencil_formats[num_depth_stencil_formats++] = dsfmts[i];
	}

	// Check for supported multisample types
	const D3DMULTISAMPLE_TYPE mstypes[] = {
		D3DMULTISAMPLE_NONE,		D3DMULTISAMPLE_2_SAMPLES,	D3DMULTISAMPLE_3_SAMPLES,
		D3DMULTISAMPLE_4_SAMPLES,	D3DMULTISAMPLE_5_SAMPLES,	D3DMULTISAMPLE_6_SAMPLES,
		D3DMULTISAMPLE_7_SAMPLES,	D3DMULTISAMPLE_8_SAMPLES,	D3DMULTISAMPLE_9_SAMPLES,
		D3DMULTISAMPLE_10_SAMPLES,	D3DMULTISAMPLE_11_SAMPLES,	D3DMULTISAMPLE_12_SAMPLES,
		D3DMULTISAMPLE_13_SAMPLES,	D3DMULTISAMPLE_14_SAMPLES,	D3DMULTISAMPLE_15_SAMPLES,
		D3DMULTISAMPLE_16_SAMPLES,
	};
	u_assert(MAX_MULTISAMPLE_TYPES == sizeof(mstypes) / sizeof(*mstypes));

	num_multisample_types = 0;

	for (i = 0; i < MAX_MULTISAMPLE_TYPES; ++i) {
		HRESULT hr = format->device->adapter->info->d3d->CheckDeviceMultiSampleType(
			format->device->adapter->ordinal,
			format->device->type, 
			format->format, 
			format->fullscreen ? FALSE : TRUE,
			mstypes[i]
		);
		if (SUCCEEDED(hr))
			multisample_types[num_multisample_types++] = mstypes[i];
	}

	if (num_depth_stencil_formats == 0)
		return "No supported depth/stencil buffer formats found";

	return true;
}

