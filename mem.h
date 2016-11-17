//-----------------------------------------------------------------------------
// File: mem.h
//
// Memory managment code. To enable this #include "mem.h" in whatever file you
// wish to track allocations and deletions from. You will also need to do one
// of the following:
// either use mem_new where you would normally use new on an instance by
// instance basis e.g. put
//   char* str = mem_new char[40];
// or just put
//   #define new mem_new
// at the start of the compilation unit. The latter is best if you wish to add
// memory manager support to an existing file, but if you are overloading new
// for a given class within the compilation unit, or if you wish to use it in
// a header file where some of the files including the header will be using
// a different new then the instance by instance basis may be more desirable.
//-----------------------------------------------------------------------------

#ifndef MEM_H
#define MEM_H

// This can be used to toggle debugging of memory on or off
#ifdef _DEBUG
	#ifndef MEM_DEBUGGING_ENABLED
		#define MEM_DEBUGGING_ENABLED
	#endif
#endif

// Report on memory usage
void mem_report();

// Override global new, new[], delete and delete[] operators
void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* ptr);
void operator delete[](void* ptr);

// Macros for the allocator functions, you should just use new and delete as
// normal, however if it is important that new be used in a file (e.g. some
// class wants to override operator new directly then just #undef new and
// delete in that file and use mem_new and mem_delete where normal allocation
// or deallocation of memory takes place

#ifdef MEM_DEBUGGING_ENABLED
	// Add overloaded new. new[], delete and delete[] that take file and line
	void *operator new(size_t size, const char* file, const int line);
	void *operator new[](size_t size, const char* file, const int line);
	// These are only called if there is an exception during a constructor
	void operator delete(void* ptr, const char* file, const int line);
	void operator delete[](void* ptr, const char* file, const int line);

	#define mem_new		new(__FILE__, __LINE__)
#else
	#define mem_new		new
#endif

#endif
