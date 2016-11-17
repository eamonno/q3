//-----------------------------------------------------------------------------
// File: mmfile.cpp
//
// Memory mapped files
//-----------------------------------------------------------------------------

#include "mmfile.h"

#include "console.h"

#include "mem.h"
#define new mem_new

namespace {
	const int MAX_ERROR_STRING_LENGTH = 1024;
}

bool
mmfile_t::open(const char* filename)
	// Open the file, create a file mapping for it and map it into memory. Note
	// that only files with a size greater than 0 may be memory mapped
{
	if (file != INVALID_HANDLE_VALUE)
		close();

	// Open the file
	if ((file = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
		// Dont map 0 length files
		if ((view_size = GetFileSize(file, NULL)) != 0)
			// Create a file mapping object
			if ((map = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL)) != INVALID_HANDLE_VALUE)
				// Memory map the file
				if ((view = static_cast<ubyte*>(MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0))) != NULL)
					return true;
				else {
					char errbuffer[MAX_ERROR_STRING_LENGTH];
					if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
					  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					  reinterpret_cast<char*>(&errbuffer), MAX_ERROR_STRING_LENGTH, NULL)) {
						console.printf("Failed to open %s, %s\n", filename, errbuffer);
					}
				}

	close();
	return false;
}

void
mmfile_t::close()
	// Unmap the view of the files contents and close the memory map handle
	// and the file handle	
{
	if (view) {
		UnmapViewOfFile(view);
		view = 0;
	}
	if (map != INVALID_HANDLE_VALUE) {
		CloseHandle(map);
		map = INVALID_HANDLE_VALUE;
	}
	if (file != INVALID_HANDLE_VALUE) {
		CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
	}
	view_size = 0;
}
