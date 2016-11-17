//-----------------------------------------------------------------------------
// File: daudio.cpp
//
// DirectX audio controller class
//-----------------------------------------------------------------------------

#include "daudio.h"
#include "mem.h"
#include "str.h"
#include "pak.h"
#include "console.h"
#include "exec.h"
#include <memory>

#define new mem_new

cvstr_t
playsound_callback(int argc, cvstr_t* argv)
{
	hsegment_t segment = daudio.get_segment(argv[0]);
	if (segment)
		daudio.play_segment(segment);
	return cvstr_t();
}

cvar_int_t	nosound("nosound", 0, CVF_CONST, 0, 1);
cvar_int_t	max_sound_segments("max_sound_segments", 32, CVF_CONST, 0);
cvar_int_t	max_sound_paths("max_sound_paths", 32, CVF_CONST, 0);
cfunc_t		cf_playsound("playsound", playsound_callback, 1, 1);

struct daudio_t::sound_t {
	str_t<64> name;
	file_t* file;
	com_ptr_t<IDirectMusicSegment8> segment;

	sound_t() : file(0) {}

	void destroy() {
		name[0] = '\0';
		delete file;
		segment = 0;
	}
};

struct daudio_t::audiopath_t {
	com_ptr_t<IDirectMusicAudioPath> path;
	com_ptr_t<IDirectSound3DBuffer8> buffer;
};

daudio_t&
daudio_t::get_instance()
{
	static std::auto_ptr<daudio_t> instance(new daudio_t());

	return *instance;
}

result_t
daudio_t::create()
{
	if (*nosound)
		return true;

	HRESULT hr;

	// Initialize COM
	hr = CoInitialize(NULL);
	if (FAILED(hr))
		return "CoInitialize failed";

	// Create the loader
	hr = CoCreateInstance(
		CLSID_DirectMusicLoader, 
		NULL, 
		CLSCTX_INPROC, 
		IID_IDirectMusicLoader8,
		reinterpret_cast<void**>(&loader)
	);
	if (FAILED(hr))
		return "Failed to create IDirectMusicLoader8 instance";
	console.printf("DirectMusic: IDirectMusicLoader8 created successfully\n");

	// Create the performance
	hr = CoCreateInstance(
		CLSID_DirectMusicPerformance, 
		NULL, 
		CLSCTX_INPROC, 
		IID_IDirectMusicPerformance8,
		reinterpret_cast<void**>(&performance)
	);
	if (FAILED(hr))
		return "Failed to create IDirectMusicPerformance8 instance";
	console.printf("DirectMusic: IDirectMusicPerformance8 created successfully\n");

	// Initialize the performance
	hr = performance->InitAudio(
		NULL,
		NULL,
		NULL,
		DMUS_APATH_DYNAMIC_3D,//DMUS_APATH_SHARED_STEREOPLUSREVERB,
		64,
		DMUS_AUDIOF_ALL,
		NULL
	);
	if (FAILED(hr))
		return "IDirectMusicPerformance8->InitAudio() failed";
	console.printf("DirectMusic: IDirectMusicPerformance8->InitAudio() succeeded\n");

	sounds = new sound_t[*max_sound_segments];
	paths = new audiopath_t[*max_sound_paths];

	sounds[0].name = "<null>";

	return true;
}

void
daudio_t::destroy()
{
	if (*nosound)
		return;

	if (loader != 0 || performance != 0) {
		if (performance != 0) {
			performance->Stop(NULL, NULL, 0, 0);
			performance->CloseDown();
		}
		loader = 0;			// Releases if nescessary
		performance = 0;	// Releases if nescessary

		for (int i = 0; i < *max_sound_segments; ++i)
			sounds[i].destroy();

		CoUninitialize();
	}

	delete [] sounds;
	sounds = 0;
	delete [] paths;
	paths = 0;
}

hsegment_t
daudio_t::get_segment(const char* name)
{
	if (*nosound)
		return 0;

	// Find an unused sound resource
	for (int i = 0; i < *max_sound_segments; ++i)
		if (segments[i].name[0] == '\0')
			break;
		else if (segments[i].name.cmpi(name) == 0)
			return i;
	
	// Cannot load any more sounds
	if (i == *max_sound_segments) {
		console.printf("max_sound_segments limit reached loading sound %s.\n", name);
		return 0;
	}
	
	// Save the name and open the file
	segments[i].name = name;
	segments[i].file = pak.open_file(segments[i].name);
	
	// If file open fails then bail out
	if (segments[i].file == 0) {
		console.printf("Failed to open sound file %s.\n", name);
		segments[i].destroy();
		return 0;
	}

	DMUS_OBJECTDESC desc;
	desc.dwSize = sizeof(DMUS_OBJECTDESC);
	desc.guidClass = CLSID_DirectMusicSegment;
	desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_MEMORY;
	desc.pbMemData = segments[i].file->data();
	desc.llMemLength = segments[i].file->size();

	HRESULT hr = loader->GetObject(
		&desc,
		IID_IDirectMusicSegment8,
		reinterpret_cast<void**>(&segments[i].segment)
	);
	if (FAILED(hr)) {
		segments[i].destroy();
		return 0;
	}
	hr = segments[i].segment->Download(performance);
	if (FAILED(hr)) {
		segments[i].destroy();
		return 0;
	}
	console.printf("sound segment %s successfully loaded\n", segments[i].name.c_str());

	return i;
}

hsound_t
daudio_t::play_segment(hsegment_t segment, int repeats)
	// Play sound plays a sound without any 3D positional information, used
	// for bacground music and any noise that does not need to be 3d spaced
{
	u_assert(segment >= 0 && segment <= *max_sound_segments);
	if (segment == 0 || *nosound)
		return 0;

	HRESULT hr = performance->PlaySegmentEx(
		segments[segment].segment,	// Segment
		NULL,						// Always NULL
		NULL,						// Transitions
		DMUS_SEGF_SECONDARY,		// Flags
		0,							// Start Time
		NULL,						// Pointer to recieve segment state
		NULL,						// Object to stop
		NULL						// Audiopath
	);

	return 1;
}

hsound_t
daudio_t::play_segment_3d(hsegment_t segment, vec3_t pos, vec3_t dir, int repeats)
	// Play sound in a given 3d position and with a given direction of movement
{
	return 0;
}

void
daudio_t::set_pos(haudiopath_t path, vec3_t pos)
{
}

void
daudio_t::set_dir(haudiopath_t path, vec3_t dir)
{
}

void
daudio_t::play_sound(hsegment_t segment, haudiopath_t path, int repeats)
{
}

void
daudio_t::stop_sound(hsegment_t segment, haudiopath_t path)
{
}
