/*
 * Copyright(c) 2020 by Boris Pavlovich Vasilyev <1024sparrow@gmail.com>
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
#include "FakeAudioSystem.hpp"
#include "Configuration.hpp"
#include "WAVE/WAVE.h"

#include <cassert>
#include <cstdlib>
#include <stdio.h>//boris debug
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h> // read

#define debug_profiling printf("--> %s: %i <--\n", __FILE__, __LINE__)

const int CONST_CONFIG_TIMEOUT = 5000;

namespace StretchPlayer
{

FakeAudioSystem::FakeAudioSystem()
	: _left(0)
	, _right(0)
	, _sample_rate(44100)
	, _period_nframes(512)
	, _fdPlayback(0)
	, _fdCapture(0)
	, _active(false)
	, _playbackDebug(true)
{
}

FakeAudioSystem::~FakeAudioSystem()
{
	cleanup();
}

int FakeAudioSystem::init(const char *app_name, Configuration *config, char *err_msg)
{
//	const char *configFilepath = getenv("AUDIO_PIPE_CONFIG"); // boris here: читаем только один раз!!
	const char *playbackFilepath = getenv("AUDIO_PIPE_PLAYBACK");
//	const char *captureFilepath = getenv("AUDIO_PIPE_CAPTURE");
//	if (!configFilepath)
//	{
//		if (err_msg)
//		{
//			strcat(err_msg, "environment variable AUDIO_PIPE_CONFIG not set");
//		}
//		goto init_bail;
//	}
//	if ((_fdConfig = open(configFilepath, /*O_ASYNC |*/ O_NONBLOCK, O_RDONLY)) <= 0)
//	{
//		if (err_msg)
//		{
//			strcat(err_msg, "can not open file '");
//			strcat(err_msg, configFilepath);
//			strcat(err_msg, "' (filepath taken from environment variable AUDIO_PIPE_CONFIG)");
//		}
//		goto init_bail;
//	}
//	if (playbackFilepath){
//		puts("if (playbackFilepath){");
//		puts(playbackFilepath);
//		_fdPlayback = open(playbackFilepath, /*O_ASYNC |*/ O_NONBLOCK, O_WRONLY);
//		//if ((_fdPlayback = open(playbackFilepath, /*O_ASYNC |*/ O_NONBLOCK, O_WRONLY)) <= 0){
//		if (_fdPlayback <= 0){
//			if (err_msg) {
//				strcat(err_msg, "can not open file '");
//				strcat(err_msg, playbackFilepath);
//				strcat(err_msg, "' (filepath taken from environment variable AUDIO_PIPE_PLAYBACK)");
//			}
//			goto init_bail;
//		}
//		printf("_fdPlayback: %i\n", _fdPlayback);
////		struct timeval timeout;
////		fd_set set;
////		int rv;
////		int fdSocket = worker->socket->fdSocket;
////		FD_ZERO(&set);
////		FD_SET(fdSocket, &set);
////		timeout.tv_sec = 0;
////		timeout.tv_usec = 500;
////		rv = select(fdSocket + 1, &set, NULL, NULL, &timeout);
////		// в Linux-е timeout разынициализуется после select-а
////		if (rv < 0) // ошибка чтения. Например, разрыв соединения
////		{
////			//
////		}

//	}
//	if (captureFilepath){
//		if ((_fdCapture = open(captureFilepath, /*O_ASYNC |*/ O_NONBLOCK, O_WRONLY)) <= 0){
//			if (err_msg) {
//				strcat(err_msg, "can not open file '");
//				strcat(err_msg, captureFilepath);
//				strcat(err_msg, "' (filepath taken from environment variable AUDIO_PIPE_PLAYBACK)");
//			}
//			goto init_bail;
//		}
//	}

	_sample_rate = config->sample_rate();
	_period_nframes = config->period_size();
	_left = new float[_period_nframes];
	_right = new float[_period_nframes];
	return 0;

init_bail:
	cleanup();
	return 0xDEADBEEF;
}

void FakeAudioSystem::cleanup()
{
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
)
{
	assert(cbPlayback);
	_cbPlayback = cbPlayback;
	_cbCapture = cbCapture;
	_callback_arg = arg;
	return 0;
}

int FakeAudioSystem::set_segment_size_callback(segment_size_callback_t cb, void* arg, char* err_msg)
{
	return 0;
}

int FakeAudioSystem::activate(char *err_msg)
{
	_active = true;
//	_tConfig = std::thread(&FakeAudioSystem::_runConfig, this);
//	_tConfig.detach();
//	if (_fdPlayback)
	{
		_tPlayback = std::thread(&FakeAudioSystem::_runPlaybackRead, this);
		_tPlayback.detach();
	}
	if (_fdCapture)
	{
		_tCapture = std::thread(&FakeAudioSystem::_runCaptureRead, this);
		_tCapture.detach();
	}
	return 0;
}

int FakeAudioSystem::deactivate(char *err_msg)
{
	_active = false;
//	if (_tConfig.joinable())
//		_tConfig.join();
	if (_tPlayback.joinable())
		_tPlayback.join();
	if (_tCapture.joinable())
		_tCapture.join();
	return 0;
}

AudioSystem::sample_t* FakeAudioSystem::output_buffer(int index)
{
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

AudioSystem::sample_t* FakeAudioSystem::input_buffer()
{
	return 0;
}

uint32_t FakeAudioSystem::output_buffer_size(int index)
{
	return _period_nframes;
}

uint32_t FakeAudioSystem::sample_rate()
{
	return _sample_rate;
}

float FakeAudioSystem::dsp_load()
{
	return 0.0f;
}

uint32_t FakeAudioSystem::time_stamp()
{
	return 0;
}

uint32_t FakeAudioSystem::segment_start_time_stamp()
{
	return 0;
}

uint32_t FakeAudioSystem::current_segment_size()
{
	return _period_nframes;
}

void FakeAudioSystem::_runConfig()
{
//	struct timeval timeout;
//	fd_set set;
//	int rv;
//	int fdSocket = worker->socket->fdSocket;
//	FD_ZERO(&set);
//	FD_SET(fdSocket, &set);
//	timeout.tv_sec = 0;
//	timeout.tv_usec = 500;
//	rv = select(fdSocket + 1, &set, NULL, NULL, &timeout);
//	// в Linux-е timeout разынициализуется после select-а
//	if (rv < 0) // ошибка чтения. Например, разрыв соединения
//	{
//		//
//	}

	const char *configFilepath = getenv("AUDIO_PIPE_CONFIG");
	if (!configFilepath)
	{
//		if (err_msg)
//		{
//			strcat(err_msg, "environment variable AUDIO_PIPE_CONFIG not set");
//		}
//		goto init_bail;
	}
	if ((_fdConfig = open(configFilepath, /*O_ASYNC |*/ O_NONBLOCK, O_RDONLY)) <= 0)
	{
//		if (err_msg)
//		{
//			strcat(err_msg, "can not open file '");
//			strcat(err_msg, configFilepath);
//			strcat(err_msg, "' (filepath taken from environment variable AUDIO_PIPE_CONFIG)");
//		}
//		goto init_bail;
	}

	char buffer[1024];
	while (_active)
	{
		int readCount = read(_fdConfig, buffer, 1024);
		if (readCount < 0)
		{
			//puts("1111 ERROR");
			perror("1111 ERROR");
		}
		if (readCount > 0)
		{
			std::lock_guard<std::mutex> lkPlayback(_mutexPlayback);
			std::lock_guard<std::mutex> lkCapture(_mutexCapture);
			puts("88");
//			printf("read %i bytes\n", readCount);
//			if (_playbackDebug)
//			{
//				if (_fdPlayback)
//				{
////					readCount = write(_fdPlayback, buffer, 2);
//					readCount = write(_fdPlayback, "hello\n", 7);
//					if (readCount < 0)
//					{
//						perror("2222 ERROR");
//					}
//					printf("wrtitten %i bytes\n", readCount);
//				}
//			}
		}
	}
}

void FakeAudioSystem::_runPlaybackRead()
{
	const char *filepathRequest = getenv("AUDIO_PIPE_PLAYBACK_REQUEST");
	if (!filepathRequest)
	{
		debug_profiling;
		return;
	}
	const char *filepath = getenv("AUDIO_PIPE_PLAYBACK");
	if (!filepath)
	{
		debug_profiling;
		return;
	}
	puts("11");
	//if ((_fdPlaybackRequest = open(filepathRequest, /*O_ASYNC |*/ /*O_NONBLOCK*/0, O_RDONLY)) <= 0)
	puts("22");
//	if ((_fdPlayback = open(filepath, O_WRONLY)) <= 0)
//	{
//		debug_profiling;
//		return;
//	}
	puts("33");

	const size_t bufferSize = 1024;
	char buffer[bufferSize];

	FILE *tmpFile = std::tmpfile();
	//FILE *tmpFile = fopen("/home/boris/2.wav", "wb");
	long int byteCounter = 0;

	while (_active)
	{
//		{
//			WaveFile wf;
//			if (!wf.OpenRead(filepathRequest))
//			{
//				perror("can not open playuback request file");
//			}
//			unsigned long sr = wf.GetSampleRate();
//			printf("** sample rate: %lu\n", sr);
//		}
		if ((_fdPlaybackRequest = open(filepathRequest, O_RDONLY)) <= 0)
		{
			debug_profiling;
			return;
		}

		int playbackRequest = 0;
		int readCount = read(_fdPlaybackRequest, buffer, 1024);
		if (readCount < 0)
		{
			perror("_fdPlaybackRequest read error");
		}

		if (readCount > 0)
		{
			if (byteCounter == 0)
			{
				if (readCount >= 40)
				{
					if (!strncmp(buffer, "RIFF", 4) && !strncmp(buffer + 8, "WAVEfmt ", 8))
					{
						byteCounter = buffer[4] + (buffer[5] << 8) + (buffer[6] << 16) + (buffer[7] << 24);
					}
				}
				else
				{
					// читаем количество сэмплов, которые надо отослать
					puts(buffer);
				}
			}
			if (byteCounter > 0)
			{
				byteCounter -= readCount;
				if (byteCounter > 0)
				{
					//tmpFile->
					fwrite(buffer, 1, readCount, tmpFile);
					fflush(tmpFile);
				}
				else
				{
					puts("I got this!");
					byteCounter = 0;
				}
			}
		}
		else if (readCount == 0)
		{
			puts("00000000000000000000000000");
			// закончили читать

			//FILE *fPlayback = fopen(filepath, "wb");
			FILE *fPlaybackTmp = std::tmpfile();
			if (!fPlaybackTmp)
			{
				perror("can not open tmp-file to write playback data");
				continue;
			}

			WaveFile wfPlayback;
			{
				WaveFile wfCapture;
				if (!wfCapture.OpenRead(tmpFile))
				{
					perror("can not open playuback request file");
				}
				unsigned long sr = wfCapture.GetSampleRate();
				printf("** sample rate: %lu\n", sr);
				// boris here: читаем записанный звук в _left и _right.

				// boris here: по клонированного заголовку пишем _left и _right в WAV-файл.
				wfPlayback.CopyFormatFrom(wfCapture);
				if (!wfPlayback.OpenWrite(fPlaybackTmp)) // boris here
				{
					perror("can not open tmp-file to write WAV-header");
					continue;
				}

				//-----------------------------
				wfPlayback.CopyFrom(wfCapture);
				//=============================
			}
			//fclose(tmpFile);
			//tmpFile = std::tmpfile();

			if ((_fdPlayback = open(filepath, O_WRONLY)) <= 0)
			{
				perror("can not open pipe-file to write playback data");
				continue;
			}
			debug_profiling;
			fseek(fPlaybackTmp, 0, SEEK_SET);
			while ((readCount = fread(buffer, 1, bufferSize, fPlaybackTmp) )> 0)
			{
				debug_profiling;
				write(_fdPlayback, buffer, readCount);
			}
			close(_fdPlayback);
		}
		else if (readCount < 0)
		{
			puts("-------"); // epic fail
		}
		close(_fdPlaybackRequest);
	}
	fclose(tmpFile);

	// ==============================================================
//	const char *playbackFilepath = getenv("AUDIO_PIPE_PLAYBACK");
//	if (playbackFilepath){
//		puts("if (playbackFilepath){");
//		puts(playbackFilepath);
//		_fdPlayback = open(playbackFilepath, /*O_ASYNC |*/ O_NONBLOCK, O_WRONLY);
//		//if ((_fdPlayback = open(playbackFilepath, /*O_ASYNC |*/ O_NONBLOCK, O_WRONLY)) <= 0){
//		if (_fdPlayback <= 0){
////			if (err_msg) {
////				strcat(err_msg, "can not open file '");
////				strcat(err_msg, playbackFilepath);
////				strcat(err_msg, "' (filepath taken from environment variable AUDIO_PIPE_PLAYBACK)");
////			}
////			goto init_bail;
//		}
//		printf("_fdPlayback: %i\n", _fdPlayback);
//	}

//	char buffer[1024];
//	while (_active)
//	{
//		int readCount = read(_fdConfig, buffer, 1024);
//		if (readCount < 0)
//		{
//			//puts("1111 ERROR");
//			perror("1111 ERROR");
//		}
//		if (readCount > 0)
//		{
//			printf("read %i bytes\n", readCount);
//			if (_playbackDebug)
//			{
//				if (_fdPlayback)
//				{
////					readCount = write(_fdPlayback, buffer, 2);
//					readCount = write(_fdPlayback, "hello\n", 7);
//					if (readCount < 0)
//					{
//						perror("2222 ERROR");
//					}
//					printf("wrtitten %i bytes\n", readCount);
//				}
//			}
//		}
//	}
}

void FakeAudioSystem::_runCaptureRead()
{
	//
}

void FakeAudioSystem::_runPlaybackWrite()
{
	//
}

void FakeAudioSystem::_runCaptureWrite()
{
	//
}


} // namespace StretchPlayer
