//-----------------------------------------------------------------------------
// File: timer.h
//
// Timer class
//-----------------------------------------------------------------------------

#ifndef TIMER_H
#define TIMER_H

// Number of times per second the timer updates
#define GAME_TICKRATE		(60.0f)

// Timer identifiers
enum timerid_t {
	TID_APP,		// Application timer, time since the app started
	TID_PROFILE0,	// Profiler timer
	TID_PROFILE1,	// Profiler timer
	TID_PROFILE2,	// Profiler timer
	TID_PROFILE3,	// Profiler timer
	TID_PROFILE4,	// Profiler timer
	TID_PROFILE5,	// Profiler timer
	TID_PROFILE6,	// Profiler timer
	TID_PROFILE7,	// Profiler timer
	TID_PROFILE8,	// Profiler timer
	TID_PROFILE9,	// Profiler timer
	MAX_TID,		// Must always be the last entry
};

/*
struct timer_t {

	__int64	start_time;		// Time the timer was last started
	__int64 ela
	static __int64 cpu_frequency;
};
*/

class timer_t {
	// Timer, keeps track of when it is in the game world
public:
	bool init();
	void start(timerid_t timer);
	void mark(timerid_t timer);
	float time(timerid_t timer) { return timers[timer].mark_time; }
	float elapsed(timerid_t timer) { return timers[timer].elapsed_time; }

	static timer_t& get_instance();

private:
	timer_t() {}

	struct {
		float elapsed_time;
		float mark_time;
		__int64 start_ticks;
		__int64 mark_ticks;
	} timers[MAX_TID];
	__int64 cpu_frequency;
};

#define timer (timer_t::get_instance())

#endif
