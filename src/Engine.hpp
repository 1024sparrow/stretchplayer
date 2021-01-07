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
#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <stdint.h>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <set>
#include "RubberBandServer.hpp"


namespace StretchPlayer
{

class Configuration2;
class EngineMessageCallback;
class AudioSystem;
class RubberBandServer;

class Engine
{
public:
	Engine(const Configuration2 &config = 0);
	~Engine();

	bool load_song(const char *filename, bool prelimanarily);
	void applyPreloaded();
	void play();
	void play_pause();
	void stop();
	bool playing() {
		std::lock_guard<std::mutex> lk(_audio_lock);
		return _playing;
	}
	bool changed() const {
		std::lock_guard<std::mutex> lk(_audio_lock);
		return _fd->_changed;
	}
	bool save(const char *p_filepath);

	float get_position(); // in seconds
	float get_length();   // in seconds
	void locate(double secs);
	float get_stretch() {
		return _stretch;
	}
	void set_stretch(float str) {
		if(str > 0.2499 && str < 1.2501) {  /* would be 'if(str >= 0.25 && str <= 1.25)', but floating point is tricky... */
			_stretch = str;
			//_state_changed = true;
		}
	}
	int get_shift() {
		return _shift;
	}
	void set_shift(int p_shift) {
		_shift = p_shift;
	}
	int get_pitch() {
		return _pitch;
	}
	void set_pitch(int pit) {
		if(pit < -12) {
			_pitch = -12;
		} else if (pit > 12) {
			_pitch = 12;
		} else {
			_pitch = pit;
		}
		//_state_changed = true;
	}

	/**
	 * Clipped to [0.0, 10.0]
	 */
	void set_volume(float gain) {
		if (gain < 0.0)
			gain = 0.0;
		if(gain > 10.0)
			gain = 10.0;
		_gain=gain;
	}

	float get_volume() {
		return _gain;
	}

	void start_recording(const unsigned long &startPos);
	void start_recording(
		const unsigned long &startPos,
		const unsigned long &stopPos
	);
	void stop_recording(bool p_reflectChangesInFile);

	/**
	 * Returns estimate of CPU load [0.0, 1.0]
	 */
	float get_cpu_load();

	void subscribe_errors(EngineMessageCallback* obj) {
		_subscribe_list(_error_callbacks, obj);
	}
	void unsubscribe_errors(EngineMessageCallback* obj) {
		_unsubscribe_list(_error_callbacks, obj);
	}
	void subscribe_messages(EngineMessageCallback* obj) {
		_subscribe_list(_message_callbacks, obj);
	}
	void unsubscribe_messages(EngineMessageCallback* obj) {
		_unsubscribe_list(_message_callbacks, obj);
	}

private:
	struct FileData
	{
		std::vector<float> // input data: candidate to push into stretcher
			_left, _right,
			_left2, _right2, // before captured with captured appended
			_left3, _right3, // not modified part (tail)
			_null
		;
		int _channelCount{0}; // 1 for mono, 2 for stereo
		size_t
			_position{0},
			_startRecordPosition{0},
			_endRecordPosition{0},
			_output_position{0} // Latency tracking
		;
		float _sample_rate{ 48000.0 };
		bool _changed{false};
	};

	static int static_process_callback(uint32_t nframes, void* arg) {
		Engine *e = static_cast<Engine*>(arg);
		return e->process_callback(nframes);
	}
	static int static_process_capture_callback(uint32_t nframes, void* arg) {
		Engine *e = static_cast<Engine*>(arg);
		return e->process_callback_capture(nframes);
	}
	static int static_segment_size_callback(uint32_t nframes, void* arg) {
		Engine *e = static_cast<Engine*>(arg);
		return e->segment_size_callback(nframes);
	}

	int process_callback(uint32_t nframes);
	int process_callback_capture(uint32_t nframes);
	int segment_size_callback(uint32_t nframes);

	void _zero_buffers(uint32_t nframes);
	void _process_playing(uint32_t nframes);
	bool _load_song_using_libsndfile(const char *p_filename, FileData *p_fileData);
	bool _load_song_using_libmpg123(const char *filename, FileData *p_fileData);

	typedef std::set<EngineMessageCallback*> callback_seq_t;

	void _error(const char *msg) const {
	_dispatch_message(_error_callbacks, msg);
	}
	void _message(const char *msg) const {
	_dispatch_message(_message_callbacks, msg);
	}
	void _dispatch_message(const callback_seq_t& seq, const char *msg) const;
	void _subscribe_list(callback_seq_t& seq, EngineMessageCallback* obj);
	void _unsubscribe_list(callback_seq_t& seq, EngineMessageCallback* obj);

	Configuration2 *_config;
	bool _playing{false}, _capturing{false};
	bool _hit_end; // boris e
	bool _state_changed; // boris here: move to FileData
	mutable std::mutex _audio_lock;
	RubberBandServer _stretcher;

	FileData _fileDatas[2];
	int _fileDataIndex{0};
	FileData *_fd{_fileDatas};

	float _stretch;
	int _shift;
	int _pitch;
	float _gain;
	std::unique_ptr<AudioSystem> _audio_system;

	mutable std::mutex _callback_lock;
	callback_seq_t _error_callbacks;
	callback_seq_t _message_callbacks;

}; // Engine

class EngineMessageCallback
{
public:
	virtual ~EngineMessageCallback() {
	if(_parent) {
		_parent->unsubscribe_errors(this);
	}
	if(_parent) {
		_parent->unsubscribe_messages(this);
	}
	}

	virtual void operator()(const char *message) = 0;

private:
	friend class Engine;
	Engine *_parent;
};

} // namespace StretchPlayer

#endif // ENGINE_HPP
