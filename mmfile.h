//-----------------------------------------------------------------------------
// File: mmfile.h
//
// Desc: Memory mapped file
//-----------------------------------------------------------------------------

#ifndef MMFILE_H
#define MMFILE_H

#include "file.h"
#include "win.h"

class mmfile_t : public file_t {
public:
	mmfile_t() : file(INVALID_HANDLE_VALUE), map(INVALID_HANDLE_VALUE), view(0), view_size(0) {}
	mmfile_t(const char* filename) : file(INVALID_HANDLE_VALUE), map(INVALID_HANDLE_VALUE),
		view(0), view_size(0) { open(filename); }
	~mmfile_t() { close(); }

	uint size()			{ return view_size; }
	ubyte* data()		{ return view; }
	bool valid()		{ return (view != NULL); }
	void close();
	bool open(const char* filename);

protected:
	HANDLE file;	// Handle to the file object
	HANDLE map;		// Handle to the memory mapped file
	ubyte* view;	// Pointer to the files contents
	uint view_size;	// Total size of the archive
};

#endif
