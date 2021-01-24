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
#include "WAVE/WAVE.h"

#include <cassert>
#include <cstdlib>
#include <stdio.h>//boris debug
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h> // read
#include <fstream>

#define debug_profiling printf("--> %s: %i <--\n", __FILE__, __LINE__)

using namespace std;

namespace little_endian_io
{
  template <typename Word>
  std::ostream& write_word( std::ostream& outs, Word value, unsigned size = sizeof( Word ) )
  {
	for (; size; --size, value >>= 8)
	  outs.put( static_cast <char> (value & 0xFF) );
	return outs;
  }
}
using namespace little_endian_io;



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

int FakeAudioSystem::init(const char *app_name, const Configuration &p_config, char *err_msg)
{
	assert(p_config.mode() == Configuration::Mode::Fake);
	_config = p_config.fake();

	_pipePath = _config.fifoPlayback.c_str();
	_sample_rate = _config.sampleRate;
	_period_nframes = _config.periodSize;

	puts(_pipePath);
//	_fdPlayback = open(_pipePath, /*O_ASYNC |*/ O_NONBLOCK, O_WRONLY);
//	if (_fdPlayback <= 0){
//		if (err_msg) {
//			strcat(err_msg, "can not open file '");
//			strcat(err_msg, _pipePath);
//			strcat(err_msg, "' (filepath taken from environment variable AUDIO_PIPE_PLAYBACK)");
//		}
//		goto init_bail;
//	}
//	printf("_fdPlayback: %i\n", _fdPlayback);





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



//	if (_capturePipePath){
//		if ((_fdCapture = open(_capturePipePath, /*O_ASYNC |*/ O_NONBLOCK, O_WRONLY)) <= 0){
//			if (err_msg) {
//				strcat(err_msg, "can not open file '");
//				strcat(err_msg, _capturePipePath);
//				strcat(err_msg, "' (filepath taken from environment variable AUDIO_PIPE_PLAYBACK)");
//			}
//			goto init_bail;
//		}
//	}

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
	_tPlayback = std::thread(&FakeAudioSystem::_runPlayback, this);
	_tPlayback.detach();
	return 0;
}

int FakeAudioSystem::deactivate(char *err_msg)
{
	_active = false;
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

void FakeAudioSystem::_runPlayback()
{
	const char *err_msg, *str_err;
	while (_active)
	{
		if( _cbPlayback(_config.periodSize, _callback_arg) != 0 ) {
			err_msg = "Application's audio callback failed.";
			str_err = 0;
			puts("error 10109.1953");
		}

		//_fdPlayback.
//		FILE *tmpFile = fopen("tmp", "wb");//std::tmpfile();
//		printf("10109.1917: %f\n", _left[0]);//
//		//FILE *tmpFile = fopen(_pipePath, "wb");
//		//write(_fdPlayback, )
//		{
//			WaveFile wf;
//			if (!wf.OpenWrite(tmpFile))
//			{
//				puts("10109.1654 error");
//			}
//			wf.SetupFormat(_config.sampleRate, _config.bitsPerSample, _config.mono ? 1 : 2);
//			wf.WriteHeaderToFile(tmpFile);
//			for (int i = 0 ; i < _period_nframes ; ++i)
//			{
//				if (!wf.WriteSample(_left[i]))
//				{
//					puts("10110 error");
//				}
//				wf.WriteSample(_right[i]);
//			}
//		}

//		//std::ifstream in("tmp", std::ifstream::ate | std::ifstream::binary);
//		int tmpFileSize = 44 + _config.bitsPerSample * _config.periodSize / 8 * (_config.mono ? 1 : 2);//in.tellg();

//		int fd = open("tmp", O_RDONLY);
//		void *b = malloc(tmpFileSize);
//		read(fd, b, tmpFileSize);
//		close(fd);
//		fd = open(_config.fifoPlayback.c_str(), O_WRONLY);
//		write(fd, b, tmpFileSize);
//		free(b);
//		close(fd);

		//{{
		{
		ofstream f( "example.wav", ios::binary );
		// Write the file headers
		 f << "RIFF----WAVEfmt ";     // (chunk size to be filled in later)
		 write_word( f,     16, 4 );  // no extension data
		 write_word( f,      1, 2 );  // PCM - integer samples
		 write_word( f,      2, 2 );  // two channels (stereo file)
		 write_word( f,  44100, 4 );  // samples per second (Hz)
		 write_word( f, 176400, 4 );  // (Sample Rate * BitsPerSample * Channels) / 8
		 write_word( f,      4, 2 );  // data block size (size of two integer samples, one for each channel, in bytes)
		 write_word( f,     16, 2 );  // number of bits per sample (use a multiple of 8)

		 // Write the data chunk header
		 size_t data_chunk_pos = f.tellp();
		 f << "data----";  // (chunk size to be filled in later)
		 write_word( f,     16, 2 );
		 for (int i = 0 ; i < _period_nframes ; ++i)
		 {
			write_word( f,     static_cast<int>(_left[i] * 32760), 2 );
			write_word( f,     static_cast<int>(_right[i] * 32760), 2 );
		 }

		 // (We'll need the final file size to fix the chunk sizes above)
		 size_t file_length = f.tellp();

		 // Fix the data chunk header to contain the data size
		 f.seekp( data_chunk_pos + 4 );
		 write_word( f, file_length - data_chunk_pos + 8 );

		 // Fix the file header to contain the proper RIFF chunk size, which is (file size - 8) bytes
		 f.seekp( 0 + 4 );
		 write_word( f, file_length - 8, 4 );
		}
		 //}}
		int tmpFileSize = 44 + _config.bitsPerSample * _config.periodSize / 8 * (_config.mono ? 1 : 2);
		int fd = open("example.wav", O_RDONLY);
		void *b = malloc(tmpFileSize);
		read(fd, b, tmpFileSize);
		close(fd);
		fd = open(_config.fifoPlayback.c_str(), O_WRONLY);
		write(fd, b, tmpFileSize);
		free(b);
		close(fd);
	}
}

void FakeAudioSystem::_runCapture()
{
	while (_capturing)
	{
		//
	}
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
