//-----------------------------------------------------------------------------
// File: exec.h
//
// Executable commands/scripts and cvar classes. The cvars were implemented
// using the template based versions below, but since the compiler kept choking
// on them nescessitating a full rebuild nearly every time any change was made
// they were replaced with explicit derivations of basecvar_t
//-----------------------------------------------------------------------------

#ifndef EXEC_H
#define EXEC_H

#include "str.h"
#include <limits.h>
#include <float.h>

class exec_t;

typedef str_t<64>	cvname_t;
typedef str_t<64>	cvstr_t;

// Cvar flags
#define CVF_NONE		0x0			// No flags set
#define CVF_CONST		0x1			// can be changed through startup.scr but not in game
#define CVF_HIDDEN		0x2			// doesnt show on cvarlist

class basecvar_t {
public:
	friend class exec_t;

	basecvar_t(const char* name, uint flags);
	~basecvar_t();

	virtual void			set_value(const char* value) = 0;
	virtual const cvstr_t	get_value() const = 0;
	virtual const cvstr_t	get_default() const = 0;

	static	basecvar_t*		find_cvar(const char* name);
	static	void			cvar_list(const char* stub);

protected:
	static basecvar_t*	first;
	static basecvar_t*	last;

	const cvname_t	cvname;
	int				cvflags;
	basecvar_t*	next;
	basecvar_t*	prev;
};

class cvar_int_t : public basecvar_t {
	// Integer cvar
public:
	cvar_int_t(const char* name, int data, uint flags = CVF_NONE, int min = INT_MIN, int max = INT_MAX) : 
		basecvar_t(name, flags),
		cvdefault(data),
		cvdata(data),
		cvmin(min),
		cvmax(max)
	{}
	
	int& operator*()	{ return cvdata; }

	void set_value(const char* value)	{ u_strtoi(value, cvdata, cvdata); cvdata = u_min(cvmax, u_max(cvmin, cvdata)); }
	const cvstr_t get_value() const		{ return cvstr_t(cvdata); }
	const cvstr_t get_default() const	{ return cvstr_t(cvdefault); }

private:
	int cvdefault;
	int cvdata;
	int cvmin;
	int cvmax;
};

class cvar_float_t : public basecvar_t {
	// Floating point cvar
public:
	cvar_float_t(const char* name, float data, uint flags = CVF_NONE, float min = FLT_MIN, float max = FLT_MAX) : 
		basecvar_t(name, flags),
		cvdefault(data),
		cvdata(data),
		cvmin(min),
		cvmax(max)
	{}
	
	float& operator*()	{ return cvdata; }

	void set_value(const char* value)	{ u_strtof(value, cvdata, cvdata); cvdata = u_min(cvmax, u_max(cvmin, cvdata)); }
	const cvstr_t get_value() const		{ return cvstr_t(cvdata); }
	const cvstr_t get_default() const	{ return cvstr_t(cvdefault); }

private:
	float cvdefault;
	float cvdata;
	float cvmin;
	float cvmax;
};

class cvar_str_t : public basecvar_t {
	// String cvar
public:
	cvar_str_t(const char* name, const cvstr_t& data, uint flags = CVF_NONE) : 
		basecvar_t(name, flags),
		cvdefault(data),
		cvdata(data)
	{}
	
	cvstr_t& operator*()	{ return cvdata; }
	cvstr_t* operator->()	{ return &cvdata; }

	void set_value(const char* value)	{ cvdata = value; }
	const cvstr_t get_value() const		{ return cvdata; }
	const cvstr_t get_default() const	{ return cvdefault; }

private:
	cvstr_t cvdefault;
	cvstr_t cvdata;
};

// execfunc_t, console functions, must taks as paramaters an int and an array
// of cvstr_t's. The int specifies the number of cvstrs passed for this invocation
// and may be 0 only if the int paramater is also 0
typedef cvstr_t (*execfunc_t)(int, cvstr_t*);

class cfunc_t {
public:
	friend class exec_t;

	cfunc_t(const char* name, execfunc_t func, int min_params = 0, int max_params = 0);
	~cfunc_t();

	static	cfunc_t*	find_cfunc(const char* name);

private:
	static cfunc_t*		first;
	static cfunc_t*		last;

	cfunc_t*			next;
	cfunc_t*			prev;

	const cvstr_t		cfname;
	const execfunc_t	cffunc;
	const int			cfmin;
	const int			cfmax;
};

/*
template <class T>
class cvar_t : public basecvar_t {
public:
	cvar_t(const char* name, const T& value) : basecvar_t(name), cvdata(value)	{}

	virtual void			set_value(const char* value);
	virtual const cvstr_t	get_value() const;

#pragma warning(disable: 4284)	// using infix notation for simple types

	T& operator*()				{ return cvdata; }
	T* operator->()				{ return &cvdata; }

	const T& operator*() const	{ return cvdata; }
	const T* operator->() const	{ return &cvdata; }

#pragma warning(default: 4284)

protected:
	T	cvdata;
};

template <class T, T MIN, T MAX>
class rvar_t : public cvar_t<T> {
	// rvars, ranged cvars
public:
	rvar_t(const char* name, const T& value) : cvar_t<T>(name, value) {}

	void set_value(const char* txt);
};

template <class T, T MIN, T MAX>
void rvar_t<T, MIN, MAX>::set_value(const char* txt)
{
	cvar_t<T>::set_value(txt);
	if (cvdata > MAX)
		cvdata = MAX;
	if (cvdata < MIN)
		cvdata = MIN;
}
*/

//typedef void *(execcallback_t)(const basecvar_t*, int);
//
//class execcmd_t
//{
//	execcmd_t(const char* name, ececcallback_t callback, int params = 0, int optional_params = 0)
//	{
//	}
//};
//
//execcmd_t execcmd_map("map", do_map_change, 1, 0);

class exec_t {
public:
	void lock_consts() { consts_locked = true; }
	void exec_script(const char* script, int length);

	static exec_t& get_instance();

private:
	exec_t() : consts_locked(false) {}

	bool consts_locked;
};

#define exec (exec_t::get_instance())

#endif
