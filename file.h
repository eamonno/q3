//-----------------------------------------------------------------------------
// File: file.h
//
// Input files. Abstract base class for any input files used throughout the
// program. Also includes the file_finder_t class for finding matches for a
// particular file name in one or more directories
//-----------------------------------------------------------------------------

#ifndef FILE_H
#define FILE_H

#include "types.h"
#include "str.h"


typedef str_t<64> filename_t;

class file_t {
public:
	virtual ~file_t() {}
	virtual uint size() = 0;
	virtual ubyte* data() = 0;
	virtual void close() = 0;
	virtual bool valid() = 0;
};

class file_finder_t {
public:
	file_finder_t() : next_name(0), next_pos(0) {}
	file_finder_t(const char* dir, const char* mask) : next_name(0), next_pos(0)
		{ search(dir, mask); }

	int search(const char* dir, const char* mask);
	int count() const { return next_name; }
	const char* operator[](int i) const { return names[i]; }

private:
	enum { MAX_FILE_FINDER_FILES = 512 };
	enum { FILE_FINDER_NAME_BUF_SIZE = 8192 };

	char* names[MAX_FILE_FINDER_FILES];
	char name_buf[FILE_FINDER_NAME_BUF_SIZE];

	int next_name;
	int next_pos;
};

#endif
