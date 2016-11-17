//-----------------------------------------------------------------------------
// File: exec.cpp
//
// Implementation for the exec object
//-----------------------------------------------------------------------------

#include "exec.h"
#include "util.h"
#include "console.h"
#include "mem.h"
#include <memory>

using std::auto_ptr;

#define new mem_new

/*
template class cvar_t<int>;
template class cvar_t<float>;
template class cvar_t<cvstr_t>;
template class rvar_t<float, 0.0f, 1.0f>;
template class rvar_t<int, 0, 1>;
template class rvar_t<int, 2, 3>;
*/

basecvar_t* basecvar_t::first = 0;
basecvar_t* basecvar_t::last = 0;
cfunc_t* cfunc_t::first = 0;
cfunc_t* cfunc_t::last = 0;

basecvar_t::basecvar_t(const char* name, uint flags) : 
	cvname(name),
	cvflags(flags)
	// Create the cvar and add it to the cvar list
{
	if (first == 0) {
		first = this;
		last = this;
		next = 0;
		prev = 0;
	} else {
		basecvar_t* ptr = first;
		while (ptr && cvname > ptr->cvname)
			ptr = ptr->next;
		if (ptr == 0) {
			prev = last;
			prev->next = this;
			last = this;
			next = 0;
		} else if (ptr == first) {
			first->prev = this;
			next = first;
			prev = 0;
			first = this;
		} else {
			next = ptr;
			prev = next->prev;
			if (prev)
				prev->next = this;
			next->prev = this;
		}
	}
}

basecvar_t::~basecvar_t() 
	// Remove the cvar from the cvar list
{
	if (prev)
		prev->next = next;
	else
		first = next;

	if (next)
		next->prev = prev;
	else
		last = prev;
}

basecvar_t*
basecvar_t::find_cvar(const char* name)
{
	basecvar_t* var = basecvar_t::first;
	while (var && var->cvname.cmpi(name) != 0)
		var = var->next;
	return var;
}

void
basecvar_t::cvar_list(const char* stub)
{
	int count = 0;
	for (basecvar_t* var = first; var; var = var->next) {
		console.printf("  %s=%s, default=%s\n", var->cvname.c_str(), var->get_value().c_str(), var->get_default().c_str());
		++count;
	}
	console.printf("Total %d cvars\n", count);
}

namespace {
	cvstr_t
	cvarlist_callback(int argc, cvstr_t* argv)
	{
		basecvar_t::cvar_list("");
		
		return cvstr_t();
	}

	cfunc_t cfunc_cvarlist("cvarlist", cvarlist_callback, 0, 1);
}

cfunc_t::cfunc_t(const char* name, execfunc_t func, int min_params, int max_params) :
	cfname(name),
	cffunc(func),
	cfmin(min_params),
	cfmax(max_params)
{
	if (first == 0) {
		first = this;
		last = this;
		next = 0;
		prev = 0;
	} else {
		cfunc_t* ptr = first;
		while (ptr && cfname > ptr->cfname)
			ptr = ptr->next;
		if (ptr == 0) {
			prev = last;
			prev->next = this;
			last = this;
			next = 0;
		} else if (ptr == first) {
			first->prev = this;
			next = first;
			prev = 0;
			first = this;
		} else {
			next = ptr;
			prev = next->prev;
			if (prev)
				prev->next = this;
			next->prev = this;
		}
	}
}

cfunc_t::~cfunc_t()
{
	if (prev)
		prev->next = next;
	else
		first = next;

	if (next)
		next->prev = prev;
	else
		last = prev;
}

cfunc_t*
cfunc_t::find_cfunc(const char* name)
{
	cfunc_t* func = first;
	while (func && func->cfname.cmpi(name) != 0)
		func = func->next;
	return func;
}

/*
void cvar_t<float>::set_value(const char* txt)		{ u_strtof(txt, cvdata, cvdata); }
void cvar_t<int>::set_value(const char* txt)		{ u_strtoi(txt, cvdata, cvdata); }
void cvar_t<cvstr_t>::set_value(const char* txt)	{ cvdata = txt; }

const cvstr_t cvar_t<float>::get_value()   const	{ return cvstr_t(cvdata); }
const cvstr_t cvar_t<int>::get_value()     const	{ return cvstr_t(cvdata); }
const cvstr_t cvar_t<cvstr_t>::get_value() const	{ return cvdata; }
*/

exec_t&
exec_t::get_instance()
	// Return the single global exec object
{
	static auto_ptr<exec_t> instance(new exec_t());
	return *instance;
}

void	
exec_t::exec_script(const char* script, int length)
	// Execute the contents of the text buffer script
{
	tokenizer_t tokenizer(script, length);

	cvstr_t field;
	cvstr_t value;

	while (tokenizer.next_token(field.str(), field.size())) {
		if (tokenizer.next_token(value.str(), value.size())) {
			cfunc_t* func = cfunc_t::find_cfunc(field);
			if (func) {
				func->cffunc(1, &value);
			} else {
				basecvar_t* var = basecvar_t::find_cvar(field);
				if (var)
					if (consts_locked && var->cvflags & CVF_CONST)
						console.printf("%s is a constant and cannot be modified at runtime\n", var->cvname.c_str());
					else
						var->set_value(value);
				else
					console.printf("unknown command: %s\n", field.c_str());
			}
		}
	}
}
