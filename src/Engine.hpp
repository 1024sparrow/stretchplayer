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

class Configuration;
class EngineMessageCallback;
class AudioSystem;
class RubberBandServer;

class Engine
{
public:
	Engine(Configuration *config = 0);
	~Engine();

	bool load_song(const char *filename);
	void play();
	void play_pause();
	void stop();
	bool playing() {
	return _playing;
	}
	void loop_ab();
	bool looping() {
	return _loop_b > _loop_a;
	}

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
	if(gain < 0.0) gain = 0.0;
	if(gain > 10.0) gain = 10.0;
	_gain=gain;
	}

	float get_volume() {
	return _gain;
	}

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
	static int static_process_callback(uint32_t nframes, void* arg) {
	Engine *e = static_cast<Engine*>(arg);
	return e->process_callback(nframes);
	}
	static int static_segment_size_callback(uint32_t nframes, void* arg) {
	Engine *e = static_cast<Engine*>(arg);
	return e->segment_size_callback(nframes);
	}

	int process_callback(uint32_t nframes);
	int segment_size_callback(uint32_t nframes);

	void _zero_buffers(uint32_t nframes);
	void _process_playing(uint32_t nframes);
	bool _load_song_using_libsndfile(const char *filename);
	bool _load_song_using_libmpg123(const char *filename);
	void _handle_loop_ab();

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

	Configuration *_config;
	bool _playing;
	bool _hit_end;
	bool _state_changed;
	mutable std::mutex _audio_lock;
	std::vector<float> _left; // input data: candidate to push into stretcher
	std::vector<float> _right; // input data: candidate to push into stretcher
	unsigned long _position;
	unsigned long _loop_a;
	unsigned long _loop_b;
	std::atomic<int> _loop_ab_pressed;
	float _sample_rate;
	float _stretch;
	int p_shift;
	int _pitch;
	float _gain;
	//std::unique_ptr<RubberBandServer> _stretcher;
	RubberBandServer _stretcher;
	std::unique_ptr<AudioSystem> _audio_system;

	/* Latency tracking */
	unsigned long _output_position;

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
