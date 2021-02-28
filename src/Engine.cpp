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

#include "Engine.hpp"
#include "AudioSystem.hpp"
#include "Configuration.h"
#include <sndfile.h>
#include <sndfile.hh>
#include <mpg123.h>
#include <stdexcept>
#include <cassert>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "config.h"

using RubberBand::RubberBandStretcher;

namespace StretchPlayer
{

const char * const FLOATS_FILE_MARK = "FLOATS  ";

Engine::Engine(const Configuration &config)
	: _config(config)
	, _hit_end(false)
	, _stretch(1.0)
	, _shift(0)
	, _pitch(0)
	, _gain(1.0)
	, _audio_system{ audio_system_factory(_config.mode()) }
{
	char err[1024] = "";

	std::lock_guard<std::mutex> lk(_audio_lock);

	_audio_system->init( "StretchPlayer" , _config, err );
	_audio_system->set_process_callback(
		Engine::static_process_callback,
		Engine::static_process_capture_callback,
		this
	);
	_audio_system->set_segment_size_callback(Engine::static_segment_size_callback, this);

	if (err[0] != '\0')
		throw std::runtime_error(err);


	uint32_t sample_rate = _audio_system->sample_rate();

	_stretcher.setSampleRate(sample_rate);
	_stretcher.set_segment_size( _audio_system->current_segment_size() );
	_stretcher.start();

	if( _audio_system->activate(err) )
		throw std::runtime_error(err);
}

Engine::~Engine()
{
	std::lock_guard<std::mutex> lk(_audio_lock);

	_stretcher.go_idle();
	_stretcher.shutdown();

	_audio_system->deactivate();
	_audio_system->cleanup();

	callback_seq_t::iterator it;
	std::lock_guard<std::mutex> lk_cb(_callback_lock);
	for( it=_error_callbacks.begin() ; it!=_error_callbacks.end() ; ++it ) {
		(*it)->_parent = 0;
	}
	for( it=_message_callbacks.begin() ; it!=_message_callbacks.end() ; ++it ) {
		(*it)->_parent = 0;
	}

	_stretcher.wait();
}

void Engine::_zero_buffers(uint32_t nframes)
{
	// MUTEX MUST ALREADY BE LOCKED

	// Just zero the buffers
	void *buf_L = 0, *buf_R = 0;
	buf_L = _audio_system->output_buffer(0);
	if(buf_L) {
		memset(buf_L, 0, nframes * sizeof(float));
	}
	buf_R = _audio_system->output_buffer(1);
	if(buf_R) {
		memset(buf_R, 0, nframes * sizeof(float));
	}
}

int Engine::segment_size_callback(uint32_t nframes)
{
	std::lock_guard<std::mutex> lk(_audio_lock);
	_stretcher.set_segment_size(nframes);
	return 0;
}

int Engine::process_callback(uint32_t nframes)
{
//	std::lock_guard<std::mutex> lk(_audio_lock);
//	if(_state_changed) {
//		_state_changed = false;
//		_fd->_stretcher.reset();
//		float left[64], right[64];
//		while( _fd->_stretcher.available_read() > 0 ) {
//			_fd->_stretcher.read_audio(left, right, 64);
//		}
//		assert( 0 == _fd->_stretcher.available_read() );
//		_fd->_position = _fd->_output_position;
//	}
//	if(_playing) {
//		if(_fd->_left.size()) {
//			_process_playing(nframes);
//		} else {
//			_playing = false;
//		}
//	} else {
//		_zero_buffers(nframes);
//	}

	bool locked = false;

	try {
		locked = _audio_lock.try_lock();
		if(_state_changed) {
			_state_changed = false;
			_stretcher.reset();
			float left[64], right[64];
			while( _stretcher.available_read() > 0 ) {
				_stretcher.read_audio(left, right, 64);
			}
			assert( 0 == _stretcher.available_read() );
			_fd->_position = _fd->_output_position;
		}
		if(locked) {
			if(_playing) {
				if(_fd->_left.size()) {
					_process_playing(nframes);
				} else {
					_playing = false;
				}
			} else {
				_zero_buffers(nframes);
			}
		} else {
			_zero_buffers(nframes);
		}
	} catch (...) {
	}

	if(locked) _audio_lock.unlock();

	return 0;
}

int Engine::process_callback_capture(uint32_t nframes)
{
	//std::lock_guard<std::mutex> lk(_audio_lock);
	if (_capturing)
	{
		for (int i = 0 ; i < nframes ; ++i)
		{
			_fd->_left2.push_back(_audio_system->input_buffer()[i]);
			_fd->_right2.push_back(_audio_system->input_buffer()[i]);
//			if (_position >= _startRecordPosition && _position < _endRecordPosition)
//				_position = _fd->_left.size();
		}
	}
	return 0;
}

static void apply_gain_to_buffer(float *buf, uint32_t frames, float gain);

void Engine::_process_playing(uint32_t nframes)
{
	//boris here: Далее:
	//  FakeAudioDevice (впоследствии будет переименовано в PipefilesAudioDevice) - запись и чтение в pipe-файлы. Отлаживаться в связке с WebSsh (там надо воспроизводить звук, считанный сервером с указанных при запуске pipe-файлов (при запуске указывается шаблон, по которому для каждого пользователя и каждого IP цели вычисляются имена для pipe-файлов)).

	// MUTEX MUST ALREADY BE LOCKED
	float
		*buf_L = _audio_system->output_buffer(0),
		*buf_R = _audio_system->output_buffer(1)
	;
	if (_capturing) {
		stop();
		_stretcher.reset();
	}

	uint32_t srate = _audio_system->sample_rate();
	float time_ratio = srate / _fd->_sample_rate / _stretch;

	_stretcher.time_ratio( time_ratio );
	_stretcher.pitch_scale( ::pow(2.0, double( _pitch )/12.0) * _fd->_sample_rate / srate );

	uint32_t frame;
	uint32_t reqd, gend, zeros, feed;

	assert( _stretcher.is_running() );

	// Determine how much data to push into the stretcher
	int32_t write_space, written, input_frames;
	write_space = _stretcher.available_write();
	written = _stretcher.written();
	if (written < _stretcher.feed_block_min() && write_space >= _stretcher.feed_block_max() ) {
		input_frames = _stretcher.feed_block_max();
	} else {
		input_frames = 0;
	}

//	if (_capturing) {
//		if (_position > _startRecordPosition && _position < _endRecordPosition) {
//			_position = _startRecordPosition;
//		}
//	}

	// Push data into the stretcher, observing A/B loop points
	int shiftInFrames = _shift * _fd->_sample_rate;
	while( input_frames > 0 ) {
		feed = input_frames;

		std::vector<float>
			&left = _capturing ? (_fd->_left2) : _fd->_left,
			&right = _capturing ? (_fd->_right2) : _fd->_right
		;
		size_t position = _fd->_position;

		if ( position + feed > left.size() ) {
			feed = left.size() - position;
			input_frames = feed;
		}

		if (_shift) {
			float *cand = &_fd->_null[0];
			if (_shift > 0) {
				// actual position at the left channel
				if (left.size() > (position + shiftInFrames))
					cand = &right[position + shiftInFrames];

				_stretcher.write_audio( &left[position], cand, feed );
			}
			else {
				// actual position at the right channel
				if (left.size() > (position - shiftInFrames))
					cand = &left[position - shiftInFrames];
				_stretcher.write_audio( cand, &right[position], feed );
			}
		}
		else {
			_stretcher.write_audio( &left[position], &right[position], feed );
		}
		_fd->_position += feed;
		assert( input_frames >= feed );
		input_frames -= feed;
	}

	// Pull generated data off the stretcher
	uint32_t read_space;
	read_space = _stretcher.available_read();

	if( read_space >= nframes ) {
		_stretcher.read_audio(buf_L, buf_R, nframes);

	} else if ( (read_space > 0) && _hit_end ) {
		_zero_buffers(nframes);
		_stretcher.read_audio(buf_L, buf_R, read_space);
	} else {
		_zero_buffers(nframes);
	}

	// Update our estimation of the output position.
	unsigned n_feed_buf = _stretcher.latency();
	if(_fd->_position > n_feed_buf) {
		_fd->_output_position = _fd->_position - n_feed_buf;
	} else {
		_fd->_output_position = 0;
	}
	assert( (_fd->_output_position > _fd->_position) ? (_fd->_output_position - _fd->_position) <= n_feed_buf : true );
	assert( (_fd->_output_position < _fd->_position) ? (_fd->_position - _fd->_output_position) <= n_feed_buf : true );

	// Apply gain... unroll loop manually so GCC will use SSE
	if(nframes & 0xf) {  // nframes < 16
		unsigned f = nframes;
		while(f--) {
			(*buf_L++) *= _gain;
			(*buf_R++) *= _gain;
		}
	} else {
		apply_gain_to_buffer(buf_L, nframes, _gain);
		apply_gain_to_buffer(buf_R, nframes, _gain);
	}

	if(_fd->_position >= _fd->_left.size()) {
		_hit_end = true;
	}
	if( (_hit_end == true) && (read_space == 0) ) {
		_hit_end = false;
		_playing = false;
		printf("4%f\n", 1000. * get_position());
		_fd->_position = 0;
		_stretcher.reset();
	}

	// Wake up, lazybones!
	_stretcher.nudge();
}

/**
 * Attempt to load a file via libsndfile
 *
 * \return true on success
 */
bool Engine::_load_song_using_libsndfile(const char *p_filename, FileData *p_fileData, std::string &error)
{
	SNDFILE *sf = 0;
	SF_INFO sf_info;
	memset(&sf_info, 0, sizeof(sf_info));

	sf = sf_open(p_filename, SFM_READ, &sf_info);
	if( !sf ) {
		error = std::string{"Error opening file: '"} + p_filename + "': " + sf_strerror(sf);
		return false;
	}

	p_fileData->_sample_rate = sf_info.samplerate;
	p_fileData->_left.reserve( sf_info.frames );
	p_fileData->_right.reserve( sf_info.frames );
	p_fileData->_null.resize( sf_info.frames, 0.f );

	if(sf_info.frames == 0) {
		sf_close(sf);
		error = std::string{"Error opening file: '"} + p_filename + "': File is empty";
		return false;
	}
	p_fileData->_channelCount = sf_info.channels;

	std::vector<float> buf(4096, 0.0f);
	sf_count_t read, k;
	unsigned mod;
	while(true) {
		read = sf_read_float(sf, &buf[0], buf.size());
		if( read < 1 ) break;
		for(k=0 ; k<read ; ++k) {
		mod = k % sf_info.channels;
		if( mod == 0 ) {
			p_fileData->_left.push_back( buf[k] );
			if (sf_info.channels == 1) // mono
				p_fileData->_right.push_back( buf[k] );
		} else if( mod == 1 ) {
			p_fileData->_right.push_back( buf[k] );
		} else {
			// remaining channels ignored
		}
		}
	}

	if( p_fileData->_left.size() != sf_info.frames ) {
		//_error("Warning: not all of the file data was read.");
	}

	sf_close(sf);
	return true;
}

/**
 * Attempt to load an MP3 file via libmpg123
 *
 * adapted by Sean Bolton from mpg123_to_wav.c
 *
 * \return true on success
 */
bool Engine::_load_song_using_libmpg123(const char *filename, FileData *p_fileData, std::string &error)
{
	mpg123_handle *mh = 0;
	int err, channels, encoding;
	long rate;

	if ((err = mpg123_init()) != MPG123_OK ||
		(mh = mpg123_new(0, &err)) == 0 ||
		mpg123_open(mh, filename) != MPG123_OK ||
		mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {

		error = std::string{"Error opening file '"} + filename + "': ";
		if (mh == NULL)
			error += mpg123_plain_strerror(err);
		else
			error += mpg123_strerror(mh);

	  mpg123error:
		mpg123_close(mh);
		mpg123_delete(mh);
		mpg123_exit();
		return false;
	}
	if (encoding != MPG123_ENC_SIGNED_16) {
		error = "unsupported encoding format.";
		goto mpg123error;
	}
	/* lock the output format */
	mpg123_format_none(mh);
	mpg123_format(mh, rate, channels, encoding);
	p_fileData->_channelCount = channels;

	off_t length = mpg123_length(mh);
	if (length == MPG123_ERR || length == 0) {
		error = "file is empty or length unknown.";
		goto mpg123error;
	}

	p_fileData->_sample_rate = rate;
	p_fileData->_left.reserve( length );
	p_fileData->_right.reserve( length );
	p_fileData->_null.reserve( length );

	std::vector<signed short> buffer(4096, 0);
	size_t read = 0, k;

	while (1) {
		err = mpg123_read(mh, (unsigned char*)&buffer[0], buffer.size(), &read);
		if (err != MPG123_OK && err != MPG123_DONE)
			break;
		if (read > 0) {
			read /= sizeof(signed short);
			for(k = 0; k < read ; k++) {
				unsigned int mod = k % channels;
				if( mod == 0 ) {
					p_fileData->_left.push_back( (float)buffer[k] / 32768.0f );
				}
				if( mod == 1 || channels == 1 ) {
					p_fileData->_right.push_back( (float)buffer[k] / 32768.0f );
				}
				/* remaining channels ignored */
			}
		}
		if (err == MPG123_DONE)
		break;
	};

	if (err == MPG123_NEED_MORE) {
		//_error("Warning: premature end of MP3 stream");
		/* allow user to play what we did manage to read */
	} else if (err != MPG123_DONE) {
		error = std::string{"Error decoding file: "} + std::string{MPG123_ERR ? mpg123_strerror(mh) : mpg123_plain_strerror(err)} + ".";
		goto mpg123error;
	}

	mpg123_close(mh);
	mpg123_delete(mh);
	mpg123_exit();
	return true;
}

bool Engine::_load_song_from_floats(const char *filename, FileData *p_fileData, std::string &error)
{
	int file = open(filename, O_RDONLY);
	if (file <= 0)
	{
		error = std::string{"can not open file for reading: "} + strerror(errno);
		return false;
	}
	char testMark[sizeof(FLOATS_FILE_MARK)];
	p_fileData->_channelCount = 1;
	if (read(file, testMark, strlen(FLOATS_FILE_MARK)) != strlen(FLOATS_FILE_MARK))
	{
		error = "it is not a FLOATS file";
		return false;
	}
	if (strncmp(testMark, FLOATS_FILE_MARK, strlen(FLOATS_FILE_MARK)) != 0)
	{
		error = "it is not a FLOATS file";
		printf("boris: <%s> instead of <%s>\n", testMark, FLOATS_FILE_MARK);
		return false;
	}
	if (read(file, &(p_fileData->_sample_rate), sizeof(p_fileData->_sample_rate)) != sizeof(p_fileData->_sample_rate))
	{
		error = std::string{"can not read sample rate: "} + strerror(errno);
		return false;
	}
	float sampleCand ;
	while (read(file, &sampleCand, sizeof(sampleCand)) == sizeof(sampleCand))
	{
		p_fileData->_left.push_back(sampleCand);
		p_fileData->_right.push_back(sampleCand);
		p_fileData->_null.push_back(0);
	}

	close(file);
	return true;
}

/**
 * Load a file
 *
 * \return Name of song
 */
bool Engine::load_song(const char *filename, bool prelimanarily, std::string &error)
{
	FileData *fd { _fd };
	if (prelimanarily) {
		fd = _fileDatas + (_fileDataIndex + 1) % 2;
	}
	else {
		stop();
		if (_capturing)
			stop_recording(false);
	}
	fd->_changed = false;

	fd->_left.clear();
	fd->_right.clear();
	fd->_position = 0;
	fd->_output_position = 0;
	if (!prelimanarily) {
		_stretcher.reset();
	}

	if (!_load_song_using_libsndfile(filename, fd, error))
	{
		if (!_load_song_using_libmpg123(filename, fd, error))
		{
			if (!_load_song_from_floats(filename, fd, error))
			{
				return false;
			}
		}
	}
	if (fd->_channelCount > 1 && _config.mono()) {
		float average = 0; // for mono option enabled and more then one channels
		for (size_t i = 0, c = fd->_left.size() ; i < c ; ++i) {
			average = (fd->_left[i] + fd->_right[i]) / 2.f;
			fd->_left[i] = average;
			fd->_right[i] = average;
		}
	}
	return true;
}

bool Engine::decodeMediaFile(const char *encodedInFile, const char *rawOutFile, std::string &p_error)
{
	/*
	 * Структура файла типа FLOATS:
	 * текстовая метка "FLOATS  " (8 байт, с двумя пробелами в конце)
	 * float sampleRate
	 * Далее идут по очереди float-ы с отдельными сэмлами. Чтобы выяснить длительность записи, необходимо вычитать все сэмплы, ну или посмотреть размер файла.
	 *
	 * Все файлы FLOATS представляют только моно-записи.
	 */
	FileData fd;
	std::string error;
	if (!_load_song_using_libsndfile(encodedInFile, &fd, error))
	{
		if (!_load_song_using_libmpg123(encodedInFile, &fd, error))
		{
			if (!_load_song_from_floats(encodedInFile, &fd, error))
			{
				p_error = "can not open file: " + error;
				return false;
			}
		}
	}
	// we use only LEFT channel
	if (fd._channelCount > 1) { //  && _config.mono()) {
		for (size_t i = 0, c = fd._left.size() ; i < c ; ++i) {
			fd._left[i] = (fd._left[i] + fd._right[i]) / 2.f;
		}
	}

	int file = open(rawOutFile, O_CREAT | O_WRONLY | O_TRUNC);
	if (file <= 0)
	{
		p_error = std::string{"can not open file for writing: "} + strerror(errno);
		return false;
	}

	if (write(file, FLOATS_FILE_MARK, strlen(FLOATS_FILE_MARK)) != strlen(FLOATS_FILE_MARK))
	{
		p_error = "can not write to file...";
		return false;
	}
	if (write(file, &fd._sample_rate, sizeof(fd._sample_rate)) != sizeof(fd._sample_rate))
	{
		p_error = "can not write to file...";
		return false;
	}

	int counter = 0, totalMegabyteCount = fd._left.size() * sizeof(float) / (1024*1024) + 1;
	for (auto oSample : fd._left)
	{
		if (write(file, &oSample, sizeof(oSample)) != sizeof(oSample))
		{
			p_error = "can not write to file...";
			return false;
		}
		if (!(counter % (1024*1024 / sizeof(oSample))))
		{
			printf(
				"writing %i-th megabyte of %i\n",
				static_cast<int>((counter + 1)* sizeof(float) / (1024*1024) + 1),
				totalMegabyteCount
			);
		}
		++counter;
	}
	close(file);

	return true;
}

void Engine::applyPreloaded()
{
	std::lock_guard<std::mutex> lk(_audio_lock);

	_stretcher.reset();
	stop();
	if (_capturing)
		stop_recording(false);
	_fileDataIndex = (_fileDataIndex + 1) % 2;
	_fd = _fileDatas + _fileDataIndex;
}

void Engine::play()
{
	if( ! _playing ) {
		_state_changed = true;
		_playing = true;
	}
}

void Engine::play_pause()
{
	_playing = (_playing) ? false : true;
	_state_changed = true;
}

void Engine::stop()
{
	if( _playing ) {
		_playing = false;
		_state_changed = true;
	}
}

bool Engine::save(const char *p_filepath)
{
	std::lock_guard<std::mutex> lk(_audio_lock);

	SF_INFO sfinfo {
		// sf_count_t	frames ;		/* Used to be called samples.  Changed to avoid confusion. */
		0,
		// int			samplerate ;
		_config.sampleRate(),
		// int			channels ;
		1, // _config->mono() ? 1 : _fd->_channelCount,
		// int			format ;
		SF_FORMAT_WAV | SF_FORMAT_FLOAT,
		// int			sections ;
		0,
		// int			seekable ;
		0
	};
	SNDFILE *outfile;
	if (outfile = sf_open(p_filepath, SFM_WRITE, &sfinfo))
	{
		sf_count_t count = sf_write_float(outfile, &_fd->_left[0], _fd->_left.size());
		sf_write_sync(outfile);
		sf_close(outfile);
		if (!count)
		{
			puts("0there's no data written");
			return false;
		}
		puts("e");
		return true;
	}
	else
	{
		printf("0can't open file %s\n", p_filepath);
	}

	return false;
}

float Engine::get_position()
{
	if(_fd->_left.size() > 0) {
		return float(_fd->_output_position) / _fd->_sample_rate;
	}
	return 0;
}

float Engine::get_length()
{
	if(_fd->_left.size() > 0) {
		return float(_fd->_left.size()) / _fd->_sample_rate;
	}
	return 0;
}

void Engine::locate(double secs)
{
	std::lock_guard<std::mutex> lk(_audio_lock);
	unsigned long pos = secs * _fd->_sample_rate;
	_fd->_output_position = _fd->_position = pos;
	_state_changed = true;
	_stretcher.reset();
}

void Engine::start_recording(const unsigned long &startPos) {
	if (!_config.mic())
	{
		puts("0recording unavailable");
		return;
	}

	if (_capturing)
	{
		puts("0already recording");
		return;
	}
	else
	{
		std::lock_guard<std::mutex> lk(_audio_lock);
		_fd->_startRecordPosition = startPos * _fd->_sample_rate / 1000;
		_fd->_endRecordPosition = _fd->_left.size();

		_fd->_left2 = std::vector<float>(_fd->_left.begin(), _fd->_left.begin() + _fd->_startRecordPosition);
		_fd->_right2 = std::vector<float>(_fd->_right.begin(), _fd->_right.begin() + _fd->_startRecordPosition);
		_fd->_left3 = std::vector<float>(_fd->_left.begin() + _fd->_endRecordPosition, _fd->_left.end());
		_fd->_right3 = std::vector<float>(_fd->_right.begin() + _fd->_endRecordPosition, _fd->_right.end());
		_capturing = true;
	}
}

void Engine::start_recording(
	const unsigned long &startPos,
	const unsigned long &stopPos
) {
	if (!_config.mic())
	{
		puts("0recording unavailable");
		return;
	}

	if (_capturing)
	{
		puts("0already recording");
		return;
	}
	else
	{
		std::lock_guard<std::mutex> lk(_audio_lock);
		_fd->_startRecordPosition = startPos * _fd->_sample_rate / 1000;
		_fd->_endRecordPosition = stopPos * _fd->_sample_rate / 1000;
		_fd->_left2 = std::vector<float>(_fd->_left.begin(), _fd->_left.begin() + _fd->_startRecordPosition);
		_fd->_right2 = std::vector<float>(_fd->_right.begin(), _fd->_right.begin() + _fd->_startRecordPosition);
		_fd->_left3 = std::vector<float>(_fd->_left.begin() + _fd->_endRecordPosition, _fd->_left.end());
		_fd->_right3 = std::vector<float>(_fd->_right.begin() + _fd->_endRecordPosition, _fd->_right.end());
		_fd->_left3 = std::vector<float>(_fd->_left.begin() + _fd->_endRecordPosition, _fd->_left.end());
		_fd->_right3 = std::vector<float>(_fd->_right.begin() + _fd->_endRecordPosition, _fd->_right.end());
		_capturing = true;
	}
}

void Engine::stop_recording(bool p_reflectChangesInFile) {
	if (!_config.mic())
	{
		puts("0recording unavailable");
		return;
	}

	std::lock_guard<std::mutex> lk(_audio_lock);
	if (p_reflectChangesInFile) {
		_fd->_left = _fd->_left2;
		_fd->_right = _fd->_right2;
		_fd->_left.insert(_fd->_left.end(), _fd->_left3.begin(), _fd->_left3.end());
		_fd->_right.insert(_fd->_right.end(), _fd->_right3.begin(), _fd->_right3.end());
		_fd->_changed = true;
	}
	_capturing = false;
}

void Engine::_dispatch_message(const Engine::callback_seq_t& seq, const char *msg) const
{
	std::lock_guard<std::mutex> lk(_callback_lock);
	Engine::callback_seq_t::const_iterator it;
	for( it=seq.begin() ; it!=seq.end() ; ++it ) {
		(**it)(msg);
	}
}

void Engine::_subscribe_list(Engine::callback_seq_t& seq, EngineMessageCallback* obj)
{
	if( obj == 0 ) return;
	std::lock_guard<std::mutex> lk(_callback_lock);
	obj->_parent = this;
	seq.insert(obj);
}

void Engine::_unsubscribe_list(Engine::callback_seq_t& seq, EngineMessageCallback* obj)
{
	if( obj == 0 ) return;
	std::lock_guard<std::mutex> lk(_callback_lock);
	obj->_parent = 0;
	seq.erase(obj);
}

float Engine::get_cpu_load()
{
	float audio_load, worker_load;

	audio_load = _audio_system->dsp_load();
	if(_playing) {
		worker_load = _stretcher.cpu_load();
	} else {
		worker_load = 0.0;
	}
	return  audio_load + worker_load;
}

/* SIMD code for optimizing the gain application.
 *
 * Below is vectorized (SSE, SIMD) code for applying
 * the gain.  If you enable >= SSE2 optimization, then
 * this will calculate 4 floats at a time.  If you do
 * not, then it will still work.
 *
 * This syntax is a GCC extension, but more portable
 * than writing x86 assembly.
 */

typedef float __vf4 __attribute__((vector_size(16)));
typedef union {
	float f[4];
	__vf4 v;
} vf4;

/**
 * \brief Multiply each element in a buffer by a scalar.
 *
 * For each element in buf[0..nframes-1], buf[i] *= gain.
 *
 * This function detects the buffer alignment, and if it's 4-byte
 * aligned and SSE optimization is enabled, it will use the
 * optimized code-path.  However, it will still work with 1-byte
 * aligned buffers.
 *
 * \param buf - Pointer to a buffer of floats.
 */
static void apply_gain_to_buffer(float *buf, uint32_t nframes, float gain)
{
	vf4* opt;
	vf4 gg = {gain, gain, gain, gain};
	int alignment;
	unsigned ctr = nframes/4;

	alignment = reinterpret_cast<uintptr_t>(buf) & 0x0F;

	switch(alignment) {
	case 4: (*buf++) *= gain;
	case 8: (*buf++) *= gain;
	case 12:(*buf++) *= gain;
		--ctr;
	case 0:
		break;
	default:
	  goto LAME;
	}

	assert( (reinterpret_cast<uintptr_t>(buf) & 0x0F) == 0 );
	opt = (vf4*) buf;
	while(ctr--) {
		opt->v *= gg.v; ++opt;
	}

	buf = (float*) opt;
	switch(alignment) {
	case 12: (*buf++) *= gain;
	case 8:  (*buf++) *= gain;
	case 4:  (*buf++) *= gain;
	}

	return;

	LAME:
	// If it's not even 4-byte aligned
	// then this is is the un-optimized code.
	while(nframes--) {
	  (*buf++) *= gain;
	}
}

} // namespace StretchPlayer
