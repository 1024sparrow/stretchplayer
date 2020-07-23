/*
 * Copyright(c) 2009 by Gabriel M. Beddingfield <gabriel@teuton.org>
 * Copyright(c) 2002 by Paul Davis
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

/**
 * Much of this code was adapted from "A Tutorial on Using the ALSA
 * Audio API" by Paul Davis.
 * http://www.equalarea.com/paul/alsa-audio.html
 */

#include "AlsaAudioSystem.hpp"
#include "Configuration.hpp"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio> // For snprintf
#include <string.h>
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <cmath>

#include "bams_format.h"
#include <endian.h>

#include <iostream>
using namespace std;

/* Formats supported by this class, in order or preference
 */
static const snd_pcm_format_t aas_supported_formats[] = {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	SND_PCM_FORMAT_FLOAT_LE,
	SND_PCM_FORMAT_S16_LE,
	SND_PCM_FORMAT_FLOAT_BE,
	SND_PCM_FORMAT_S16_BE,
	SND_PCM_FORMAT_U16_LE,
	SND_PCM_FORMAT_U16_BE,
#elif __BYTE_ORDER == __BIG_ENDIAN
	SND_PCM_FORMAT_FLOAT_BE,
	SND_PCM_FORMAT_S16_BE,
	SND_PCM_FORMAT_FLOAT_LE,
	SND_PCM_FORMAT_S16_LE,
	SND_PCM_FORMAT_U16_BE,
	SND_PCM_FORMAT_U16_LE,
#else
#error Unsupport byte order.
#endif
	SND_PCM_FORMAT_UNKNOWN
};

namespace StretchPlayer
{

inline bool not_aligned_16(void* ptr) {
return (reinterpret_cast<uintptr_t>(ptr) & 0x0F);
}

AlsaAudioSystem::AlsaAudioSystem() :
	_channels(2),
	_type(FLOAT),
	_bits(32),
	/*	_type(INT),
	_bits(16), */
	_little_endian(true),
	_sample_rate(44100),
	_period_nframes(512),
	_active(false),
	_playback_handle(0),
	_record_handle(0),
	_left_root(0),
	_right_root(0),
	_left(0),
	_right(0),
	_cbPlayback(0),
	_cbCapture(0),
	_callback_arg(0),
	_dsp_load_pos(0),
	_dsp_load(0.0f)
{
	memset(&_dsp_a, 0, sizeof(timeval));
	memset(&_dsp_b, 0, sizeof(timeval));
	memset(_dsp_idle_time, 0, sizeof(_dsp_idle_time));
	memset(_dsp_work_time, 0, sizeof(_dsp_work_time));
}

AlsaAudioSystem::~AlsaAudioSystem()
{
	cleanup();
}

int AlsaAudioSystem::init(const char * /*app_name*/, Configuration *config, char *err_msg)
{
	unsigned nfrags;
	int err;
	snd_pcm_format_t format = SND_PCM_FORMAT_UNKNOWN;
	int k;
	snd_pcm_hw_params_t *hw_params_playback = nullptr;
	snd_pcm_sw_params_t *sw_params_playback = nullptr;
	snd_pcm_hw_params_t *hw_params_record = nullptr;
	snd_pcm_sw_params_t *sw_params_record = nullptr;

	_sample_rate = config->sample_rate();
	_period_nframes = config->period_size();
	nfrags = config->periods_per_buffer();

	if( config == 0 ) {
		if (err_msg){
			strcat(err_msg, "The AlsaAudioSystem::init() function must have a non-null config parameter.");// boris e: replace "strcat" for "strncat(..., 1024)"
		}
		goto init_bail;
	}

	int nfds;
	struct pollfd *pfds;

	if((err = snd_pcm_open(&_playback_handle, config->audio_device(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot open default ALSA audio device for playback (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}
	assert(_playback_handle);

	if((err = snd_pcm_hw_params_malloc(&hw_params_playback)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot allocate hardware parameter structure for sound playback (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}

	if((err = snd_pcm_hw_params_any(_playback_handle, hw_params_playback)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot initialize hardware parameter structure for sound playback (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}

	if((err = snd_pcm_hw_params_set_access(_playback_handle, hw_params_playback,
						   SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot set access type (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}

	if (config->sound_recording()) {
		if((err = snd_pcm_open(&_record_handle, config->audio_device(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
			if (err_msg){
				strcat(err_msg, "cannot open default ALSA audio device for audio-capturing (");
				strcat(err_msg, snd_strerror(err));
				strcat(err_msg, ")");
			}
			goto init_bail;
		}
		assert(_record_handle);
		if((err = snd_pcm_hw_params_malloc(&hw_params_record)) < 0) {
			if (err_msg){
				strcat(err_msg, "cannot allocate hardware parameter structure for sound recording (");
				strcat(err_msg, snd_strerror(err));
				strcat(err_msg, ")");
			}
			goto init_bail;
		}
		if((err = snd_pcm_hw_params_any(_record_handle, hw_params_record)) < 0) {
			if (err_msg){
				strcat(err_msg, "cannot initialize hardware parameter structure for sound recording (");
				strcat(err_msg, snd_strerror(err));
				strcat(err_msg, ")");
			}
			goto init_bail;
		}
		if((err = snd_pcm_hw_params_set_access(_record_handle, hw_params_record,
							   SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			if (err_msg){
				strcat(err_msg, "cannot set access type (");
				strcat(err_msg, snd_strerror(err));
				strcat(err_msg, ")");
			}
			goto init_bail;
		}
	}

	// set to _playback_handle and to _record_handle the same format
	for(k = 0 ; aas_supported_formats[k] != SND_PCM_FORMAT_UNKNOWN ; ++k) {
		format = aas_supported_formats[k];
		if(snd_pcm_hw_params_test_format(_playback_handle, hw_params_playback, format)) {
			if (config->sound_recording()) {
				if(snd_pcm_hw_params_test_format(_record_handle, hw_params_record, format)) {
					format = SND_PCM_FORMAT_UNKNOWN;
				}
			} else {
				format = SND_PCM_FORMAT_UNKNOWN;
			}
		} else {
			break;
		}
	}

	switch(format) {
	case SND_PCM_FORMAT_FLOAT_LE:
		_type = FLOAT;
		_bits = 32;
		_little_endian = true;
		break;
	case SND_PCM_FORMAT_S16_LE:
		_type = INT;
		_bits = 16;
		_little_endian = true;
		break;
	case SND_PCM_FORMAT_FLOAT_BE:
		_type = FLOAT;
		_bits = 32;
		_little_endian = false;
		break;
	case SND_PCM_FORMAT_S16_BE:
		_type = INT;
		_bits = 16;
		_little_endian = false;
		break;
	case SND_PCM_FORMAT_U16_LE:
		_type = UINT;
		_bits = 32;
		_little_endian = true;
		break;
	case SND_PCM_FORMAT_U16_BE:
		_type = UINT;
		_bits = 32;
		_little_endian = false;
		break;
	case SND_PCM_FORMAT_UNKNOWN:
		if (err_msg){
			strcat(err_msg, "The audio card does not support any PCM audio formats that StretchPlayer supports");
		}
		goto init_bail;
		break;
	default:
		assert(false);
	}

	if((err = snd_pcm_hw_params_set_format(_playback_handle, hw_params_playback, format)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot set sample format (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}
	if (config->sound_recording()) {
		if((err = snd_pcm_hw_params_set_format(_record_handle, hw_params_record, format)) < 0) {
			if (err_msg){
				strcat(err_msg, "cannot set sample format for sound recording (");
				strcat(err_msg, snd_strerror(err));
				strcat(err_msg, ")");
			}
			goto init_bail;
		}
	}

	if((err = snd_pcm_hw_params_set_rate(_playback_handle, hw_params_playback, _sample_rate, 0)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot set sample rate (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}
	if (config->sound_recording()) {
		if((err = snd_pcm_hw_params_set_rate(_record_handle, hw_params_record, _sample_rate, 0)) < 0) {
			if (err_msg){
				strcat(err_msg, "cannot set sample rate for sound recording (");
				strcat(err_msg, snd_strerror(err));
				strcat(err_msg, ")");
			}
			goto init_bail;
		}
	}

	if((err = snd_pcm_hw_params_set_channels(_playback_handle, hw_params_playback, 2)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot set channel count (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}
	if (config->sound_recording()) {
		if((err = snd_pcm_hw_params_set_channels(_record_handle, hw_params_record, 2)) < 0) {
			if (err_msg){
				strcat(err_msg, "cannot set channel count for sound recording (");
				strcat(err_msg, snd_strerror(err));
				strcat(err_msg, ")");
			}
			goto init_bail;
		}
	}

	if((err = snd_pcm_hw_params_set_periods_near(_playback_handle, hw_params_playback, &nfrags, 0)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot set the period count (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}

	if ((err = snd_pcm_hw_params_set_buffer_size(_playback_handle, hw_params_playback, _period_nframes * nfrags)) < 0){
		if (err_msg){
			char tmp[512];
			sprintf(tmp, "cannot set the buffer size to %i x %i (", nfrags, _period_nframes);
			strcat(err_msg, tmp);
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}

	if((err = snd_pcm_hw_params(_playback_handle, hw_params_playback)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot set parameters (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}

	if (config->sound_recording()) {
		if((err = snd_pcm_hw_params_set_periods_near(_record_handle, hw_params_record, &nfrags, 0)) < 0) {
			if (err_msg){
				strcat(err_msg, "cannot set the period count for sound recording (");
				strcat(err_msg, snd_strerror(err));
				strcat(err_msg, ")");
			}
			goto init_bail;
		}

		if ((err = snd_pcm_hw_params_set_buffer_size(_record_handle, hw_params_record, _period_nframes * nfrags)) < 0){
			if (err_msg){
				char tmp[512];
				sprintf(tmp, "cannot set the buffer size to %i x %i for sound recording (", nfrags, _period_nframes);
				strcat(err_msg, tmp);
				strcat(err_msg, snd_strerror(err));
				strcat(err_msg, ")");
			}
			goto init_bail;
		}

		if((err = snd_pcm_hw_params(_record_handle, hw_params_record)) < 0) {
			if (err_msg){
				strcat(err_msg, "cannot set parameters for sound recording (");
				strcat(err_msg, snd_strerror(err));
				strcat(err_msg, ")");
			}
			goto init_bail;
		}
	}

	snd_pcm_hw_params_free(hw_params_playback);
	snd_pcm_hw_params_free(hw_params_record);

	/* Tell ALSA to wake us up whenever _period_nframes or more frames
	 * of playback data can be delivered.  Also, tell ALSA
	 * that we'll start the device ourselves.
	 */

	if((err = snd_pcm_sw_params_malloc(&sw_params_playback)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot allocate software parameters structure (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}

	if((err = snd_pcm_sw_params_current(_playback_handle, sw_params_playback)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot initialize software parameters structure (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}

	if((err = snd_pcm_sw_params_set_avail_min(_playback_handle, sw_params_playback, _period_nframes)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot set minimum available count (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}

	if((err = snd_pcm_sw_params_set_start_threshold(_playback_handle, sw_params_playback, 0U)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot set start mode (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}
	if((err = snd_pcm_sw_params(_playback_handle, sw_params_playback)) < 0) {
		if (err_msg){
			strcat(err_msg, "cannot set software parameters (");
			strcat(err_msg, snd_strerror(err));
			strcat(err_msg, ")");
		}
		goto init_bail;
	}
	if (config->sound_recording()) {
		if((err = snd_pcm_sw_params(_record_handle, sw_params_playback)) < 0) {
			if (err_msg){
				strcat(err_msg, "cannot set software parameters (");
				strcat(err_msg, snd_strerror(err));
				strcat(err_msg, ")");
			}
			goto init_bail;
		}
	}

	size_t data_size;

	switch(_bits) {
	case 8:  data_size = 1; break;
	case 16: data_size = 2; break;
	case 24: data_size = 4; break;
	case 32: data_size = 4; break;
	default: assert(false);
	}

	_buf = _buf_root = new unsigned short[_period_nframes * _channels * data_size + 16];
	_left = _left_root = new float[_period_nframes + 4];
	_right = _right_root = new float[_period_nframes + 4];

	assert(_buf);
	assert(_left);
	assert(_right);

	while( not_aligned_16(_buf) ) ++_buf;
	while( not_aligned_16(_left) ) ++_left;
	while( not_aligned_16(_right) ) ++_right;

	return 0;

init_bail:
	cleanup();
	return 0xDEADBEEF;
}

void AlsaAudioSystem::cleanup()
{
	deactivate();
	if(_right_root) {
		delete [] _right_root;
		_right = _right_root = 0;
	}
	if(_left_root) {
		delete [] _left_root;
		_left = _left_root = 0;
	}
	if(_buf_root) {
		delete [] _buf_root;
		_buf = _buf_root = 0;
	}
	if(_playback_handle) {
		snd_pcm_close(_playback_handle);
		_playback_handle = 0;
	}
}

int AlsaAudioSystem::set_process_callback(
	process_callback_t cbPlayback,
	process_callback_t cbCapture,
	void* arg,
	char* err_msg
)
{
	assert(cbPlayback);
	_cbPlayback = cbPlayback;
	_cbCapture = cbCapture;
	_callback_arg = arg;
	return 0;
}

int AlsaAudioSystem::set_segment_size_callback(process_callback_t, void*, char*)
{
	// This API never changes the segment size automatically
	return 0;
}

int AlsaAudioSystem::activate(char *err_msg)
{
	// boris here: if in init() was detected sound_recording option, then we have to start TWO threads: for playback and for capture.
	assert(!_active);
	assert(_left);
	assert(_right);
	assert(_playback_handle);

	_active = true;
	_tPlayback = std::thread(&AlsaAudioSystem::_run, this);
	_tPlayback.detach();
	if (_record_handle) {
		_tCapture = std::thread(&AlsaAudioSystem::_runCapture, this);
		_tCapture.detach();
	}

	return 0;
}

int AlsaAudioSystem::deactivate(char *err_msg)
{
	_active = false;
	if (_tCapture.joinable())
		_tCapture.join();
	if (_tPlayback.joinable())
		_tPlayback.join();
	return 0;
}

AudioSystem::sample_t* AlsaAudioSystem::output_buffer(int index)
{
	if(index == 0) {
		assert(_left);
		return _left;
	}else if(index == 1) {
		assert(_right);
		return _right;
	}
	return 0;
}

uint32_t AlsaAudioSystem::output_buffer_size(int /*index*/)
{
	return _period_nframes;
}

uint32_t AlsaAudioSystem::sample_rate()
{
	return _sample_rate;
}

float AlsaAudioSystem::dsp_load()
{
	return _dsp_load;
}

uint32_t AlsaAudioSystem::time_stamp()
{
	return 0;
}

uint32_t AlsaAudioSystem::segment_start_time_stamp()
{
	return 0;
}

uint32_t AlsaAudioSystem::current_segment_size()
{
	return _period_nframes;
}

static inline unsigned long calc_elapsed(const timeval& a, const timeval& b)
{
	unsigned long ans;
	if(b.tv_sec < a.tv_sec)
		return 0;
	ans = (b.tv_sec - a.tv_sec) * 100000 + b.tv_usec;
	if(ans >= a.tv_usec)
		ans -= a.tv_usec;
	return ans;
}

void AlsaAudioSystem::_stopwatch_init()
{
	gettimeofday(&_dsp_a, 0);
}

void AlsaAudioSystem::_stopwatch_start_idle()
{
	gettimeofday(&_dsp_b, 0);
	_dsp_work_time[_dsp_load_pos] = calc_elapsed(_dsp_a, _dsp_b);
	_dsp_load_update();
	_dsp_a = _dsp_b;
	++_dsp_load_pos;
	if(_dsp_load_pos > DSP_AVG_SIZE)
		_dsp_load_pos = 0;
}

void AlsaAudioSystem::_stopwatch_start_work()
{
	gettimeofday(&_dsp_b, 0);
	_dsp_idle_time[_dsp_load_pos] = calc_elapsed(_dsp_a, _dsp_b);
	_dsp_a = _dsp_b;
}

void AlsaAudioSystem::_dsp_load_update()
{
	int k = DSP_AVG_SIZE;
	unsigned long work = 0, idle = 0, tot;
	while(k--) {
		idle += _dsp_idle_time[k];
		work += _dsp_work_time[k];
	}
	tot = work + idle;
	if(tot)
		_dsp_load = float(work) / float(tot);
	else
		_dsp_load = 0.0f;
	assert(_dsp_load <= 1.0f);
	assert(_dsp_load >= 0.0f);
	assert( !isnan(_dsp_load) );
}

void AlsaAudioSystem::_run()
{
	int err;
	snd_pcm_sframes_t frames_to_deliver;
	uint32_t f;
	const char *err_msg, *str_err;
	const int misc_msg_size = 256;
	char misc_msg[misc_msg_size] = "";

	assert(_active);

	// Set RT priority
	sched_param thread_sched_param;
	thread_sched_param.sched_priority = 80;
	pthread_setschedparam( pthread_self(), SCHED_FIFO, &thread_sched_param );

	err = 0;

	/* the interface will interrupt the kernel every
	 * _period_nframes frames, and ALSA will wake up this program
	 * very soon after that.
	 */
	if((err = snd_pcm_prepare(_playback_handle)) < 0) {
		err_msg = "Cannot prepare audio interface for use [snd_pcm_prepare()].";
		str_err = snd_strerror(err);
		goto run_bail;
	}

	_stopwatch_init();
	while(_active) {
		assert(_cbPlayback);

		_stopwatch_start_idle();
		if((err = snd_pcm_wait(_playback_handle, 1000)) < 0) {
			err_msg = "Audio poll failed [snd_pcm_wait()].";
			str_err = strerror(errno);
			goto run_bail;
		}

		_stopwatch_start_work();
		if((frames_to_deliver = snd_pcm_avail_update(_playback_handle)) < 0) {
			if(frames_to_deliver == -EPIPE) {
				/* An XRUN Occurred.  Ignoring. */
			} else {
				err_msg = "Unknown ALSA snd_pcm_avail_update return value [snd_pcm_avail_update()].";
				snprintf(misc_msg, misc_msg_size, "%ld", frames_to_deliver);
				str_err = misc_msg;
				goto run_bail;
			}
		}

		if(frames_to_deliver < _period_nframes)
			continue;

		frames_to_deliver = frames_to_deliver > _period_nframes ? _period_nframes : frames_to_deliver;
		//printf("*********************** frames to deliver: %li **\n", frames_to_deliver); // 1024

		assert( 0 == ((frames_to_deliver-1)&frames_to_deliver) );  // is power of 2.

		if( _cbPlayback(frames_to_deliver, _callback_arg) != 0 ) {
			err_msg = "Application's audio callback failed.";
			str_err = 0;
			goto run_bail;
		}

		_convert_to_output(frames_to_deliver);

		/*if ((err = snd_pcm_drain(_playback_handle)) < 0)
		{
			err_msg = "1234";
			str_err = snd_strerror(err);
			goto run_bail;
		}*/

		if((err = snd_pcm_writei(_playback_handle, _buf, frames_to_deliver)) < 0) {
			err_msg = "Write to audio card failed [snd_pcm_writei()].";
			str_err = snd_strerror(err);
			goto run_bail;
		}

	}
	return;

	run_bail:

	_active = false; // boris e: остановить также и второй поток...
	thread_sched_param.sched_priority = 0;
	pthread_setschedparam( pthread_self(), SCHED_OTHER, &thread_sched_param );

	cerr << "ERROR: " << err_msg;
	if(str_err)
		cerr << " (" << str_err << ")";
	cerr << endl;
	cerr << "Aborting audio driver." << endl;

	return;
}

void AlsaAudioSystem::_runCapture()
{
	printf("############### capture starting ############\n");
	int err;
	uint32_t f;
	const char *err_msg, *str_err;
	const int misc_msg_size = 256;
	char misc_msg[misc_msg_size] = "";

	assert(_active);

	// Set RT priority
	sched_param thread_sched_param;
	thread_sched_param.sched_priority = 80;
	pthread_setschedparam( pthread_self(), SCHED_FIFO, &thread_sched_param );

	err = 0;

	/* the interface will interrupt the kernel every
	 * _period_nframes frames, and ALSA will wake up this program
	 * very soon after that.
	 */
	if((err = snd_pcm_prepare(_record_handle)) < 0) {
		err_msg = "Cannot prepare audio interface for use [snd_pcm_prepare()] in case of sound recording.";
		str_err = snd_strerror(err);
		goto run_bail;
	}

	while(_active) {
		assert(_cbCapture);
		if((err = snd_pcm_wait(_record_handle, 1000)) < 0) {
			err_msg = "Audio poll failed [snd_pcm_wait()].";
			str_err = strerror(errno);
			goto run_bail;
		}

		//_convert_to_output(frames_to_deliver);

		if((err = snd_pcm_readi(_record_handle, _buf, _period_nframes)) < 0) {
			err_msg = "Write to audio card failed [snd_pcm_writei()].";
			str_err = snd_strerror(err);
			goto run_bail;
		}
		printf("############## read data length: %i %i ##\n", err, _buf[0]);
	}
	return;

	run_bail:

	_active = false; // boris e: остановить также и второй поток...
	thread_sched_param.sched_priority = 0;
	pthread_setschedparam( pthread_self(), SCHED_OTHER, &thread_sched_param );

	cerr << "ERROR: " << err_msg;
	if(str_err)
		cerr << " (" << str_err << ")";
	cerr << endl;
	cerr << "Aborting audio driver (capture thread)." << endl;

	return;
}

/**
 * \brief Convert, copy, and interleave _left and _right to _buf;
 */
void AlsaAudioSystem::_convert_to_output(uint32_t nframes)
{
	switch(_type) {
	case INT: _convert_to_output_int(nframes); break;
	case UINT: _convert_to_output_uint(nframes); break;
	case FLOAT: _convert_to_output_float(nframes); break;
	default: assert(false);
	}
}

void AlsaAudioSystem::_convert_to_output_int(uint32_t nframes)
{
	switch(_bits) {
	case 16: {
		bams_sample_s16le_t *dst = (bams_sample_s16le_t*)_buf;
	#if __BYTE_ORDER == __LITTLE_ENDIAN
		if(_little_endian) {
		bams_copy_s16le_floatle(dst, 2, &_left[0], 1, nframes);
		bams_copy_s16le_floatle(dst+1, 2, &_right[0], 1, nframes);
		} else {
		bams_copy_s16be_floatle(dst, 2, &_left[0], 1, nframes);
		bams_copy_s16be_floatle(dst+1, 2, &_right[0], 1, nframes);
		}
	#else
		if(_little_endian) {
		bams_copy_s16le_floatbe(dst, 2, &_left[0], 1, nframes);
		bams_copy_s16le_floatbe(dst+1, 2, &_right[0], 1, nframes);
		} else {
		bams_copy_s16be_floatbe(dst, 2, &_left[0], 1, nframes);
		bams_copy_s16be_floatbe(dst+1, 2, &_right[0], 1, nframes);
		}
	#endif
	}   break;
	case 8:
	case 24:
	case 32:
	default:
		assert(false);
	}
}

void AlsaAudioSystem::_convert_to_output_uint(uint32_t nframes)
{
	switch(_bits) {
	case 16: {
		bams_sample_u16le_t *dst = (bams_sample_u16le_t*)_buf;
	#if __BYTE_ORDER == __LITTLE_ENDIAN
		if(_little_endian) {
		bams_copy_u16le_floatle(dst, 2, &_left[0], 1, nframes);
		bams_copy_u16le_floatle(dst+1, 2, &_right[0], 1, nframes);
		} else {
		bams_copy_u16be_floatle(dst, 2, &_left[0], 1, nframes);
		bams_copy_u16be_floatle(dst+1, 2, &_right[0], 1, nframes);
		}
	#else
		if(_little_endian) {
		bams_copy_u16le_floatbe(dst, 2, &_left[0], 1, nframes);
		bams_copy_u16le_floatbe(dst+1, 2, &_right[0], 1, nframes);
		} else {
		bams_copy_u16be_floatbe(dst, 2, &_left[0], 1, nframes);
		bams_copy_u16be_floatbe(dst+1, 2, &_right[0], 1, nframes);
		}
	#endif
	}   break;
	case 8:
	case 24:
	case 32:
	default:
		assert(false);
	}
}

void AlsaAudioSystem::_convert_to_output_float(uint32_t nframes)
{
	float *out, *l, *r;
	uint32_t f, count;
	assert(_bits == 32);
	assert(_little_endian);
	out = (float*)_buf;
	l = &_left[0];
	r = &_right[0];
	count = nframes;
	while(count--) {
		(*out++) = (*l++);
		(*out++) = (*r++);
	}
	/* Check for non-native byte ordering */
	#if __BYTE_ORDER == __LITTLE_ENDIAN
	if(!_little_endian) {
		bams_byte_reorder_in_place(_buf, 4, 1, 2*nframes);
	}
	#else
	if(_little_endian) {
		bams_byte_reorder_in_place(_buf, 4, 1, 2*nframes);
	}
	#endif
}

} // namespace StretchPlayer
