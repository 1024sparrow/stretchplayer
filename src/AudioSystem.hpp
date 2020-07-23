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
#ifndef AUDIOSYSTEM_HPP
#define AUDIOSYSTEM_HPP

#include <stdint.h>

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
	class AudioSystem
	{
	public:
	typedef float sample_t;
	typedef int (*process_callback_t)(uint32_t nframes, void *arg);
	typedef int (*segment_size_callback_t)(uint32_t nframes, void *arg);

	virtual ~AudioSystem() {}

	/**
	 * 'Construct' the system.
	 *
	 * The Constructor should only initialize bare-bones stuff.
	 * This function should actually do the initialization.  It
	 * does /not/ need to be realtime safe.
	 *
	 * \param app_name if non-zero, identifies this application by
	 * the name given.  If the sound system requires the name be
	 * changed, then app_name will be quietly changed to match.
	 *
	 * \returns 0 on success, nonzero on error.
	 */
	virtual int init(const char *app_name, Configuration *config, char *err_msg = 0) = 0;

	/**
	 * Clean up the system.
	 *
	 * This is the opposite of init().  De-initialization should
	 * be done here rather than the destructor... but the
	 * destructor should be able to call this function, even after
	 * the function was called explicitly.
	 */
	virtual void cleanup() = 0;

	/**
	 * Set the process() callback function.
	 *
	 * The audio system will call this function periodically when
	 * it is time to process data.  This function may only be
	 * called before activate() or after deactivate().
	 *
	 * Pass a null pointer to clear the callback.
	 *
	 * \return 0 on success, otherwise non-zero.
	 */
	virtual int set_process_callback(
		process_callback_t cbPlayback,
		process_callback_t cbCapture,
		void* arg,
		char* err_msg = 0
	) = 0;


	/**
	 * Set the segment size change callback function
	 */
	virtual int set_segment_size_callback(
		segment_size_callback_t cb,
		void* arg,
		char* err_msg = 0
	) = 0;

	/**
	 * Activate the driver (may start processing audio).
	 *
	 * \returns 0 on success, nonzero on error.
	 */
	virtual int activate(char *err_msg = 0) = 0;

	/**
	 * Deactivates the driver (must stop processing audio).
	 *
	 * \returns 0 on success, nonzero on error.
	 */
	virtual int deactivate(char *err_msg = 0) = 0;

	/**
	 * Returns a pointer to the output buffer. [RT SAFE]
	 *
	 * index is 0 for Left, 1 for Right.  Any others will return a
	 * null pointer.
	 *
	 * \returns pointer to buffer, or null pointer if the buffer
	 * does not exist.
	 */
	virtual sample_t* output_buffer(int index) = 0;

	virtual sample_t* input_buffer() = 0;

	/**
	 * Returns the size of the output buffer. [RT SAFE]
	 */
	virtual uint32_t output_buffer_size(int index) = 0;

	virtual uint32_t sample_rate() = 0;

	/**
	 * Returns the current CPU/DSP load as a float [0.0, 1.0]
	 *
	 * \return A float [0.0, 1.0].  -1.0 if the audio system doesn't
	 * support this feature.
	 */
	virtual float dsp_load() = 0;

	/**
	 * Returns a timestamp of the current output, in audio frames.
	 *
	 * \return Approximate frame of current audio output.
	 */
	virtual uint32_t time_stamp() = 0;

	/**
	 * Return a timestamp of the start frame of the current audio segment.
	 *
	 * \return Timestamp for the current callback processing segement.
	 */
	virtual uint32_t segment_start_time_stamp() = 0;

	/**
	 * Return the current size of a segment (nframes)
	 *
	 */
	virtual uint32_t current_segment_size() = 0;
	};

	AudioSystem* audio_system_factory(int driver);

} // namespace StretchPlayer

#endif // AUDIOSYSTEM_HPP
