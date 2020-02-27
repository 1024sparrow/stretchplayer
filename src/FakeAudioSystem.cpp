#include "FakeAudioSystem.hpp"

#include <cassert>

namespace StretchPlayer
{

	FakeAudioSystem::FakeAudioSystem():
	_left(0),
	_right(0),
	_sample_rate(44100),
	_period_nframes(512)
	{
	}

	FakeAudioSystem::~FakeAudioSystem()
	{
		cleanup();
	}

	int FakeAudioSystem::init(const char *app_name, Configuration *config, char *err_msg){
		_left = new float[_period_nframes];
		_right = new float[_period_nframes];
		return 0;
	}

	void FakeAudioSystem::cleanup(){
		deactivate();
		delete [] _left;
		delete [] _right;
		_left = 0;
		_right = 0;
	}

	int FakeAudioSystem::set_process_callback(process_callback_t cb, void* arg, char* err_msg){
		assert(cb);
		_callback = cb;
		_callback_arg = arg;
		return 0;
	}

	int FakeAudioSystem::set_segment_size_callback(segment_size_callback_t cb, void* arg, char* err_msg){
		return 0;
	}

	int FakeAudioSystem::activate(char *err_msg){
		return 0;
	}

	int FakeAudioSystem::deactivate(char *err_msg){
		return 0;
	}

	AudioSystem::sample_t* FakeAudioSystem::output_buffer(int index){
		if (index == 0){
			assert(_left);
			return _left;
		}
		else if (index == 1){
			assert(_right);
			return _right;
		}
		return 0;
	}

	uint32_t FakeAudioSystem::output_buffer_size(int index){
		return _period_nframes;
	}

	uint32_t FakeAudioSystem::sample_rate(){
		return _sample_rate;
	}

	float FakeAudioSystem::dsp_load(){
		return 0.0f;
	}

	uint32_t FakeAudioSystem::time_stamp(){
		return 0;
	}

	uint32_t FakeAudioSystem::segment_start_time_stamp(){
		return 0;
	}

	uint32_t FakeAudioSystem::current_segment_size(){
		return _period_nframes;
	}


} // namespace StretchPlayer
