//-----------------------------------------------------------------------------
// File: win.h
//
// Windows specific stuff
//-----------------------------------------------------------------------------

#ifndef WIN_H
#define WIN_H

#define STRICT 1
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef min
#undef max

extern HWND hwnd;
extern HINSTANCE hinstance;

#endif
