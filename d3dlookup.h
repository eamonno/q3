//-----------------------------------------------------------------------------
// File: d3dlookup.h
//
// Look up the text names for various D3D constants, and get the constants for
// the text names supplied
//-----------------------------------------------------------------------------

#ifndef D3DLOOKUP_H
#define D3DLOOKUP_H

// Parse and write D3DFORMATs
const char* D3DFORMAT_to_str(const D3DFORMAT format);
D3DFORMAT str_to_D3DFORMAT(const char* str);

// Parse and write D3DMULTISAMPLE_TYPEs
const char* D3DMULTISAMPLE_TYPE_to_str(const D3DMULTISAMPLE_TYPE type);
D3DMULTISAMPLE_TYPE str_to_D3DMULTISAMPLE_TYPE(const char* str);

// Parse and write D3DBLENDs
const char* D3DBLEND_to_str(const D3DBLEND blend);
D3DBLEND str_to_D3DBLEND(const char* str);

#endif