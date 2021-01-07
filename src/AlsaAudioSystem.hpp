/*
 * Copyright(c) 2009 by Gabriel M. Beddingfield <gabriel@teuton.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef ALSAAUDIOSYSTEM_HPP
#define ALSAAUDIOSYSTEM_HPP

#include <AudioSystem.hpp>
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <thread>

namespace StretchPlayer
{
	class Configuration;

	/**
	 * \brief ALSA audio driver implementation.
	 *
	 */
	class AlsaAudioSystem : public AudioSystem
	{
	public:
	AlsaAudioSystem();
	virtual ~AlsaAudioSystem();

	/* Implementing all of AudioSystem's interface:
	 */
	int init(const char *app_name, const Configuration2 &config, char *err_msg = 0) override;
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
	sample_t* input_buffer() override;
	uint32_t output_buffer_size(int index) override;
	uint32_t sample_rate() override;
	float dsp_load() override;
	uint32_t time_stamp() override;
	uint32_t segment_start_time_stamp() override;
	uint32_t current_segment_size() override;

	private:
	void _run();
	void _runCapture();
	void _convert_to_output(uint32_t nframes);
	void _convert_to_output_int(uint32_t nframes);
	void _convert_to_output_uint(uint32_t nframes);
	void _convert_to_output_float(uint32_t nframes);
	void _convert_from_input(uint32_t nframes);
	void _convert_from_input_int(uint32_t nframes);
	void _convert_from_input_uint(uint32_t nframes);
	void _convert_from_input_float(uint32_t nframes);

	void _stopwatch_init();
	void _stopwatch_start_idle();
	void _stopwatch_start_work();
	void _dsp_load_update();

	private:
	// Configuration variables:
	unsigned _channels;
	enum { INT, UINT, FLOAT } _type;
	unsigned _bits;
	bool _little_endian;
	uint32_t _sample_rate;
	uint32_t _period_nframes;

	// ALSA handles
	bool _active;
	bool _capturing;
	snd_pcm_t *_playback_handle;
	snd_pcm_t *_record_handle;
	float *_left_root, *_right_root, *_capturedBuffer_root;
	float *_left, *_right, *_capturedBuffer;
	unsigned short *_buf_root, *_buf, *_buf_capture_root, *_buf_capture;

	process_callback_t _cbPlayback, _cbCapture;
	void *_callback_arg;

	// DSP Load estimation (playback only)
	enum { DSP_AVG_SIZE = 32 };
	int _dsp_load_pos;
	timeval _dsp_a, _dsp_b;
	unsigned long _dsp_idle_time[DSP_AVG_SIZE];
	unsigned long _dsp_work_time[DSP_AVG_SIZE];
	float _dsp_load;

	Configuration2::Alsa _config;

	std::thread _tPlayback, _tCapture;

	};

} // namespace StretchPlayer

#endif // AUDIOSYSTEM_HPP
