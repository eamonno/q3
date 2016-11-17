//-----------------------------------------------------------------------------
// File: win.cpp
//
// All the windows stuff goes in here
//-----------------------------------------------------------------------------

#include "win.h"
#include "app.h"
#include "util.h"
#include "console.h"
#include "input.h"

#include "mem.h"
#define new mem_new

HWND hwnd;
HINSTANCE hinstance;

bool created = false;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE prev, LPSTR cmd_line, INT cmd_show)
	// WinMain, program entry point, create the app window then create the app
{
	atexit(mem_report);

	// First set up wll the windows stuff
	WNDCLASSEX wndclass;
	wndclass.cbSize			= sizeof(WNDCLASSEX);
	wndclass.style			= CS_CLASSDC;
	wndclass.lpfnWndProc	= WndProc;
	wndclass.cbClsExtra		= 0L;
	wndclass.cbWndExtra		= 0L;
	wndclass.hInstance		= GetModuleHandle(NULL);
	wndclass.hIcon			= NULL;
	wndclass.hCursor		= NULL;
	wndclass.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.lpszMenuName	= NULL;
	wndclass.lpszClassName	= APP_NAME;
	wndclass.hIconSm		= NULL;

	RegisterClassEx(&wndclass);

	hwnd = CreateWindow(APP_NAME, APP_NAME, WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
						GetDesktopWindow(), NULL, wndclass.hInstance, NULL);
	hinstance = instance;

	ShowWindow(hwnd, cmd_show);
	UpdateWindow(hwnd);

	ShowCursor(FALSE);

	// Confine the cursor to the client area
	RECT rect;
	GetWindowRect(hwnd, &rect);
#ifndef _DEBUG
//	ClipCursor(&rect);
#endif
	POINT centre = {(rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2 };
	SetCursorPos(centre.x, centre.y);

	while (!created);	// Cant initialize D3D while processing WM_CREATE

	// Now initialize the app itself
	if (app.create(cmd_line)) {
	    MSG msg = { NULL, WM_NULL, 0, 0, 0, {0, 0}};
	
		// Main message pump
	    while(msg.message != WM_QUIT)
	    {
			POINT mouse_pos;
			GetCursorPos(&mouse_pos);
			if (hwnd == GetForegroundWindow() && (mouse_pos.x != centre.x || mouse_pos.y != centre.y)) {
				mouse.move(mouse_pos.x - centre.x, mouse_pos.y - centre.y);
				SetCursorPos((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
			}
			if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
			{	// If any windows messages are incoming process them
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			} else {
				app.frame();
			}
		}
	} else {
		MessageBox(hwnd, result_t::last, "App Creation Failure", MB_OK);
	}

	// Unclip the cursor
	ClipCursor(NULL);

	return 0;
}

LRESULT CALLBACK WINAPI
WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	// Windows message proceedure for the app
{
	static bool create_recieved = false;
	static bool showhide = false;
	// Bit messy but gurantees that WM_CREATE is fully processed
	// before created is set to true
	if (create_recieved)
		created = true;

	switch (msg)
	{
	case WM_CHAR:
		while (lparam-- & 0xff)
			keyboard.char_typed(wparam);
		return 0;
	case WM_CREATE:
		create_recieved = true;
		return TRUE;
	case WM_DESTROY:
		app.destroy();
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		ValidateRect(hwnd, NULL);
		return 0;
	case WM_SETCURSOR:
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return TRUE;
	case WM_KEYDOWN:
		switch (wparam) {
		default:
			while (lparam-- & 0xff)
				keyboard.key_down(wparam);
			break;
		case VK_ESCAPE:
			PostQuitMessage(0);
		};
		return 0;
	case WM_KEYUP:
		keyboard.key_up(wparam);
		return 0;
	case WM_LBUTTONDOWN:
		mouse.button_down(mouse_t::BUTTON1);
		return 0;
	case WM_LBUTTONUP:
		mouse.button_up(mouse_t::BUTTON1);
		return 0;
	case WM_MBUTTONDOWN:
		mouse.button_down(mouse_t::BUTTON2);
		return 0;
	case WM_MBUTTONUP:
		mouse.button_up(mouse_t::BUTTON2);
		return 0;
	case WM_RBUTTONDOWN:
		mouse.button_down(mouse_t::BUTTON3);
		return 0;
	case WM_RBUTTONUP:
		mouse.button_up(mouse_t::BUTTON3);
		return 0;
	case WM_MOUSEMOVE:	// Processed in winmain
		return 0;
	};
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
