#include "FakeAudioSystem.hpp"

#include <cassert>
#include <cstdlib>
#include <stdio.h>//boris debug
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

namespace StretchPlayer
{

	FakeAudioSystem::FakeAudioSystem()
		: _left(0)
		, _right(0)
		, _sample_rate(44100)
		, _period_nframes(512)
		, _fdPlayback(0)
		, _fdCapture(0)
	{
	}

	FakeAudioSystem::~FakeAudioSystem()
	{
		cleanup();
	}

	int FakeAudioSystem::init(const char *app_name, Configuration *config, char *err_msg){
		const char *configFilepath = getenv("AUDIO_PIPE_CONFIG"); // boris here: читаем только один раз!!
		const char *playbackFilepath = getenv("AUDIO_PIPE_PLAYBACK");
		const char *captureFilepath = getenv("AUDIO_PIPE_CAPTURE");
		//puts(playbackFilepath);
		//printf("++ %i, %s\n", playbackFilepath, playbackFilepath);
		if (!configFilepath) {
			if (err_msg) {
				strcat(err_msg, "environment variable AUDIO_PIPE_CONFIG not set");
			}
			goto init_bail;
		}
		if ((_fdConfig = open(configFilepath, O_ASYNC | O_NONBLOCK, O_RDONLY)) <= 0){
			if (err_msg) {
				strcat(err_msg, "can not open file '");
				strcat(err_msg, configFilepath);
				strcat(err_msg, "' (filepath taken from environment variable AUDIO_PIPE_CONFIG)");
			}
			goto init_bail;
		}
		if (playbackFilepath){
			if ((_fdPlayback = open(playbackFilepath, O_ASYNC | O_NONBLOCK, O_WRONLY)) <= 0){
				if (err_msg) {
					strcat(err_msg, "can not open file '");
					strcat(err_msg, playbackFilepath);
					strcat(err_msg, "' (filepath taken from environment variable AUDIO_PIPE_PLAYBACK)");
				}
				goto init_bail;
			}
//			struct timeval timeout;
//			fd_set set;
//			int rv;
//			int fdSocket = worker->socket->fdSocket;
//			FD_ZERO(&set);
//			FD_SET(fdSocket, &set);
//			timeout.tv_sec = 0;
//			timeout.tv_usec = 500;
//			rv = select(fdSocket + 1, &set, NULL, NULL, &timeout);
//			// в Linux-е timeout разынициализуется после select-а
//			if (rv < 0) // ошибка чтения. Например, разрыв соединения
//			{
//				//
//			}

		}
		if (captureFilepath){
			if ((_fdCapture = open(captureFilepath, O_ASYNC | O_NONBLOCK, O_WRONLY)) <= 0){
				if (err_msg) {
					strcat(err_msg, "can not open file '");
					strcat(err_msg, captureFilepath);
					strcat(err_msg, "' (filepath taken from environment variable AUDIO_PIPE_PLAYBACK)");
				}
				goto init_bail;
			}
		}

		_left = new float[_period_nframes];
		_right = new float[_period_nframes];
		return 0;

	init_bail:
		cleanup();
		return 0xDEADBEEF;
	}

	void FakeAudioSystem::cleanup(){
		deactivate();
		delete [] _left;
		delete [] _right;
		_left = 0;
		_right = 0;
	}

	int FakeAudioSystem::set_process_callback(
		process_callback_t cbPlayback,
		process_callback_t cbCapture,
		void* arg,
		char* err_msg
	){
		assert(cbPlayback);
		_cbPlayback = cbPlayback;
		_cbCapture = cbCapture;
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

	AudioSystem::sample_t* FakeAudioSystem::input_buffer(){
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
