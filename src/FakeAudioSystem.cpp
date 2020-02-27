#include "FakeAudioSystem.hpp"

namespace StretchPlayer
{

	FakeAudioSystem::FakeAudioSystem()
	{
	}

	FakeAudioSystem::~FakeAudioSystem()
	{
	}

	int FakeAudioSystem::init(const char *app_name, Configuration *config, char *err_msg = 0){
		return 0;
	}

	void cleanup(){
	}

	int FakeAudioSystem::set_process_callback(process_callback_t cb, void* arg, char* err_msg = 0){
		assert(cb);
		_callback = cb;
		_callback_arg = arg;
		return 0;
	}

	int FakeAudioSystem::set_segment_size_callback(segment_size_callback_t cb, void* arg, char* err_msg = 0){
		return 0;
	}

	int FakeAudioSystem::activate(char *err_msg = 0){
		// boris here
	}

	int FakeAudioSystem::deactivate(char *err_msg = 0){
		// boris here
	}

	sample_t* FakeAudioSystem::output_buffer(int index){
		if (index == 0){
			return _left;
		}
		else if (index == 1){
			return _right;
		}
		return 0;
	}

	uint32_t FakeAudioSystem::output_buffer_size(int index){
	}

	uint32_t FakeAudioSystem::sample_rate(){
	}

	float FakeAudioSystem::dsp_load(){
	}

	uint32_t FakeAudioSystem::time_stamp(){
	}

	uint32_t FakeAudioSystem::segment_start_time_stamp(){
	}

	uint32_t FakeAudioSystem::current_segment_size(){
	}


} // namespace StretchPlayer
