//-----------------------------------------------------------------------------
// File: file.cpp
//
// Contains the implementation for file i/o classes
//-----------------------------------------------------------------------------

#include "file.h"
#include "util.h"
#include "win.h"
#include <search.h>

#include "mem.h"
#define new mem_new

int
file_finder_t::search(const char* dir, const char* mask)
	// Searches dir for filenames that match mask
{
	WIN32_FIND_DATA find_data;
	char name[MAX_PATH];
	u_snprintf(name, MAX_PATH, "%s%s", dir, mask);

	HANDLE hfind = FindFirstFile(name, &find_data);
	if (hfind != INVALID_HANDLE_VALUE) {
		names[next_name++] = name_buf + next_pos;
		next_pos += u_snprintf(name_buf + next_pos, FILE_FINDER_NAME_BUF_SIZE - next_pos,
			"%s%s", dir, find_data.cFileName) + 1;
		while (FindNextFile(hfind, &find_data)) {
			names[next_name++] = name_buf + next_pos;
			next_pos += u_snprintf(name_buf + next_pos, FILE_FINDER_NAME_BUF_SIZE - next_pos,
				"%s%s", dir, find_data.cFileName) + 1;
		}
		FindClose(hfind);
	}
	qsort(names, next_name, sizeof(char*), u_ppstr_cmp_ppstr);
	return next_name;
}
