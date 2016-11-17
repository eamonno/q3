//-----------------------------------------------------------------------------
// File: daudio.h
//
// DirectX audio controller class
//-----------------------------------------------------------------------------

#ifndef DAUDIO_H
#define DAUDIO_H

#include "dxcommon.h"
#include "types.h"
#include "util.h"
#include "maths.h"

//#define INITGUID
#include <dmusici.h>

class daudio_t {
public:
	~daudio_t()		{ destroy(); }

	result_t	create();
	void		destroy();

	hsegment_t		get_segment(const char* name);

	haudiopath_t	create_path(const vec3_t& pos, const vec3_t& dir);
	void			free_path(haudiopath_t path);

	void	set_pos(haudiopath_t path, const vec3_t& pos);
	void	set_dir(haudiopath_t path, const vec3_t& dir);

	void	play_sound(hsound_t sound, haudiopath_t path, bool repeat = false);
	void	stop_sound(hsound_t sound, haudiopath_t path);

	bool	is_playing(hsound_t segment, haudiopath_t path);
	bool	is_playing(hsound_t segment);

	static daudio_t& get_instance();

private:
	struct audiopath_t;
	struct sound_t;

	daudio_t() :
		sounds(0),
		paths(0)
	{}

	sound_t* sounds;
	audiopath_t* audiopaths;

	com_ptr_t<IDirectMusicPerformance8>	performance;
	com_ptr_t<IDirectMusicLoader8>		loader;
};

#define daudio (daudio_t::get_instance())

#endif
