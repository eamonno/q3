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

#undef mem_new

namespace {

	/////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration variables - Changing these will alter the behaviour of the Memory Manager //
	/////////////////////////////////////////////////////////////////////////////////////////////

	const int guard_size	 = 4;			// Size of guard buffers
	const int break_on_alloc = -1;			// Break when this request arrives, -1 to disable

	const bool break_on_fore_guard_corruption = true;	// Break if fore guard is mangled
	const bool break_on_rear_guard_corruption = true;	// Break if rear guard is mangled
	const bool break_on_node_loss = true;			// Break if the nodes go missing from the list
	const bool break_on_node_recovery = true;		// Break if lost nodes are recovered
	const bool break_on_multiple_deletion = true;	// Break if a node is deleted 2 or more times
	const bool break_on_incorrect_delete = true;	// Break if delete[] used with new and vice versa
	const bool log_all_allocations = true;			// Log every allocation of memory
	const bool log_all_frees = true;				// Log every time memory is freed
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

	bool add_to_free_list = true;

	struct free_list_node_t {
		free_list_node_t() { next = this; prev = this; size = 0; }	// only needed for dummy
		~free_list_node_t() 
		{
			while (next != this) {
				free_list_node_t* tmp = next->next;
				free(next);
				next = tmp;
			}
			add_to_free_list = false;	// any more memory freed just gets freed
		}

		uint				size;	// sizeof memory allocated
		free_list_node_t*	next;
		free_list_node_t*	prev;
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

	free_list_node_t	free_list;

	struct logfile_t {
		logfile_t() { fp = fopen("memory.log", "w"); }
		~logfile_t() { if (fp) { fflush(fp); fclose(fp); } }
		operator FILE*() { return fp; }
		FILE* fp;
	};
	logfile_t log;

	// Methods that do the actual memory allocation and freeing
	inline void*
	mem_alloc_final(size_t size)
	{
		size += sizeof(int);	// We need an extra int

		// Find the smallest free_list node that will suffice
		free_list_node_t* best = 0;
		for (free_list_node_t* node = free_list.next; node != &free_list; node = node->next)
			if (node->size >= size && (!best || (node->size < best->size)))
				best = node;
		if (best) {
			best->next->prev = best->prev;
			best->prev->next = best->next;
#ifdef MEM_DEBUGGING_ENABLED
			fprintf(log, "using node from free list: requested size = %d, node size = %d\n", size - sizeof(int), best->size);
#endif
		} else {
			best = static_cast<free_list_node_t*>(malloc(u_max(sizeof(free_list_node_t), size)));
			if (best) {
				best->size = u_max(sizeof(free_list_node_t), size);
#ifdef MEM_DEBUGGING_ENABLED
				fprintf(log, "allocated new node: size = %d\n", best->size);
			} else {
				fprintf(log, "malloc failed for size: %d\n", u_max(sizeof(free_list_node_t), size));
#endif
			}
		}
		if (best)
			return reinterpret_cast<int*>(best) + 1;
		return 0;
	}

	inline void 
	mem_free_final(void* ptr)
	{
		int* iptr = static_cast<int*>(ptr);
		if (add_to_free_list) {
			free_list_node_t* node = reinterpret_cast<free_list_node_t*>(iptr - 1);
#ifdef MEM_DEBUGGING_ENABLED
			for (free_list_node_t* test = free_list.next; test != &free_list; test = test->next)
				if (test == node)
					u_break();	// Adding the same node twice
#endif
			node->next = free_list.next;
			free_list.next = node;
			node->prev = &free_list;
			node->next->prev = node;
#ifdef MEM_DEBUGGING_ENABLED
			fprintf(log, "node added to free list: size = %d\n", node->size);
#endif
		} else {
			free(iptr - 1); 
		}
	}

	// Methods that do the allocation and freeing in debug mode
	void* mem_alloc_debug(size_t size, const char* file, const int line, const int method);
	void mem_free_debug(void* ptr, const int method);

	// Return the name of the allocation or freeing method
	const char* mem_alloc_method(int method);
	const char* mem_free_method(int method);
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
	if (ptr == 0)
		return;
#ifdef MEM_DEBUGGING_ENABLED
	mem_free_debug(ptr, MEM_FREE_METHOD_DELETE);
#else
	mem_free_final(ptr);
#endif
}

void
operator delete[](void* ptr)
{
	if (ptr == 0)
		return;
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
			u_assert(!break_on_incorrect_delete);
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
