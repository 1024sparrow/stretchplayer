#pragma once

#include <AudioSystem.hpp>

namespace StretchPlayer
{
	class FakeAudioSystem : public AudioSystem
	{
	public:
	FakeAudioSystem();
	~FakeAudioSystem() override;
	int init(const char *app_name, Configuration *config, char *err_msg = 0) override;
	void cleanup() override;
	int set_process_callback(
		process_callback_t cbPlayback,
		process_callback_t cbCapture,
		void* arg,
		char* err_msg = 0
	) override;
	int set_segment_size_callback(
		segment_size_callback_t cb,
		void* arg,
		char* err_msg = 0
	) override;
	int activate(char *err_msg = 0) override;
	int deactivate(char *err_msg = 0) override;
	sample_t* output_buffer(int index) override;
	uint32_t output_buffer_size(int index) override;
	uint32_t sample_rate() override;
	float dsp_load() override;
	uint32_t time_stamp() override;
	uint32_t segment_start_time_stamp() override;
	uint32_t current_segment_size() override;

	private:
	process_callback_t _cbPlayback, _cbCapture;
	void *_callback_arg;
	float *_left, *_right;
	uint32_t _sample_rate;
	uint32_t _period_nframes;
	};

} // namespace StretchPlayer
