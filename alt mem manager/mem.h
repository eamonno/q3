//-----------------------------------------------------------------------------
// File: mem.h
//
// Memory managment. All memory is allocated from a single global memory block
// which is allocated during program startup. This memory block works in a 
// stack fashion, with permanant allocations being allocated from the bottom
// of the stack, and temporary allocations being allocated from the top of the
// stack. Any objects that will remain in use should be allocated from the
// permanant area, as the temporary memory region should be clear between
// frames.
//-----------------------------------------------------------------------------

#ifndef MEM_H
#define MEM_H

// This can be used to toggle debugging of memory on or off
#ifdef _DEBUG
	#ifndef MEM_DEBUGGING_ENABLED
		#define MEM_DEBUGGING_ENABLED
	#endif
#endif

// Class for initializing memory, one global instance of this is included in
// every file that includes "mem.h", thus mem_init() and mem_uninit() are
// called automatically
class mem_initializer_t {
	static int count;
	
	void mem_init();
	void mem_uninit();
public:
	mem_initializer_t()		{ if (count++ == 0) mem_init();		}
	~mem_initializer_t()	{ if (--count == 0) mem_uninit();	}
};

namespace {
	mem_initializer_t the_amazing_memory_initilizer;
}

enum mem_region_t {
	perm_mem,	// Permanant storage for objects that last a long time
	temp_mem,	// Temporary allocation, should be quickly released
};

enum mem_alloc_type_t {
	mem_alloc_new,			// Allocated with new
	mem_alloc_new_array,	// Allocated with new[]
};


// Override global new, new[], delete and delete[] operators
void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* ptr);
void operator delete[](void* ptr);

// Placement new, new[], delete and delete[]
void* operator new(size_t size, void* where)	{ return where; }
void* operator new[](size_t size, void* where)	{ return where; }
void operator delete(void* ptr, void* where)	{}
void operator delete[](void* ptr, void* where)	{}

// Memory manager operators new and delete
void* operator new(size_t size, mem_region_t region);
void* operator new[](size_t size, mem_region_t region);
void operator delete(void* ptr, mem_region_t region);
void operator delete[](void* ptr, mem_region_t region);

#ifdef MEM_DEBUGGING_ENABLED
// Debug versions of new, new[], delete and delete[]
void* operator new(size_t size, mem_region_t region, const char* file, const int line);
void* operator new[](size_t size, mem_region_t region, const char* file, const int line);
void operator delete(void* ptr, mem_region_t region, const char* file, const int line);
void operator delete[](void* ptr, mem_region_t region, const char* file, const int line);

// Debug versions of placement new, new[], delete and delete[]
void* operator new(size_t size, void* where, const char* file, const int line)		{ return where; }
void* operator new[](size_t size, void* where, const char* file, const int line)	{ return where; }
void operator delete(void* ptr, void* where, const char* file, const int line)		{}
void operator delete[](void* ptr, void* where, const char* file, const int line)	{}

// Update the calls to new
#define new(x) new((x), __FILE__, __LINE__)

#endif

#endif
