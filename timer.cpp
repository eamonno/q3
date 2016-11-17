//-----------------------------------------------------------------------------
// File: timer.cpp
//
// Implementation of the timer
//-----------------------------------------------------------------------------

#include "timer.h"
#include "maths.h"
#include "win.h"

#include <memory>

#include "mem.h"
#define new mem_new

timer_t&
timer_t::get_instance()
{
	static std::auto_ptr<timer_t> instance(new timer_t());
	return *instance;
}

bool
timer_t::init()
{
	LARGE_INTEGER li;
	if (QueryPerformanceFrequency(&li)) {
		cpu_frequency = li.QuadPart;
		return cpu_frequency != 0;
	}
	return false;
}

void
timer_t::start(timerid_t timerid)
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	timers[timerid].start_ticks = li.QuadPart;
	timers[timerid].mark_ticks = li.QuadPart;
	timers[timerid].elapsed_time = 0.0f;
	timers[timerid].mark_time = 0.0f;
}

void
timer_t::mark(timerid_t timerid)
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	__int64 now = li.QuadPart;
	timers[timerid].elapsed_time = (m_lltof(now) - m_lltof(timers[timerid].mark_ticks)) / m_lltof(cpu_frequency);
	timers[timerid].mark_ticks = now;
	timers[timerid].mark_time = (m_lltof(now) - m_lltof(timers[timerid].start_ticks)) / m_lltof(cpu_frequency);
}
