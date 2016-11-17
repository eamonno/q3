//-----------------------------------------------------------------------------
// File: util.cpp
//
// General purpose utility functions
//-----------------------------------------------------------------------------

#include "util.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "mem.h"
#define new mem_new

result_t result_t::last;
/*
const unsigned char chars[] = {
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,
};
*/
const unsigned char U_TOLOWER_LOOKUP[] = {
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,
};

const unsigned char U_TOUPPER_LOOKUP[] = {
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0x60,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,
};

const bool U_ISALNUM_LOOKUP[] = {
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	 true, true, true, true, true, true, true, true, true, true,false,false,false,false,false,false,
	false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
	 true, true, true, true, true, true, true, true, true, true, true,false,false,false,false,false,
	false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
	 true, true, true, true, true, true, true, true, true, true, true,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
};

const bool U_ISDIGIT_LOOKUP[] = {
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	 true, true, true, true, true, true, true, true, true, true,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
};

const bool U_ISSPACE_LOOKUP[] = {
	false,false,false,false,false,false,false,false,false, true, true, true, true, true,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	 true,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
	false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,
};

char*
u_itoa(char* buffer, int num)
	// Convert an int to a string
{
	return itoa(num, buffer, 10);
}

char*
u_ftoa(char* buffer, float num)
	// Convert a float to a string
{
	u_snprintf(buffer, 20, "%f", num);
	return buffer;
}

int
u_strcpy(char* dest, const char* src)
	// Copies src to dest up to and including terminating null
	// Return the length of dest 
{
	char* start = dest;
	while ((*dest = *src) != 0) {
		++dest;
		++src;
	}
	return dest - start;
}

int
u_strncpy(char* dest, const char* src, int size)
	// Copies src to dest up to size chars (including terminating null)
	// Return the length of dest
{
	char* start = dest;
	while (size-- && (*dest = *src)) {
		++dest;
		++src;
	}
	if (size == -1)
		*(--dest) = '\0';
	return dest - start;
}

int
u_snprintf(char* dest, int size, const char* format, ...)
	// Print up to n chars to a string (including terminating null)
	// Return the length of dest
{
	va_list argptr;
	va_start(argptr, format);
	int n = _vsnprintf(dest, size, format, argptr);
	va_end(argptr);
	if (n == -1) {	// _vsnprintf returns -1 on buffer overflow
		*(dest + size - 1) = '\0';
		return size - 1;
	} else if (n < -1) { // some other error occured?
		*dest = '\0';
		return 0;	
	}
	return n;
}

int
u_vsnprintf(char* dest, int size, const char* format, va_list val)
	// Print up to n chars to a string (including terminating null)
	// Return the length of dest
{
	int n = _vsnprintf(dest, size, format, val);
	if (n == -1) {	// _vsnprintf returns -1 on buffer overflow
		*(dest + size - 1) = '\0';
		return size - 1;
	} else if (n < -1) { // some other error occured?
		*dest = '\0';
		return 0;	
	}
	return n;
}

int
u_strcmp(const char* a, const char* b)
	// Compare a to b until either ends
	// returns < 0 if a < b, 0 if a == b, > 0 if a > b
{
	while (*a && *a == *b) {
		++a;
		++b;
	}
	return *a - *b;
}

int
u_stricmp(const char* a, const char* b)
	// Compare a to b until either ends ignoring case
	// returns < 0 if a < b, 0 if a == b, > 0 if a > b
{
	while (*a && u_tolower(*a) == u_tolower(*b)) {
		++a;
		++b;
	}
	return u_tolower(*a) - u_tolower(*b);
}

int
u_strncmp(const char* a, const char* b, int size)
	// Compare up to size chars from a to b
	// returns < 0 if a < b, 0 if a == b, > 0 if a > b
{
	if (size == 0)
		return 0;

	while (--size && *a && *a == *b) {
		++a;
		++b;
	}
	return *a - *b;
}

int
u_strnicmp(const char* a, const char* b, int size)
	// Compare up to size chars from a to b ignoring case
	// returns < 0 if a < b, 0 if a == b, > 0 if a > b
{
	if (size == 0)
		return 0;

	while (--size && *a && u_tolower(*a) == u_tolower(*b)) {
		++a;
		++b;
	}
	return u_tolower(*a) - u_tolower(*b);
}

int
u_strncat(char* dest, const char* src, int size)
	// Concatenate src to dest, returns the length of dest
	// after concatenation
{
	while (*dest && size--)
		dest++;
	return u_strncpy(dest, src, size);
}

int
u_strlen(const char* str)
	// Return the length of str excluding the terminating null
{
	const char* end = str;
	while (*end)
		++end;
	return end - str;
}

void
u_strlwr(char* str)
	// Convert str to lower case and store in dest
{
	while (*str) {
		*str = u_tolower(*str);
		++str;
	}
}

void
u_strupr(char* str)
	// Convert str to upper case
{
	while (*str) {
		*str = u_toupper(*str);
		++str;
	}
}

bool
u_fnmatch(const char* filename, const char* pattern)
	// Check if the filename matches the pattern string, the pattern string can
	// contain the characters '*' which matches 0 or more chars and '?' which
	// matches any individual character, comparisons ignore case
{
	while (*filename && *pattern) {
		if (*pattern == '*') {
			while (*filename)
				if (u_fnmatch(filename, pattern + 1))
					return true;
				else
					filename++;
			return false;
		} else if (*pattern != '?') {
			if (u_tolower(*pattern) != u_tolower(*filename))
				return false;
		}
		// No need to do anything other than advance if *pattern == '?'
		pattern++;
		filename++;
	}
	return !*filename && !*pattern;	// True if both have reached the ending null
}

char*
u_strdup(const char* str)
	// Duplicate src
{
	int length = u_strlen(str);
	char* dup = new char[length + 1];
	u_memcpy(dup, str, length + 1);
	return dup;
}

char*
u_strndup(const char* str, int size)
	// Duplicate str but new string should be no more than size chars
{
	const char* end = str;
	while (--size && *end)
		end++;
	char* dup = new char[end - str + 1];
	u_memcpy(dup, str, end - str);
	dup[end - str] = '\0';
	return dup;
}

bool
u_strtof(const char* str, float& f, float def)
	// Parse str for a float, and store it in f, if it fails set f to def
	// and return false
{
	f = static_cast<float>(atof(str));

	if (f == 0.0f) { // atof returns 0.0f on failure
		// valid float 0 = "[+-]+[0]*.[0]*{[dDeE][+-]+[0-9]+"
		int zeros = 0;
		while (u_isspace(*str))
			++str;
		if (*str == '-' || *str == '+')
			++str;
		while (*str == '0') {
			++str;
			++zeros;
		}
		if (*str == '.')
			++str;
		while (*str == '0') {
			++str;
			++zeros;
		}
		if (u_tolower(*str) == 'd' || u_tolower(*str) == 'e') {
			++str;
			if (*str == '-' || *str == '+')
				++str;
			if (!u_isdigit(*str))
				zeros = 0;	// Fail must be some numbers after e
			while (u_isdigit(*str))
				++str;
		}
		if (zeros && (u_isspace(*str) || *str == '\0'))
			return true;
		f = def;
		return false;
	}
	return true;
}

bool
u_strtoi(const char* str, int& i, int def)
	// Parse str for an int, and store it in i, if it fails set i to def
	// and return false
{
	i = atoi(str);
	if (i == 0) {
		while (u_isspace(*str))
			++str;
		int zeros = 0;
		if (*str == '0') {
			++str;
			++zeros;
		}
		if ((u_isspace(*str) || *str == '\0') && zeros >= 0)
			return true;
		i = def;
		return false;
	}
	return true;
}

int
u_ppstr_cmp_ppstr(const void* a, const void* b)
	// Compare contents of string pointers for qsort
{
	return u_strcmp(*(static_cast<char* const*>(a)), *(static_cast<char* const*>(b)));
}

int
u_ppstr_icmp_ppstr(const void* a, const void* b)
	// Compare contents of string pointers for qsort
{
	return u_stricmp(*(static_cast<char* const*>(a)), *(static_cast<char* const*>(b)));
}

char*
tokenizer_t::current_token(char* dest, int size) const
	// Return a copy of the current token in dest up to size chars
{
	// The const_cast is a little messy, but it gurantees that this function
	// behaves exactly like the next_token function with regard to quotation etc.
	tokenizer_t* tok = const_cast<tokenizer_t*>(this);
	mark_t mark = tok->mark();
	tok->pos = tok->cur;
	// Retrieve the token
	char* token = tok->next_token(dest, size);
	tok->reset(mark);
	// Return the token
	return token;
}

char*
tokenizer_t::next_token(char* dest, int size)
	// Return the next token in the buffer, copied into dest. returns 0 if
	// there are no more tokens to read. if size is non-zero then dest must
	// not be NULL
{
	if (pos == end || *pos == '\0')
		return 0;

	// Skip past any white space or comments
	while (pos != end && *pos && (u_isspace(*pos) || (*pos == '/' && *(pos + 1) == '/'))) {
		while (pos != end && u_isspace(*pos)) {
			if (*pos == '\n') {
				++line;
				line_start = pos + 1;
			}
			++pos;
		}
		if (pos != end && *pos && *pos == '/' && *(pos + 1) == '/') {
			pos += 2;
			while (pos != end && *pos && *pos != '\n')
				++pos;
			if (pos != end && *pos) {
				++pos;	// Skip the '\n'
				++line;
				line_start = pos;
			}
		}
	}
	// If at end of buffer now there are no more tokens
	if (pos == end || *pos == '\0')
		return 0;

	char* tok = dest;
	cur = pos;
	if (*pos == '"') {	// Token is a double quoted string
		++pos;	// Skip the double quote
		while (pos != end && *pos != '"' && *pos && tok < dest + size - 1)
			*tok++ = *pos++;
		if (*pos == '"')
			++pos;
	} else if (*pos == '\'') {	// Token is a single quoted string
		++pos;	// Skip the single quote
		while (pos != end && *pos != '\'' && *pos && tok < dest + size - 1)
			*tok++ = *pos++;
		if (*pos == '\'')
			++pos;
	} else {
		while (pos != end && !u_isspace(*pos) && *pos && tok < dest + size - 1)
			*tok++ = *pos++;
	}
	*tok = '\0';	// Null terminate

	return dest;
}

char*
tokenizer_t::peek_token(char* dest, int size) const
	// Copies the next token requred into dest up to size chars total
	// (including terminating null), returns dest.
{
	// The const_cast is a little messy, but it gurantees that this function
	// behaves exactly like the next_token function with regard to quotation etc.
	tokenizer_t* tok = const_cast<tokenizer_t*>(this);
	mark_t mark = tok->mark();
	// Retrieve the token
	char* token = tok->next_token(dest, size);
	tok->reset(mark);
	// Return the token
	return token;
}

char* 
tokenizer_t::current_line(char* dest, int size) const
	// Returns the line of text that is currently being processed (up to size
	// chars, copied into dest, returns a pointer to dest
{
	const char* start = line_start;
	char* tok = dest;
	while (*start && *start != '\n' && *start != '\r' && tok < dest + size - 1)
		*tok++ = *start++;
	*tok = '\0';
	return dest;
}

tokenizer_t::mark_t
tokenizer_t::mark()
	// Return a mark that can be used to return to the current position in the
	// buffer at a later time
{
	return mark_t(pos, cur, line_start, line);
}

void
tokenizer_t::reset(const mark_t& mark)
	// Reset the tokenizer to a previously saved position
{
	pos = mark.pos;
	cur = mark.cur;
	line_start = mark.line_start;
	line = mark.line;
}
