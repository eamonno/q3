//-----------------------------------------------------------------------------
// File: pak.cpp
//
// Implementation for pak manager
//-----------------------------------------------------------------------------

#include "pak.h"
#include "console.h"
#include "util.h"
#include "exec.h"
#include <search.h>
#include <memory>

#include "mem.h"
#define new mem_new

cvar_str_t	pakdir("pakdir", "g:/", CVF_CONST);

pak_t&
pak_t::get_instance()
{
	static std::auto_ptr<pak_t> instance(new pak_t());
	return *instance;
}

int
compare_filenames(const void* fn1, const void* fn2)
{
	return static_cast<const filename_t*>(fn1)->cmpfn(*static_cast<const filename_t*>(fn2));
}

int
pak_file_finder_t::search(const char* dir, const char* mask)
	// Search all the paks for matches for mask in the named dir (and sub-
	// directories of dir). The ? and * chars are wildcards in mask
{
	int dirlen = u_strlen(dir);

	for (int i = pak.zips.size() - 1; i >= 0; --i) {
		int j = 0;
		while (j < pak.zips[i]->num_entries() && pak.zips[i]->entry_name(j).cmpfnn(dir, dirlen))
			++j;
		while (j < pak.zips[i]->num_entries() && !pak.zips[i]->entry_name(j).cmpfnn(dir, dirlen)) {
			if (u_fnmatch(pak.zips[i]->entry_name(j).c_str() + dirlen, mask)) {
				for (int k = 0; k < names.size(); ++k)
					if (!names[k].cmpfn(pak.zips[i]->entry_name(j)))
						break;	// Prevent duplicates
				if (k == names.size())
					names.append(pak.zips[i]->entry_name(j));
			}
			++j;
		}
	}
	qsort(names.contents(), names.size(), sizeof(filename_t), compare_filenames);
	return names.size();
}

pak_t::pak_t()
{
}

pak_t::~pak_t()
	// Delete the zip files
{
	for (int i = 0; i < zips.size(); i++)
		delete zips[i];
}

result_t
pak_t::init()
{
	return scan_dir(*pakdir);
}

bool
pak_t::add_archive(const char* name)
	// Opens a single named archive and adds its contents to the index
{
	zip_t* zip = new zip_t(name);

	if (zip->valid()) {
		zips.append(zip);
		console.printf("Using pak: %s (%d files)\n", name, zip->num_entries());
		return true;
	} else {
		delete zip;
		return false;
	}
}

result_t
pak_t::scan_dir(const char* dir)
	// Scans a directory for pak files and adds their combined contents to the
	// index
{
	file_finder_t finder(dir, "*.pk3");

	bool errors = false;
	for (int i = 0; i < finder.count(); ++i)
		if (!add_archive(finder[i]))
			errors = true;
	if (errors)
		return "One or more errors occured opening pak files, check console log for more details";
	return true;
}

file_t*
pak_t::open_file(const char* name)
	// Open the named file
{
	file_t* file = 0;
	for (int i = zips.size() - 1; i >= 0; --i)
		if ((file = zips[i]->open_file(name)) != 0)
			return file;
	return 0;
}

bool
pak_t::file_exists(const char* name)
	// Check if the file exists in the pak
{
	// Check in reverse order since later paks overwrite earlier ones
	for (int i = zips.size() - 1; i >= 0; --i)
		if (zips[i]->file_exists(name))
				return true;
	return false;
}
