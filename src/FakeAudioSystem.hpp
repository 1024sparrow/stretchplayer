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

/*

Протокол:
Читаем
	- playback_request.fifo:
		* WAV-заголовок (с указанием частоты дискретизации и количества каналов)
		* количество сэмплов, которые надо отослать (запрос)
	- capture.fifo:
		* WAV-заголовок
		* записанные данные
Пишем
	- capture_request.fifo:
		* количество сэмплов, которые жду (запрос).
	- playback.fifo:
		* WAV-заголовок
		* данные на воспроизведение


Читаем Wav-заголовок с данными (capture).
Пишем Wav-заголовок (копия входного) с данными (playback).
*/
#pragma once

#include <AudioSystem.hpp>
#include <thread>
#include <mutex>

namespace StretchPlayer
{
\
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
	sample_t* input_buffer() override;
	uint32_t output_buffer_size(int index) override;
	uint32_t sample_rate() override;
	float dsp_load() override;
	uint32_t time_stamp() override;
	uint32_t segment_start_time_stamp() override;
	uint32_t current_segment_size() override;

private:
	void _runConfig();
	void _runPlaybackRead();
	void _runCaptureRead();
	void _runPlaybackWrite();
	void _runCaptureWrite();

private:
	mutable std::mutex _mutexPlayback, _mutexCapture;
	struct {
		//
	} _requestForPlayback;

	int _fdConfig, _fdPlayback, _fdPlaybackRequest, _fdCapture;
	process_callback_t _cbPlayback, _cbCapture;
	void *_callback_arg;
	float *_left, *_right;
	uint32_t _sample_rate;
	uint32_t _period_nframes;
	bool _active;
	std::thread _tConfig, _tPlayback, _tCapture;
	bool _playbackDebug;
};

} // namespace StretchPlayer
