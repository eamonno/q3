//-----------------------------------------------------------------------------
// File: dxcommon.h
//
// Functionality used anywhere that DirectX is being used
//-----------------------------------------------------------------------------

#ifndef DXCOMMON_H
#define DXCOMMON_H

template <class T>
class com_ptr_t {
	// A pointer to a COM object, automatically calls release when needed. Any
	// assignments will call AddRef for the object to which the com_ptr_t is
	// assigned. This behaviour can be overridden for the constructor call but
	// in all other cases make sure that if the pointer being assigned has had
	// its reference count increased that the reference is decreased after the
	// assignment.
public:
	com_ptr_t() : ptr(0) {}
	com_ptr_t(T* t, bool addref = true) : ptr(t) { if (addref) add_ref(); }
	com_ptr_t(const com_ptr_t& c) { ptr = c.ptr; add_ref(); }
	~com_ptr_t() { release(); }
	
	T* operator->() { return ptr; }
	T& operator*() { return *ptr; }
	operator bool() { return ptr != 0; }
	operator T*() { return ptr; }
	T** operator&() { return &ptr; }
	T* get() { return ptr; }

	com_ptr_t& operator=(T* t) { release(); ptr = t; add_ref(); return *this; }
	com_ptr_t& operator=(const com_ptr_t& p) { release(); ptr = p.ptr; add_ref(); return *this; }
	void add_ref() { if (ptr) ptr->AddRef(); }
	void release() { if (ptr) { ptr->Release(); } ptr = 0; }
	int ref_count() { if (ptr) { ptr->AddRef(); return ptr->Release(); } return 0; }
	void set(T* t, bool addref = true) { release(); ptr = t; if (addref) add_ref(); }

protected:
	T* ptr;
};

#endif
