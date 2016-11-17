//-----------------------------------------------------------------------------
// File: zip.cpp
//
// Desc: Interfacing with zip files
//-----------------------------------------------------------------------------

#include "zip.h"
#include "ziptypes.h"
#include "zlib.h"
#include <search.h>
#include "console.h"

#include "mem.h"
#define new mem_new

void
zip_file_t::close()
	// Close the zip file, if the data in the file was decompressed at any time
	// then its contents are deleted here
{
	if (_data && _data != _entry->compressed_data())
		delete [] _data;
	_data = NULL;
	_entry = NULL;
}

ubyte*
zip_file_t::data()
	// Return a pointer to a byte array containing the full contents of the zip
	// file. If the file is compressed then the file will be fully decompressed
	// to a newly allocated region of memory and returned. If the file has an
	// uncompressed size of 0 then NULL will be returned
{
	if (_data == NULL && _entry != NULL && (_entry->uncompressed_size() > 0)) {
		if (_entry->method() == zip_method_stored) {
			_data = _entry->compressed_data();
		} else if (_entry->method() == zip_method_deflated) {
			_data = new ubyte[_entry->uncompressed_size() + 1];

			z_stream zs;
			zs.next_in = _entry->compressed_data();
			zs.avail_in = _entry->compressed_size() + 1;
			zs.next_out = _data;
			zs.avail_out = _entry->uncompressed_size() + 1;
			zs.zalloc = 0;
			zs.zfree = 0;
			zs.opaque = 0;

			int num;
			if (inflateInit2(&zs, -MAX_WBITS) == Z_OK)
				if ((num = inflate(&zs, Z_SYNC_FLUSH)) != Z_STREAM_END) {
					delete [] _data;	// Failed, free memory
					_data = NULL;
				}
			inflateEnd(&zs);
		}
	}
	return _data;
}

namespace {
	filename_t* sort_names;

	int
	compare_index(const void* p1, const void* p2)
	{
		const int index1 = *(static_cast<const int*>(p1));
		const int index2 = *(static_cast<const int*>(p2));

		return sort_names[index1].cmpfn(sort_names[index2]);
	}
}

bool
zip_t::open(const char* filename)
	// Open a named zip archive. Will also index the contents of the archive
	// for future access. If any parts of the archive accessed are invalid then
	// this operation will fail
{
	if (mmfile_t::open(filename)) {
		// Check the start signiture
		if (*reinterpret_cast<uint*>(view) == zip_file_sig) {
			// First locate the zip directory at the end of the file, since this may be
			// trailed by a file comment of up to ushort.max() bytes it may be
			// nescessary to backtrack through the file until a correct header is found
			ubyte* pos = view + view_size - zip_dir_size;
			ubyte* min_pos = u_max(pos - 65536, view);	// May be a 64k comment after the dir
			while (pos >= min_pos && !(zip_dir = reinterpret_cast<zip_dir_t*>(pos))->is_valid())
				pos--;

			if (zip_dir->is_valid()) {
				if (zip_dir->total_entries()) {	// Its possible the archive has no files
					zip_dir_entries = new zip_dir_entry_t*[zip_dir->total_entries()];
					zip_entries = new zip_entry_t*[zip_dir->total_entries()];
					names = new filename_t[zip_dir->total_entries()];
					index = new int[zip_dir->total_entries()];

					pos = view + zip_dir->offset();
					for (int i = 0; i < zip_dir->total_entries(); i++) {
						zip_dir_entries[i] = reinterpret_cast<zip_dir_entry_t*>(pos);
						if (zip_dir_entries[i]->is_valid()) {
							zip_entries[i] = reinterpret_cast<zip_entry_t*>(view + zip_dir_entries[i]->offset());
							names[i].set(zip_entries[i]->name(), zip_entries[i]->name_length() + 1);
							index[i] = i;
						} else {
							close();	// Just fail the archive altogether if anything is invalid
							return false;
						}
						pos += zip_dir_entries[i]->extent();
					}
					// Sort the entries - Note the use of u_ppfn_cmp_ppfn, since name is the first part of the
					// index_entries struct we can just treat the pointer as a string pointer
					sort_names = names;
					qsort(index, zip_dir->total_entries(), sizeof(int), compare_index);
					sort_names = 0;
				}
				return true;
			}
		}
	}
	close();
	return false;
}

void
zip_t::close()
	// Close a zip archive
{
	delete [] names;
	names = 0;
	delete [] index;
	index = 0;
	delete [] zip_dir_entries;
	zip_dir_entries = 0;
	delete [] zip_entries;
	zip_entries = 0;
	zip_dir = 0;

	// Base class closes the file itself
	mmfile_t::close();
}

zip_file_t*
zip_t::open_file(const char* filename)
	// Opens a single named subfile inside the zip archive. The caller
	// should delete the returned zip_file_t when finished with it
{
	int file_index = find_index(filename);
	if (file_index == -1)
		return 0;
	else
		return new zip_file_t(zip_entries[file_index]);
}

bool
zip_t::file_exists(const char* filename)
	// Check whether or not the archive has a file with name filename
{
	return find_index(filename) != -1;
}

int
zip_t::find_index(const char* filename)
	// Return the index into the index entries array for filename, -1 if not found
{
	int left = 0, right = zip_dir->total_entries() - 1;

	// Since the names array is sorted perform a binary search
	for (;;) {
		if (left > right)
			return -1;
		int middle = (left + right) / 2;
		int cmp = names[index[middle]].cmpfn(filename);
		if (cmp == 0)
			return index[middle];
		if (left == right)
			return -1;
		if (cmp > 0)
			right = middle - 1;
		else
			left = middle + 1;
	}

}
