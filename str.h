//-----------------------------------------------------------------------------
// File: str.h
//
// Simple fixed buffer based string class
//-----------------------------------------------------------------------------

#ifndef STR_H
#define STR_H

#include "util.h"

template <const int buf_size>
class str_t {
	// Simple array based string class
public:
	str_t()									{ buffer[0] = '\0'; }
	str_t(const char* str)					{ u_strncpy(buffer, str, buf_size);	}
	explicit str_t(const int num)			{ u_itoa(buffer, num); }
	explicit str_t(const float num)			{ u_ftoa(buffer, num); }

	str_t& operator=(const int num)			{ u_itoa(buffer, num); return *this; }
	str_t& operator=(const float num)		{ u_ftoa(buffer, num); return *this; }
	str_t& operator=(const char* str)		{ u_strncpy(buffer, str, buf_size); return *this; }

	operator char*()						{ return buffer; }
	operator const char*() const			{ return buffer; }

	char& operator[](int pos)				{ u_assert(pos < buf_size && pos >= 0); return buffer[pos]; }
	const char& operator[](int pos) const	{ u_assert(pos < buf_size && pos >= 0); return buffer[pos]; }

	bool operator==(const char* str) const	{ return u_strcmp(buffer, str) == 0; }
	bool operator!=(const char* str) const	{ return u_strcmp(buffer, str) != 0; }
	bool operator<(const char* str) const	{ return u_strcmp(buffer, str) < 0; }
	bool operator>(const char* str) const	{ return u_strcmp(buffer, str) > 0; }
	bool operator<=(const char* str) const	{ return u_strcmp(buffer, str) <= 0; }
	bool operator>=(const char* str) const	{ return u_strcmp(buffer, str) >= 0; }

	void operator+=(const char* str)		{ u_strncat(buffer, str, buf_size); }
	void operator+=(const char c);

	int set(const char* str, const int len) { return u_strncpy(buffer, str, u_min((int)buf_size, len)); }

	int cmp(const char* str) const			{ return u_strcmp(buffer, str); }
	int cmpi(const char* str) const			{ return u_stricmp(buffer, str); }
	int cmpfn(const char* str) const		{ return u_fncmp(buffer, str); }

	int cmpn(const char* str, int n) const	{ return u_strncmp(buffer, str, n); }
	int cmpin(const char* str, int n) const	{ return u_strnicmp(buffer, str, n); }
	int cmpfnn(const char* str, int n) const{ return u_fnncmp(buffer, str, n); }

	bool fnmatch(const char* pattern) const	{ return u_fnmatch(buffer, pattern); }

	int length() const						{ return u_strlen(buffer); }
	int size() const						{ return buf_size; }

	void insert(int chr, int pos);
	void remove(int pos);

	char*		str()						{ return buffer; }
	const char*	c_str() const				{ return buffer; }

protected:
	char buffer[buf_size];
};

template <int buf_size>
void str_t<buf_size>::insert(int chr, int pos)
{
	for (int i = u_min(length(), buf_size - 2); i >= pos; --i)
		buffer[i + 1] = buffer[i];
	buffer[pos] = chr; 
	buffer[buf_size - 1] = '\0';
}

template <int buf_size>
void str_t<buf_size>::remove(int pos)
{
	for (int i = pos; i < length(); ++i)
		buffer[i] = buffer[i + 1];
}

template <int buf_size>
void str_t<buf_size>::operator+=(const char c)
{ 
	int pos = length();
	if (pos < buf_size - 1) {
		buffer[pos] = c;
		buffer[pos + 1] = '\0'; 
	} 
}

#endif
