//-----------------------------------------------------------------------------
// File: util.h
//
// Desc: General purpose utility functions and definitions
//-----------------------------------------------------------------------------

#ifndef UTIL_H
#define UTIL_H

#include "types.h"
#include <stdarg.h>
#include <memory.h>

#include "mem.h"

// Character case checks
extern const unsigned char U_TOLOWER_LOOKUP[];
extern const unsigned char U_TOUPPER_LOOKUP[];
extern const bool U_ISDIGIT_LOOKUP[];
extern const bool U_ISALNUM_LOOKUP[];
extern const bool U_ISSPACE_LOOKUP[];

char* u_itoa(char* buffer, int num);
char* u_ftoa(char* buffer, float num);

#ifndef _DEBUG
inline char u_tolower(int c) { return U_TOLOWER_LOOKUP[c]; }
inline char u_toupper(int c) { return U_TOUPPER_LOOKUP[c]; }
inline bool u_isalnum(int c) { return U_ISALNUM_LOOKUP[c]; }
inline bool u_isdigit(int c) { return U_ISDIGIT_LOOKUP[c]; }
inline bool u_isspace(int c) { return U_ISSPACE_LOOKUP[c]; }
#else
// Startup takes AGES in debug mode since the inline directive
// gets ignored, so in debug mode just use macros
#define u_tolower(c) (U_TOLOWER_LOOKUP[(c)])
#define u_toupper(c) (U_TOUPPER_LOOKUP[(c)])
#define u_isalnum(c) (U_ISALNUM_LOOKUP[(c)])
#define u_isdigit(c) (U_ISDIGIT_LOOKUP[(c)])
#define u_isspace(c) (U_ISSPACE_LOOKUP[(c)])
#endif

// String manipulation functions. 
// Any that return ints return the length of the destination string
// excluding the terminating null
int u_strcpy(char* dest, const char* src);
int u_strncpy(char* dest, const char* src, int size);
int u_snprintf(char* dest, int size, const char* format, ...);
int u_vsnprintf(char* dest, int size, const char* format, va_list val);
int u_strncat(char* dest, const char* src, int size);

int u_strcmp(const char* a, const char* b);
int u_stricmp(const char* a, const char* b);
int u_strncmp(const char* a, const char* b, int size);
int u_strnicmp(const char* a, const char* b, int size);
int u_strlen(const char* str);

void u_strlwr(char* str);
void u_strupr(char* str);

// Comparison functions for working with file names
inline int u_fncmp(const char* a, const char* b) { return u_stricmp(a, b); }
inline int u_fnncmp(const char* a, const char* b, int size) { return u_strnicmp(a, b, size); }
bool u_fnmatch(const char* filename, const char* pattern);

char* u_strdup(const char* str);
char* u_strndup(const char* str, int size);

inline void u_memcpy(void* dest, const void* src, int size) { memcpy(dest, src, size); }
inline void u_memset(void* dest, int c, int size) { memset(dest, c, size); }
inline void u_zeromem(void* dest, int size) { memset(dest, 0, size); }

// Convsions, parse str and store it, store def if conversion fails, return success or failure
bool u_strtof(const char* str, float& f, float def = 0.0f);
bool u_strtoi(const char* str, int& i, int def = 0);

// Break, assert etc
#ifdef _DEBUG
inline void u_break() { __asm { int 3 } };
inline void u_assert(bool test) { if (!test) u_break(); }
#else
#define u_break()
#define u_assert(x)
#endif

template <class T>
inline void u_swap(T& a, T& b)
{
	T temp = b;
	b = a;
	a = temp;
}

template <class T>
inline T u_min(const T& a, const T& b)
{
	return a < b ? a : b;
}

template <class T>
inline T u_max(const T& a, const T& b)
{
	return a > b ? a : b; 
}

// Used for qsorting char* arrays
int u_ppstr_cmp_ppstr(const void*, const void*);
// Used for qsorting char* arrays (case insensitive
int u_ppstr_icmp_ppstr(const void*, const void*);

inline int u_ppfn_cmp_ppfn(const void* a, const void* b) { return u_ppstr_icmp_ppstr(a, b); }

template <class T>
class list_t {
	// Simple linked list class, can take a destroy function, if this is set
	// then that function will be called for each item before deleting its node
	// Implementation is a circular linked list with dummy head node, to iterate
	// use for (node_t* n = list.begin(); n->next != list.end(); n = n->next)
public:
	class node_t {
	public:
		node_t(T& t, node_t* n) : item(t), next(n) {}
		node_t* next;
		T item;

		friend class list_t;
	};

	friend class node_t;

	list_t(T t = T()) { head = tail = mem_new node_t(t, 0); head->next = head; }
	~list_t() { while (head->next != head) delete_after(head); delete head; }

	node_t* begin() { return head; }
	node_t* end()  { return tail; }

	void insert_after(node_t* node, T& t) { node->next = mem_new node_t(t, node->next); }
	void delete_after(node_t* node)
	{ node_t* tmp = node->next; node->next = node->next->next; delete tmp; }

private:
	node_t* head;
	node_t* tail;
};

/*
template <class T, int (*compare)(const T&, const T&)>
class sorted_list_t : private list_t<T> {
};

template <class T>
class stack_t : private list_t<T> {
	// Stack, simple specialisation of the list class
public:
	void push(T t) { push_head(t); }
	T pop() { return pop_head(); }
};
*/

class result_t {
	// Return value from functions, can return true, false or a string and
	// works as either. Use when a reason for failure may be required
public:
	result_t() {}
	result_t(bool b) { if (b) msg = last.msg = 0; else msg = last.msg = "Unspecified Error"; }
	result_t(char* txt) { msg = last.msg = txt; }
	operator bool() const { return msg == 0; }
	operator const char*() const { return msg ? msg : "Success"; } 
	bool operator!() const { return msg != 0; }

	char* msg;
	static result_t last;
};

template <class T>
class vector_t {
	// Simple vector class, not much use for anything more complicated
	// than pointers or simple types
public:
	vector_t(int grow = 8) : grow_size(grow), vec_size(0), items(0) { if (grow <= 0) grow = 1; }
	~vector_t() { if (items) delete [] items; }

	T& operator[](int i) { return items[i]; }
	const T& operator[](int i) const { return items[i]; }

	int size() const { return vec_size; }
	void append(const T& t);
	T* contents() { return items; }

private:
	T* items;
	int vec_size;
	int grow_size;
};

template <class T> void 
vector_t<T>::append(const T& t)
	// Append an element to a vector
{
	if (vec_size == 0 || vec_size % grow_size == 0) {
		T* new_items = mem_new T[vec_size + grow_size];
		for (int i = 0; i < vec_size; i++)
			new_items[i] = items[i];
		if (items)
			delete [] items;
		items = new_items;
	}
	items[vec_size++] = t;
}

// Maximum length of an individual token
#define MAX_TOKEN 1024

class tokenizer_t {
	// String tokenizer
public:
	class mark_t {
		// Used to mark a position in the tokenizer
	public:
		friend class tokenizer_t;
		mark_t(const mark_t& mark) : pos(mark.pos), cur(mark.cur), 
			line_start(mark.line_start), line(mark.line) {}
		mark_t& operator=(const mark_t& mark) {
			pos = mark.pos; cur = mark.cur; line_start = mark.line_start; line = mark.line;
			return *this;
		}
	private:
		mark_t(const char* p, const char* c, const char* ls, int l) 
			: pos(p), cur(c), line_start(ls), line(l) {}
		const char* pos;
		const char* cur;
		const char* line_start;
		int line;
	};

	tokenizer_t(const char* buffer, int size, bool newlines = false) : buf(buffer), pos(buffer), line(0), 
		line_start(buffer), cur(buffer), new_lines(newlines)
		{ if (!pos) cur = pos = buffer = line_start = token; *token = '\0'; end = buffer + size; }

	// Retrieve a token/line into a user specified buffer
	char* current_token(char* dest, int size) const;
	char* next_token(char* dest, int size);
	char* peek_token(char* dest, int size) const;
	char* current_line(char* dest, int size) const;

	// Retrieve a token/line using the tokenizers internal buffer, if only one
	// token at a time will ever be required these should be more than adequate
	// Note that these all return pointers to the same buffer, so using them will
	// corrupt any previous values returned
	char* current_token() { return current_token(token, MAX_TOKEN); }
	char* next_token() { return next_token(token, MAX_TOKEN); }
	char* peek_token() { return peek_token(token, MAX_TOKEN); }
	char* current_line() { return current_line(token, MAX_TOKEN); }

	// Save the current position in the buffer
	mark_t mark();
	// Restore a previously saved position
	void reset(const mark_t& mark);

	// Distance into the buffer to the start of the current token
	int offset() const { return pos - buf; }
	// line number of the current token (0 based)
	int line_no() const { return line; }
	// Offset into the current line of the start of the current token
	int line_pos() const { return cur - line_start; }

private:
	const char* buf;
	const char* pos;
	const char* end;
	const char* cur;
	const char* line_start;
	int line;
	bool new_lines;		// Return newlines as tokens
	char token[MAX_TOKEN];
};

#endif
