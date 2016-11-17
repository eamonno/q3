//-----------------------------------------------------------------------------
// File: zip.h
//
// Class for handling of zip archives. 
//-----------------------------------------------------------------------------

#ifndef ZIP_H
#define ZIP_H

#include "mmfile.h"
#include "ziptypes.h"
#include "util.h"

class zip_file_t : public file_t {
	// A single file within the archive
public:
	friend class zip_t;

	zip_file_t() : _entry(NULL), _data(NULL) {}
	zip_file_t(const zip_file_t& z) : _entry(z._entry), _data(NULL) {}
	~zip_file_t() { close(); }

	uint size() { return _entry->uncompressed_size(); }
	ubyte* data();
	void close();
	bool valid() { return _entry == NULL; }

private:
	zip_file_t(zip_entry_t* z) : _entry(z), _data(NULL) {}
	ubyte* _data;
	zip_entry_t* _entry;
};

class zip_t : public mmfile_t {
	// Represents a zip archive
public:
	zip_t() : zip_dir_entries(0), zip_entries(0), zip_dir(0), index(0), names(0) {}
	zip_t(const char* filename) : zip_dir_entries(0), zip_entries(0),
		zip_dir(0), index(0), names(0)
	{ open(filename); }
	~zip_t() { close(); }

	bool open(const char* filename);
	void close();

	int num_entries() const							{ return zip_dir->total_entries(); }
	const filename_t& entry_name(int entry) const	{ return names[index[entry]]; }

	zip_entry_t** contents()						{ return zip_entries; }
	bool file_exists(const char* name);
	zip_file_t* open_file(const char* filename);

private:
	int find_index(const char* name);

	filename_t* names;	// Names of all the files (unsorted)
	int*		index;	// Indexes for sorted names

	zip_dir_entry_t** zip_dir_entries;	// All the dir entries (same order as in archive)
	zip_entry_t** zip_entries;			// All the zip entries (same order as in archive)
	zip_dir_t* zip_dir;					// The central directory record
};

#endif // ZIP_H
