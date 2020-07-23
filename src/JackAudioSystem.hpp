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
#ifndef JACKAUDIOSYSTEM_HPP
#define JACKAUDIOSYSTEM_HPP

#include <AudioSystem.hpp>
#include <jack/jack.h>

namespace StretchPlayer
{
	class Configuration;

	/**
	 * \brief Pure virtual interface to an audio driver API.
	 *
	 * This AudioSystem assumes a very simple system with two audio
	 * outputs.  It maintains those ports and buffers and the
	 * connection of them.
	 */
	class JackAudioSystem : public AudioSystem
	{
	public:
	JackAudioSystem();
	virtual ~JackAudioSystem();

	/* Implementing all of AudioSystem's interface:
	 */
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
	jack_client_t *_client;
	jack_port_t* _port[2];
	Configuration* _config;
	};

} // namespace StretchPlayer

#endif // AUDIOSYSTEM_HPP
