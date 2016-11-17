//-----------------------------------------------------------------------------
// File: mem.cpp
//
// Memory management
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <new>
#include "util.h"
#include "types.h"
#include "mem.h"
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

/*
enum mem_region_t {
	perm_mem,	// Permanant storage for objects that last a long time
	temp_mem,	// Temporary allocation, should be quickly released
};

enum mem_alloc_type_t {
	mem_alloc_new,			// Allocated with new
	mem_alloc_new_array,	// Allocated with new[]
};

// Report on memory usage
void mem_report();

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
*/

namespace {
	const int	heap_size = 20 * 1024 * 1024;		// 20 megabyte heap

	ubyte*	heap_ptr;		// Pointer to the heaps physical location in memory

	void	mem_report();
	void*	mem_alloc_final(size_t size, mem_region_t region);
	void	mem_dealloc_final(void* ptr);
	void	mem_alloc_debug(size_t size, mem_region_t region, const char* file, const int line);
	void	mem_dealloc_debug(size_t size, mem_region_t region, const char* file, const int line);

#pragma pack (push, 1)

	struct heap_chunk_t {
		heap_chunk*	prev;				// Previous chunk
		heap_chunk*	next;				// Next chunk
		heap_chunk*	prev_free;			// Previous free chunk
		heap_chunk*	next_free;			// Next free chunk
		size_t		reported_size;		// Size of memory requested
		size_t		actual_size;		// Size of memory allocated (including the header)
#ifdef MEM_DEBUGGING_ENABLED
		const char* alloc_file;			// Filename of the file where this was allocated
		int			alloc_line;			// Line number in that file where it was allocated
		short		method;				// mem_alloc_type_t with which this memory was allocated
		short		dealloc_count;		// Number of times this memory has been deallocated
		int			request;			// Request number
#endif
		char		filler[4];			// Qword align the buffer
		uint		guard;				// Checked for corruption
	};

#pragma pack (pop)

	heap_chunk*		free_list_last;
	heap_chunk*		free_list_first;
}

int mem_initializer_t::count = 0;

void
mem_initializer_t::mem_init()
	// Initialize unaligned_heap_ptr and heap_ptr
{
	u_assert(heap_ptr == 0);
	u_assert((sizeof(heap_chunk_t) & 0xf) == 0);	// must be multiple of 16 bytes
	heap_ptr = malloc(heap_size + 15);

	ubyte* aligned_heap_ptr = (heap_ptr + 15) - ((heap_ptr + 15) & 0xf);

	free_list_first = reinterpret_cast<heap_chunk_t*>aligned_heap_ptr;
	free_list_first->prev = 0;
	free_list_first->next = 0;
	free_list_first->prev_free = 0;
	free_list_first->next_free = 0;
	free_list_first->actual_size = heap_size;
	free_list_first->reported_size = 0;
#ifdef DEBUG_MEMORY_MANAGER
	free_list_first->alloc_line = 0;
	free_list_first->alloc_file = 0;
	free_list_first->method = 0;
	free_list_first->dealloc_count = 0;
	free_list_first->request = 0;
#endif
	free_list_last  = free_list_first;
}

void*
mem_alloc_perm(size_t size)
{
	heap_chunk_t* chunk = free_list_first;

	// we need size + sizeof(heap_chunk_t) rounded up to nearest 16 byte boundary bytes
	int size_needed = size + sizeof(heap_chunk_t);
	size_needed = (size_needed + 15) - ((size_needed + 15) & 0xf);

	// Find the earliest chunk that can fit this much mem in the list
	while (chunk && chunk->actual_size < size_needed)
		chunk = chunk->next_free;

	if (chunk) {
		// Take memory from the bottom of the free chunk, this requires moving
		// all the chunk information to the start of its new location
		ubyte* free_chunk_pos = reinterpret_cast<ubyte*>(chunk) + size_needed;
		heap_chunk_t* free_chunk = reinterpret_cast<heap_chunk_t*>(free_chunk_pos);
		*free_chunk = *chunk;
		free_chunk->actual_size -= size_needed;
		// Update the pointer to this chunk
		if (free_chunk->prev_free)
			free_chunk->prev_free->next_free = free_chunk;

		chunk->prev = ???;
		chunk->next = ???;
		chunk->prev_free = 0;
		chunk->next_free = 0;
		chunk->size = size;
	}
}

void*
mem_alloc_temp(size_t size)
{
	heap_chunk_t* chunk = free_list_last;
	
	// we need size + sizeof(heap_chunk_t) rounded up to nearest 16 byte boundary bytes
	int size_needed = size + sizeof(heap_chunk_t);
	size_needed = (size_needed + 15) - ((size_needed + 15) & 0xf);

	// Find the chunk nearest the end of the free list of sufficient size
	while (chunk && chunk->actual_size < size_needed)
		chunk = chunk->prev_free;

	if (chunk) {
		ubyte* new_chunk_pos = reinterpret_cast<ubyte*>(chunk) + chunk->actual_size + sizeof(heap_chunk_t) - size_needed);
		heap_chunk_t* new_chunk = reinterpret_cast<heap_chunk_t*>(new_chunk_pos);
		chunk->actual_size -= size_needed;

		new_chunk->prev = ???;
		new_chunk->next = ???;
		new_chunk->prev_free = 0;
		new_chunk->next_free = 0;
		new_chunk->actual_size = size_needed;
		new_chunk->reported_size = size;
	}
}

class mem_manager_t {
	void* allocate(size_t size);
	void  deallocate(void* ptr);
};

void* operator new(size_t size, mem_manager_t& mmgr)
{
	return mmgr.allocate(size);
}

void operator delete(void* ptr, 
void
mem_initializer_t::mem_uninit()
	// Free the heap memory
{
	u_assert(unaligned_heap_ptr != 0);
	u_assert(heap_ptr != 0);

	mem_report();

	free(unaligned_heap_ptr);
	unaligned_heap_ptr = 0;
	heap_ptr = 0;
}

/*
	/////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration variables - Changing these will alter the behaviour of the Memory Manager //
	/////////////////////////////////////////////////////////////////////////////////////////////

	const int guard_size	 = 4;			// Size of guard buffers
	const int break_on_alloc = -1;			// Break when this request arrives, -1 to disable

	const bool break_on_fore_guard_corruption = false;	// Break if fore guard is mangled
	const bool break_on_rear_guard_corruption = false;	// Break if rear guard is mangled
	const bool break_on_node_loss = false;			// Break if the nodes go missing from the list
	const bool break_on_node_recovery = false;		// Break if lost nodes are recovered
	const bool break_on_multiple_deletion = false;	// Break if a node is deleted 2 or more times
	const bool log_all_allocations = false;			// Log every allocation of memory
	const bool log_all_frees = false;				// Log every time memory is freed
	const bool prefer_mem_leaks_to_crashes = true;	// If true, then corrupt nodes will not be freed

	// Im working on these :)
//	const bool retain_freed_memory = true;	// Keep freed memory and check if its been overwritten
//											// Can also provide more detail on multiple deletions

	/////////////////////////////////////////////////////////////////////////////////////////////
	//                             End of Configuration variables                              //
	/////////////////////////////////////////////////////////////////////////////////////////////

	// Memory allocation methods, must match the corresponding appropriate mem
	// free method below
	const int MEM_ALLOC_METHOD_NEW = 0;
	const int MEM_ALLOC_METHOD_NEW_ARRAY = 1;

	// Memory deallocation methods, must match the corresponding mem alloc
	// method declared above
	const int MEM_FREE_METHOD_DELETE = 0;
	const int MEM_FREE_METHOD_DELETE_ARRAY = 1;

	// Values to use when filling buffers
	const int guard_value	 = 0xDEADBEEF;	// Filler for guard buffers
	const int initial_value	 = 0xBABEFACE;	// Initialize allocated memory to this
	const int clear_value	 = 0xBADDECAF;	// Overwrite freed memory with this

	// Char array versions of the buffers
	const char* guard_chars = reinterpret_cast<const char*>(&guard_value);
	const char* initial_chars = reinterpret_cast<const char*>(&initial_value);
	const char* clear_chars = reinterpret_cast<const char*>(&clear_value);

	#pragma pack (push, 1)

	// Structure to track individual allocations of memory, every piece of memory
	// allocated will contain one of these just before the requested memory
	struct mem_alloc_info_t {
		mem_alloc_info_t* next;	// Next allocated memory chunk
		mem_alloc_info_t* prev;	// Previous allocated memory chunk
		const char* alloc_file;	// Filename where the memory was allocated
		int alloc_line;			// Line number in file where memory was allocated
		short method;			// Method of allocation
		short free_count;		// Number of times the memory was deleted
		int request;			// Number of the allocation
		size_t reported_size;	// Amount of memory requested by the user
		size_t actual_size;		// Actual size of the memory allocated
		char guard[guard_size];	// Guard buffer
	};

	#pragma pack (pop)

	// Utility functions used within the memory manager
	const char*	mem_alloc_method(int method);
	const char*	mem_free_method(int method);
	void mem_validate_free_method(mem_alloc_info_t* node, int method);
	bool mem_check_fore_guard(mem_alloc_info_t* node);
	bool mem_check_rear_guard(mem_alloc_info_t* node);
	void mem_fill_mem(void* dest, int size, const char* fill);
	void mem_remove_corrupted_alloc_info(mem_alloc_info_t* node);
	void mem_recover_alloc_infos(mem_alloc_info_t* node);

	// Structure to track overall allocation of memory
	struct memory_manager_info_t {
		memory_manager_info_t() { dummy.next = dummy.prev = &dummy; dummy.request = INT_MAX; mem_fill_mem(dummy.guard, guard_size, guard_chars); }

		int total_actual_memory_allocated;	// Total memory the memory manager has allocated
		int total_actual_memory_freed;		// Total memory the memory manager has freed
		int total_reported_memory_allocated;// Total memory the user has requested
		int total_reported_memory_freed;	// Total memory the user has freed
		int total_allocations;				// Total memory allocations by the user
		int total_frees;					// Total memory frees by the user
		int total_lost_nodes;				// Number of nodes missing from the list
		int peak_actual_memory_allocated;	// Most memory the memory manager had allocated at any time
		int peak_reported_memory_allocated;	// Most memory the user allocated at any time
		int current_actual_memory_allocated;// The amount of memory allocated at present
		int current_reported_memory_allocated;	// The amount of user memory allocated at present

		mem_alloc_info_t dummy;	// Dummy node for the mem_alloc_info_t list
	};

	// Structure to track overall allocation of memory
	memory_manager_info_t mminfo;

	// Methods that do the actual memory allocation and freeing
	inline void* mem_alloc_final(size_t size)	{ return malloc(size); }
	inline void mem_free_final(void* ptr)		{ free(ptr); }

	// Methods that do the allocation and freeing in debug mode
	void* mem_alloc_debug(size_t size, const char* file, const int line, const int method);
	void mem_free_debug(void* ptr, const int method);

	// Return the name of the allocation or freeing method
	const char* mem_alloc_method(int method);
	const char* mem_free_method(int method);

	struct logfile_t {
		logfile_t() { fp = fopen("memory.log", "w"); }
		~logfile_t() { if (fp) { fflush(fp); fclose(fp); } }
		operator FILE*() { return fp; }
		FILE* fp;
	};
	logfile_t log;
};

void
mem_report()
{
	fprintf(log, "================================================================================================\n");
	fprintf(log, "Application Memory Usage Report:\n");
	fprintf(log, "================================================================================================\n");
	fprintf(log, "Total Actual Memory Allocated:   %12d\n", mminfo.total_actual_memory_allocated);
	fprintf(log, "Total Actual Memory Freed:       %12d\n", mminfo.total_actual_memory_freed);
	fprintf(log, "Total Reported Memory Allocated: %12d\n", mminfo.total_reported_memory_allocated);
	fprintf(log, "Total Reported Memory Freed:     %12d\n", mminfo.total_reported_memory_freed);
	fprintf(log, "Total Memory Allocations:        %12d\n", mminfo.total_allocations);
	fprintf(log, "Total Memory Frees:              %12d\n", mminfo.total_frees);
	fprintf(log, "Peak Actual Memory Allocated :   %12d\n", mminfo.peak_actual_memory_allocated);
	fprintf(log, "Peak Reported Memory Allocated:  %12d\n", mminfo.peak_reported_memory_allocated);
	fprintf(log, "Memory Leaks:                    %12d\n", mminfo.total_allocations - mminfo.total_frees);
	fprintf(log, "Actual Memory Leaked:            %12d\n", mminfo.current_actual_memory_allocated);
	fprintf(log, "Reported Memory Leaked:          %12d\n", mminfo.current_reported_memory_allocated);
	fprintf(log, "================================================================================================\n");
	fprintf(log, "Memory Leaks (in order of allocation):\n");
	fprintf(log, "================================================================================================\n");
	fprintf(log, "Request Filename                       Line  Size (Actual)   Size (Reported) Address    Method\n");
	fprintf(log, "------- ------------------------------ ----- --------------- --------------- ---------- --------\n");
	for (mem_alloc_info_t* info = mminfo.dummy.next; info != &mminfo.dummy; info = info->next)
		fprintf(log, "%7d %-30s %5d %15d %15d %#x %s\n", info->request, info->alloc_file, info->alloc_line, 
			info->actual_size, info->reported_size, info, mem_alloc_method(info->method));
	fprintf(log, "================================================================================================\n");
}

#ifdef MEM_DEBUGGING_ENABLED

void*
operator new(size_t size, const char* file, const int line)
	// Debug mode operator new
{
	if (size == 0)
		size = 1;
	void* ptr;
	do {
		ptr = mem_alloc_debug(size, file, line, MEM_ALLOC_METHOD_NEW);
		if (ptr == 0) {
			new_handler nh = set_new_handler(0);
			set_new_handler(nh);
			nh();
		}
	} while (!ptr);
	return ptr;
}

void*
operator new[](size_t size, const char* file, const int line)
	// Debug mode operator new[]
{
	if (size == 0)
		size = 1;
	void* ptr;
	do {
		ptr = mem_alloc_debug(size, file, line, MEM_ALLOC_METHOD_NEW_ARRAY);
		if (ptr == 0) {
			new_handler nh = set_new_handler(0);
			set_new_handler(nh);
			nh();
		}
	} while (!ptr);
	return ptr;
}

void 
operator delete(void* ptr, const char* file, const int line)
	// Debug mode operator delete, this is only caused if an exception is thrown
	// in a constructor call, for an object allocated using the overloaded
	// operator new
{
	mem_free_debug(ptr, MEM_FREE_METHOD_DELETE);
}

void
operator delete[](void* ptr, const char* file, const int line)
	// Debug mode operator delete[], this is only caused if an exception is thrown
	// in a constructor call, for an object allocated using the overloaded
	// operator new[]
{
	mem_free_debug(ptr, MEM_FREE_METHOD_DELETE_ARRAY);
}

#endif

void*
operator new(size_t size)
	// Operator new
{
	if (size == 0)
		size = 1;
	void* ptr;
	do {
#ifdef MEM_DEBUGGING_ENABLED
		ptr = mem_alloc_debug(size, "<new>", 0, MEM_ALLOC_METHOD_NEW);
#else
		ptr = mem_alloc_final(size);
#endif
		if (ptr == 0) {
			new_handler nh = set_new_handler(0);
			set_new_handler(nh);
			nh();
		}
	} while (!ptr);
	return ptr;
}

void*
operator new[](size_t size)
{
	if (size == 0)
		size = 1;
	void* ptr;
	do {
#ifdef MEM_DEBUGGING_ENABLED
		ptr = mem_alloc_debug(size, "<new>", 0, MEM_ALLOC_METHOD_NEW_ARRAY);
#else
		ptr = mem_alloc_final(size);
#endif
		if (ptr == 0) {
			new_handler nh = set_new_handler(0);
			set_new_handler(nh);
			nh();
		}
	} while (!ptr);
	return ptr;
}

void
operator delete(void* ptr)
{
#ifdef MEM_DEBUGGING_ENABLED
	mem_free_debug(ptr, MEM_FREE_METHOD_DELETE);
#else
	mem_free_final(ptr);
#endif
}

void
operator delete[](void* ptr)
{
#ifdef MEM_DEBUGGING_ENABLED
	mem_free_debug(ptr, MEM_FREE_METHOD_DELETE_ARRAY);
#else
	mem_free_final(ptr);
#endif
}

namespace {

	const char*
	mem_alloc_method(int method)
	{
		// Names of the mem alloc methods, indexes should match the mem alloc consts
		static const char* mem_alloc_method_names[] = {
			"new",
			"new[]",
		};

		if (method < sizeof(mem_alloc_method_names) / sizeof(*mem_alloc_method_names))
			return mem_alloc_method_names[method];
		return "InvalidAllocMethod!";
	}

	const char*
	mem_free_method(int method)
	{
		// Names of the mem free methods, indexes should match the mem free consts
		static const char* mem_free_method_names[] = {
			"delete",
			"delete[]",
		};

		if (method < sizeof(mem_free_method_names) / sizeof(*mem_free_method_names))
			return mem_free_method_names[method];
		return "InvalidFreeMethod!";
	}

	void
	mem_validate_free_method(mem_alloc_info_t* node, int method)
	{
		if (node->method != method) {
			fprintf(log, "Error: Memory allocated with %s released with %s"
						"(request %d, file %s, line %d, size(actual) %d, size(reported) %d)\n", 
				mem_alloc_method(node->method), mem_free_method(method), node->request, node->alloc_file,
				node->alloc_line, node->actual_size, node->reported_size);
		}
	}

	bool
	mem_check_fore_guard(mem_alloc_info_t* node)
		// Check if the fore guard is still valid, returns true if it is
	{
		char* fore_guard = node->guard;
		for (int i = 0; i < guard_size; ++i)
			if (*fore_guard++ != guard_chars[i & 0x3]) {
				u_assert(!break_on_fore_guard_corruption);
				return false;
			}
		return true;
	}

	bool
	mem_check_rear_guard(mem_alloc_info_t* node)
		// Check if the rear guard is still valid, returns true if it is
	{
		char* rear_guard = reinterpret_cast<char*>(node + 1) + node->reported_size;
		for (int i = 0; i < guard_size; ++i)
			if (*rear_guard++ != guard_chars[i & 0x3]) {
				u_assert(!break_on_rear_guard_corruption);
				return false;
			}
		return true;
	}

	void
	mem_fill_mem(void* dest, int size, const char* fill)
		// Fill a region of memory with the data in fill
	{
		char* buffer = reinterpret_cast<char*>(dest);
		for (int i = 0; i < size; ++i)
				*buffer++ = fill[i & 0x3];
	}

	void
	mem_remove_corrupted_alloc_info(mem_alloc_info_t* node)
		// Remove a node whose node information has been overwritten by garbage
		// from the list of memory and fix the memory list. This wont work very
		// well if a lot of nodes are corrupted, as at some stage the pointers
		// may have been overwritten.
	{
		mem_alloc_info_t* prev;
		mem_alloc_info_t* next;

		//++mminfo.total_frees;
		//mminfo.total_actual_memory_freed += info->actual_size;
		//mminfo.total_reported_memory_freed += info->reported_size;
		//mminfo.current_actual_memory_allocated -= info->actual_size;
		//mminfo.current_reported_memory_allocated -= info->reported_size;

		// Find the node before the corrupted node
		for (prev = mminfo.dummy.next; prev != &mminfo.dummy; prev = prev->next)
			if (prev->next == node || !mem_check_fore_guard(prev->next))
				break;
		// Find the node after the corrupted node
		for (next = mminfo.dummy.prev; next != &mminfo.dummy; next = next->prev)
			if (prev->next == node || !mem_check_fore_guard(prev->next))
				break;
		if (prev->next == node && next->prev == node) {
			// This is the only corrupt node so just dump it
			fprintf(log, "Corrupted memory released: Allocation was between request %d and request %d at address %#x, size unknown\n",
				prev->request, next->request, node);
			prev->next = next;
			next->prev = prev;
			++mminfo.total_frees;
			mem_free_final(node);
		} else if (prev == &mminfo.dummy && next == &mminfo.dummy) {
			// The node no longer exists in the list, it has either been deleted already
			// or it is one of the lost nodes if any exist
			if (mminfo.total_lost_nodes == 0) {
				fprintf(log, "Multiple deletion of memory at address %#x\n", node);
				u_assert(!break_on_multiple_deletion);
			} else {
				fprintf(log, "Unrecognized corrupt node found at address %#x\n", node);
				--mminfo.total_lost_nodes;
				++mminfo.total_frees;
				if (!prefer_mem_leaks_to_crashes)
					mem_free_final(node);
			}
		} else {
			// It is impossible to fully traverse the node list from either end
			// multiple nodes have been corrupted, Repair the node list as much
			// as possible
			prev->next = next;
			next->prev = prev;
			int remaining_nodes = 0;
			for (next = mminfo.dummy.next; next != &mminfo.dummy; next = next->next)
				++remaining_nodes;
			mminfo.total_lost_nodes = (mminfo.total_allocations - mminfo.total_frees) - remaining_nodes;
			if (mminfo.total_lost_nodes) {
				fprintf(log, "Allocation list corrupted: %d nodes lost\n", mminfo.total_lost_nodes);
				u_assert(!break_on_node_loss);
			}
			++mminfo.total_frees;
			if (!prefer_mem_leaks_to_crashes)
				mem_free_final(node);
		}
	}

	void
	mem_recover_alloc_infos(mem_alloc_info_t* node)
	{
		// Find the earliest node in the node list
		mem_alloc_info_t* recover_start;
		for (recover_start = node; recover_start != &mminfo.dummy; recover_start = recover_start->prev) {
			if (!mem_check_fore_guard(recover_start->prev))
				break;
		}
		// If we tracked back to the start of the alloc list the node was already in our list
		if (recover_start == &mminfo.dummy)
			return;

		mem_alloc_info_t* recover_end;
		for (recover_end = node; recover_end != &mminfo.dummy; recover_end = recover_end->next) {
			if (!mem_check_fore_guard(recover_end->next))
				break;
		}

		// If we tracked to the end of the alloc list the node was already in our list
		if (recover_end == &mminfo.dummy) {
			u_assert(false); // This should never happen, as long as a node can be traced from either
			return;			 // the beginning or the end of the alloc list it should be tracable from both
		}

		// At this point we have recovered a bunch of nodes, reinsert them into the list
		mem_alloc_info_t* insert_pos = mminfo.dummy.next;
		while (insert_pos->request < recover_start->request)
			insert_pos = insert_pos->next;
		// Insert the recovered nodes into the list
		recover_start->prev = insert_pos;
		recover_end->next = insert_pos->next;
		insert_pos->next->prev = recover_end;
		insert_pos->next = recover_start;
		
		int count = 1;
		while (recover_start != recover_end) {
			recover_start = recover_start->next;
			++count;
		}
		mminfo.total_lost_nodes -= count;
		fprintf(log, "Recovered %d lost allocation nodes, requests %d to %d\n", count, insert_pos->next->request, recover_end->request);
		u_assert(!break_on_node_recovery);
	}

	void*
	mem_alloc_debug(size_t size, const char* file, const int line, int method)
		// Allocate memory in debug mode
	{
		mem_alloc_info_t* info;
		int alloc_size = size + sizeof(mem_alloc_info_t) + guard_size;
		info = static_cast<mem_alloc_info_t*>(mem_alloc_final(alloc_size));

		if (info == 0)
			return 0;

		// Update mminfo
		++mminfo.total_allocations;
		u_assert(mminfo.total_allocations != break_on_alloc);
		mminfo.total_actual_memory_allocated += alloc_size;
		mminfo.total_reported_memory_allocated += size;
		mminfo.current_actual_memory_allocated += alloc_size;
		if (mminfo.peak_actual_memory_allocated < mminfo.current_actual_memory_allocated)
			mminfo.peak_actual_memory_allocated = mminfo.current_actual_memory_allocated;
		mminfo.current_reported_memory_allocated += size;
		if (mminfo.peak_reported_memory_allocated < mminfo.current_reported_memory_allocated)
			mminfo.peak_reported_memory_allocated = mminfo.current_reported_memory_allocated;

		// Set infos fields and insert it into the memory list
		info->alloc_file = file;
		info->alloc_line = line;
		info->method = method;
		info->request = mminfo.total_allocations;
		info->reported_size = size;
		info->actual_size = alloc_size;
		info->next = &mminfo.dummy;
		info->prev = mminfo.dummy.prev;
		info->next->prev = info;	// mminfo.dummy.prev = info
		info->prev->next = info;

		// Fill fore guard, buffer and rear guard with initializer crap
		mem_fill_mem(info->guard, guard_size, guard_chars);
		mem_fill_mem(info + 1, size, initial_chars);
		mem_fill_mem(reinterpret_cast<char*>(info + 1) + size, guard_size, guard_chars);

		if (log_all_allocations) {
			if (mminfo.total_allocations == 1) {
				fprintf(log, "Request Filename                       Line  Size (Actual)   Size (Reported) Address    Method\n");
				fprintf(log, "------- ------------------------------ ----- --------------- --------------- ---------- --------\n");
			}
			fprintf(log, "%7d %-30s %5d %15d %15d %#x %s\n", info->request, info->alloc_file, info->alloc_line, 
				info->actual_size, info->reported_size, info, mem_alloc_method(info->method));
		}
		return static_cast<void*>(info + 1);	
	}

	void
	mem_free_debug(void* ptr, const int method)
		// Free memory in debug mode
	{
		if (ptr == 0)
			return;

		mem_alloc_info_t* info = static_cast<mem_alloc_info_t*>(ptr) - 1;

//	Aggressive multiple deletion check, slows things down a lot
//		mem_alloc_info_t* start;
//		for (start = mminfo.dummy.next; start != &mminfo.dummy; start = start->next)
//			if (start == info)
//				break;
//		if (start == &mminfo.dummy)
//			return;

		// Check the fore guard for corruption
		if (mem_check_fore_guard(info)) {
			if (mem_check_rear_guard(info)) {
				// Check the memory was freed with the correct method
				mem_validate_free_method(info, method);
				// Fill the memory with crap
				mem_fill_mem(info + 1, info->reported_size, clear_chars);
				// Check if we need to try and recover lost nodes from info
				if (mminfo.total_lost_nodes)
					mem_recover_alloc_infos(info);
				// Log the request if nescessary
				if (log_all_frees) {
					fprintf(log, "%7d %-30s %5d %15d %15d %#x %s\n", info->request, info->alloc_file, info->alloc_line, 
						info->actual_size, info->reported_size, info, mem_free_method(info->method));
				}
				// Take info out of the list
				info->prev->next = info->next;
				info->next->prev = info->prev;
				// Update mminfo
				++mminfo.total_frees;
				mminfo.total_actual_memory_freed += info->actual_size;
				mminfo.total_reported_memory_freed += info->reported_size;
				mminfo.current_actual_memory_allocated -= info->actual_size;
				mminfo.current_reported_memory_allocated -= info->reported_size;
				// Finally free the memory
				mem_free_final(info);
			}
		} else {
			// Make an attempt to fix the node lists, if the node is corrupted
			// then mem_remove_corrupted_alloc_info is responsible for the
			// updating of mminfo
			mem_remove_corrupted_alloc_info(info);
		}
	}
};
*/