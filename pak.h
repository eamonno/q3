//-----------------------------------------------------------------------------
// File: pak.h
//
// Manages all of the opened zip archives
//-----------------------------------------------------------------------------

#ifndef PAK_H
#define PAK_H

#include "zip.h"
#include "util.h"

class pak_file_finder_t {
	// Finds any files whose names match the wildcarded version in the pak
public:
	int search(const char* dir, const char* mask);
	int count() { return names.size(); }
	const char* operator[](int i) const { return names[i]; }
private:
	vector_t<filename_t> names;
};

class pak_t {
	// Several zip files representing all the files available to the app
public:
	friend class pak_file_finder_t;

	~pak_t();

	result_t init();
	bool add_archive(const char* name);
	result_t scan_dir(const char* dir);
	file_t* open_file(const char* name);
	bool file_exists(const char* name);

	static pak_t& get_instance();

private:
	pak_t();

	vector_t<zip_t*> zips;
};

#define pak (pak_t::get_instance())

#endif

