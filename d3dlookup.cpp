//-----------------------------------------------------------------------------
// File: d3dlookup.cpp
//
// Convert text to D3D values and vice versa
//-----------------------------------------------------------------------------

#include "util.h"
#include <d3d8.h>

const char*
D3DFORMAT_to_str(const D3DFORMAT format)
	// Get a text name for a D3DFORMAT
{
	switch (format) {
	default:
	case D3DFMT_UNKNOWN:	 return "D3DFMT_UNKNOWN";
	case D3DFMT_R8G8B8:		 return "D3DFMT_R8G8B8";
	case D3DFMT_A8R8G8B8:	 return "D3DFMT_A8R8G8B8";
	case D3DFMT_X8R8G8B8:	 return "D3DFMT_X8R8G8B8";
	case D3DFMT_R5G6B5:		 return "D3DFMT_R5G6B5";
	case D3DFMT_X1R5G5B5:	 return "D3DFMT_X1R5G5B5";
	case D3DFMT_A1R5G5B5:	 return "D3DFMT_A1R5G5B5";
	case D3DFMT_A4R4G4B4:	 return "D3DFMT_A4R4G4B4";
	case D3DFMT_A8:			 return "D3DFMT_A8";
	case D3DFMT_R3G3B2:		 return "D3DFMT_R3G3B2";
	case D3DFMT_A8R3G3B2:	 return "D3DFMT_A8R3G3B2";
	case D3DFMT_X4R4G4B4:	 return "D3DFMT_X4R4G4B4";
	case D3DFMT_A2B10G10R10: return "D3DFMT_A2B10G10R10";
	case D3DFMT_G16R16:		 return "D3DFMT_G16R16";
	case D3DFMT_A8P8:		 return "D3DFMT_A8P8";
	case D3DFMT_P8:			 return "D3DFMT_P8";
	case D3DFMT_L8:			 return "D3DFMT_L8";
	case D3DFMT_A8L8:		 return "D3DFMT_A8L8";
	case D3DFMT_A4L4:		 return "D3DFMT_A4L4";
	case D3DFMT_V8U8:		 return "D3DFMT_V8U8";
	case D3DFMT_Q8W8V8U8:	 return "D3DFMT_Q8W8V8U8";
	case D3DFMT_V16U16:		 return "D3DFMT_V16U16";
	case D3DFMT_W11V11U10:	 return "D3DFMT_W11V11U10";
	case D3DFMT_L6V5U5:		 return "D3DFMT_L6V5U5";
	case D3DFMT_X8L8V8U8:	 return "D3DFMT_X8L8V8U8";
	case D3DFMT_A2W10V10U10: return "D3DFMT_A2W10V10U10";
	case D3DFMT_UYVY:		 return "D3DFMT_UYVY";
	case D3DFMT_YUY2:		 return "D3DFMT_YUY2";
	case D3DFMT_DXT1:		 return "D3DFMT_DXT1";
	case D3DFMT_DXT2:		 return "D3DFMT_DXT2";
	case D3DFMT_DXT3:		 return "D3DFMT_DXT3";
	case D3DFMT_DXT4:		 return "D3DFMT_DXT4";
	case D3DFMT_DXT5:		 return "D3DFMT_DXT5";
	case D3DFMT_VERTEXDATA:	 return "D3DFMT_VERTEXDATA";
	case D3DFMT_INDEX16:	 return "D3DFMT_INDEX16";
	case D3DFMT_INDEX32:	 return "D3DFMT_INDEX32";
	case D3DFMT_D16_LOCKABLE:return "D3DFMT_D16_LOCKABLE";
	case D3DFMT_D32:		 return "D3DFMT_D32";
	case D3DFMT_D15S1:		 return "D3DFMT_D15S1";
	case D3DFMT_D24S8:		 return "D3DFMT_D24S8";
	case D3DFMT_D16:		 return "D3DFMT_D16";
	case D3DFMT_D24X8:		 return "D3DFMT_D24X8";
	case D3DFMT_D24X4S4:	 return "D3DFMT_D24X4S4";
	};
}

D3DFORMAT
str_to_D3DFORMAT(const char* str)
	// Convert a string to a D3DFORMAT
{
	if (!u_strncmp(str, "D3DFMT_", 7)) {
		str += 7;
		if (!u_strcmp(str, "R8G8B8"))		return D3DFMT_R8G8B8;
		if (!u_strcmp(str, "A8R8G8B8"))		return D3DFMT_A8R8G8B8;
		if (!u_strcmp(str, "X8R8G8B8"))		return D3DFMT_X8R8G8B8;
		if (!u_strcmp(str, "R5G6B5"))		return D3DFMT_R5G6B5;
		if (!u_strcmp(str, "X1R5G5B5"))		return D3DFMT_X1R5G5B5;
		if (!u_strcmp(str, "A1R5G5B5"))		return D3DFMT_A1R5G5B5;
		if (!u_strcmp(str, "A4R4G4B4"))		return D3DFMT_A4R4G4B4;
		if (!u_strcmp(str, "A8"))			return D3DFMT_A8;
		if (!u_strcmp(str, "R3G3B2"))		return D3DFMT_R3G3B2;
		if (!u_strcmp(str, "A8R3G3B2"))		return D3DFMT_A8R3G3B2;
		if (!u_strcmp(str, "X4R4G4B4"))		return D3DFMT_X4R4G4B4;
		if (!u_strcmp(str, "A2B10G10R10"))	return D3DFMT_A2B10G10R10;
		if (!u_strcmp(str, "G16R16"))		return D3DFMT_G16R16;
		if (!u_strcmp(str, "A8P8"))			return D3DFMT_A8P8;
		if (!u_strcmp(str, "P8"))			return D3DFMT_P8;
		if (!u_strcmp(str, "L8"))			return D3DFMT_L8;
		if (!u_strcmp(str, "A8L8"))			return D3DFMT_A8L8;
		if (!u_strcmp(str, "A4L4"))			return D3DFMT_A4L4;
		if (!u_strcmp(str, "V8U8"))			return D3DFMT_V8U8;
		if (!u_strcmp(str, "Q8W8V8U8"))		return D3DFMT_Q8W8V8U8;
		if (!u_strcmp(str, "V16U16"))		return D3DFMT_V16U16;
		if (!u_strcmp(str, "W11V11U10"))	return D3DFMT_W11V11U10;
		if (!u_strcmp(str, "L6V5U5"))		return D3DFMT_L6V5U5;
		if (!u_strcmp(str, "X8L8V8U8"))		return D3DFMT_X8L8V8U8;
		if (!u_strcmp(str, "A2W10V10U10"))	return D3DFMT_A2W10V10U10;
		if (!u_strcmp(str, "UYVY"))			return D3DFMT_UYVY;
		if (!u_strcmp(str, "YUY2"))			return D3DFMT_YUY2;
		if (!u_strcmp(str, "DXT1"))			return D3DFMT_DXT1;
		if (!u_strcmp(str, "DXT2"))			return D3DFMT_DXT2;
		if (!u_strcmp(str, "DXT3"))			return D3DFMT_DXT3;
		if (!u_strcmp(str, "DXT4"))			return D3DFMT_DXT4;
		if (!u_strcmp(str, "DXT5"))			return D3DFMT_DXT5;
		if (!u_strcmp(str, "VERTEXDATA"))	return D3DFMT_VERTEXDATA;
		if (!u_strcmp(str, "INDEX16"))		return D3DFMT_INDEX16;
		if (!u_strcmp(str, "INDEX32"))		return D3DFMT_INDEX32;
		if (!u_strcmp(str, "D16_LOCKABLE"))	return D3DFMT_D16_LOCKABLE;
		if (!u_strcmp(str, "D32"))			return D3DFMT_D32;
		if (!u_strcmp(str, "D15S1"))		return D3DFMT_D15S1;
		if (!u_strcmp(str, "D24S8"))		return D3DFMT_D24S8;
		if (!u_strcmp(str, "D16"))			return D3DFMT_D16;
		if (!u_strcmp(str, "D24X8"))		return D3DFMT_D24X8;
		if (!u_strcmp(str, "D24X4S4"))		return D3DFMT_D24X4S4;
	}
	return D3DFMT_UNKNOWN;
}

const char*
D3DMULTISAMPLE_TYPE_to_str(const D3DMULTISAMPLE_TYPE type) 
	// Get a text representation of a D3DMULTISAMPLE_TYPE
{
	switch (type) {
	default:
	case D3DMULTISAMPLE_NONE:		return "D3DMULTISAMPLE_NONE";
	case D3DMULTISAMPLE_2_SAMPLES:	return "D3DMULTISAMPLE_2_SAMPLES";
	case D3DMULTISAMPLE_3_SAMPLES:	return "D3DMULTISAMPLE_3_SAMPLES";
	case D3DMULTISAMPLE_4_SAMPLES:	return "D3DMULTISAMPLE_4_SAMPLES";
	case D3DMULTISAMPLE_5_SAMPLES:	return "D3DMULTISAMPLE_5_SAMPLES";
	case D3DMULTISAMPLE_6_SAMPLES:	return "D3DMULTISAMPLE_6_SAMPLES";
	case D3DMULTISAMPLE_7_SAMPLES:	return "D3DMULTISAMPLE_7_SAMPLES";
	case D3DMULTISAMPLE_8_SAMPLES:	return "D3DMULTISAMPLE_8_SAMPLES";
	case D3DMULTISAMPLE_9_SAMPLES:	return "D3DMULTISAMPLE_9_SAMPLES";
	case D3DMULTISAMPLE_10_SAMPLES:	return "D3DMULTISAMPLE_10_SAMPLES";
	case D3DMULTISAMPLE_11_SAMPLES:	return "D3DMULTISAMPLE_11_SAMPLES";
	case D3DMULTISAMPLE_12_SAMPLES:	return "D3DMULTISAMPLE_12_SAMPLES";
	case D3DMULTISAMPLE_13_SAMPLES:	return "D3DMULTISAMPLE_13_SAMPLES";
	case D3DMULTISAMPLE_14_SAMPLES:	return "D3DMULTISAMPLE_14_SAMPLES";
	case D3DMULTISAMPLE_15_SAMPLES:	return "D3DMULTISAMPLE_15_SAMPLES";
	case D3DMULTISAMPLE_16_SAMPLES:	return "D3DMULTISAMPLE_16_SAMPLES";
	};
};

D3DMULTISAMPLE_TYPE
str_to_D3DMULTISAMPLE_TYPE(const char* str)
	// Convert a D3DMULTISAMPLE_TYPE name into the appropriate value
{
	if (!u_strncmp(str, "D3DMULTISAMPLE_", 15)) {
		str += 15;
		if (!u_strcmp(str, "2_SAMPLES"))	return D3DMULTISAMPLE_2_SAMPLES;
		if (!u_strcmp(str, "3_SAMPLES"))	return D3DMULTISAMPLE_3_SAMPLES;
		if (!u_strcmp(str, "4_SAMPLES"))	return D3DMULTISAMPLE_4_SAMPLES;
		if (!u_strcmp(str, "5_SAMPLES"))	return D3DMULTISAMPLE_5_SAMPLES;
		if (!u_strcmp(str, "6_SAMPLES"))	return D3DMULTISAMPLE_6_SAMPLES;
		if (!u_strcmp(str, "7_SAMPLES"))	return D3DMULTISAMPLE_7_SAMPLES;
		if (!u_strcmp(str, "8_SAMPLES"))	return D3DMULTISAMPLE_8_SAMPLES;
		if (!u_strcmp(str, "9_SAMPLES"))	return D3DMULTISAMPLE_9_SAMPLES;
		if (!u_strcmp(str, "10_SAMPLES"))	return D3DMULTISAMPLE_10_SAMPLES;
		if (!u_strcmp(str, "11_SAMPLES"))	return D3DMULTISAMPLE_11_SAMPLES;
		if (!u_strcmp(str, "12_SAMPLES"))	return D3DMULTISAMPLE_12_SAMPLES;
		if (!u_strcmp(str, "13_SAMPLES"))	return D3DMULTISAMPLE_13_SAMPLES;
		if (!u_strcmp(str, "14_SAMPLES"))	return D3DMULTISAMPLE_14_SAMPLES;
		if (!u_strcmp(str, "15_SAMPLES"))	return D3DMULTISAMPLE_15_SAMPLES;
		if (!u_strcmp(str, "16_SAMPLES"))	return D3DMULTISAMPLE_16_SAMPLES;
	}
	return D3DMULTISAMPLE_NONE;
}

// Parse and write D3DBLENDs
const char*
D3DBLEND_to_str(const D3DBLEND blend)
	// Get a text representation of a D3DBLEND
{
	switch (blend) {
	case D3DBLEND_ZERO:				return "D3DBLEND_ZERO";
	case D3DBLEND_ONE:				return "D3DBLEND_ONE";
	case D3DBLEND_SRCCOLOR:			return "D3DBLEND_SRCCOLOR";
	case D3DBLEND_INVSRCCOLOR:		return "D3DBLEND_INVSRCCOLOR";
	case D3DBLEND_SRCALPHA:			return "D3DBLEND_SRCALPHA";
	case D3DBLEND_INVSRCALPHA:		return "D3DBLEND_INVSRCALPHA";
	case D3DBLEND_DESTALPHA:		return "D3DBLEND_DESTALPHA";
	case D3DBLEND_INVDESTALPHA:		return "D3DBLEND_INVDESTALPHA";
	case D3DBLEND_DESTCOLOR:		return "D3DBLEND_DESTCOLOR";
	case D3DBLEND_INVDESTCOLOR:		return "D3DBLEND_INVDESTCOLOR";
	case D3DBLEND_SRCALPHASAT:		return "D3DBLEND_SRCALPHASAT";
	case D3DBLEND_BOTHSRCALPHA:		return "D3DBLEND_BOTHSRCALPHA";
	case D3DBLEND_BOTHINVSRCALPHA:	return "D3DBLEND_BOTHINVSRCALPHA";
	default:						return "Invalid_D3DBLEND";
	};
}

D3DBLEND
str_to_D3DBLEND(const char* str)
	// Convert a D3DBLEND name into the appropriate value
{
	if (!u_strncmp(str, "D3DBLEND_", 9)) {
		str += 9;
		if (!u_strcmp(str, "ZERO"))				return D3DBLEND_ZERO;
		if (!u_strcmp(str, "ONE"))				return D3DBLEND_ONE;
		if (!u_strcmp(str, "SRCCOLOR"))			return D3DBLEND_SRCCOLOR;
		if (!u_strcmp(str, "INVSRCCOLOR"))		return D3DBLEND_INVSRCCOLOR;
		if (!u_strcmp(str, "SRCALPHA"))			return D3DBLEND_SRCALPHA;
		if (!u_strcmp(str, "INVSRCALPHA"))		return D3DBLEND_INVSRCALPHA;
		if (!u_strcmp(str, "DESTALPHA"))		return D3DBLEND_DESTALPHA;
		if (!u_strcmp(str, "INVDESTALPHA"))		return D3DBLEND_INVDESTALPHA;
		if (!u_strcmp(str, "DESTCOLOR"))		return D3DBLEND_DESTCOLOR;
		if (!u_strcmp(str, "INVDESTCOLOR"))		return D3DBLEND_INVDESTCOLOR;
		if (!u_strcmp(str, "SRCALPHASAT"))		return D3DBLEND_SRCALPHASAT;
		if (!u_strcmp(str, "BOTHSRCALPHA"))		return D3DBLEND_BOTHSRCALPHA;
		if (!u_strcmp(str, "BOTHINVSRCALPHA"))	return D3DBLEND_BOTHINVSRCALPHA;
	};
	return D3DBLEND_ZERO;	// No unknown value available
}
