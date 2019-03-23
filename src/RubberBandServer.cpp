/*
 * Copyright(c) 2010 by Gabriel M. Beddingfield <gabriel@teuton.org>
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

#include "RubberBandServer.hpp"
#include <rubberband/RubberBandStretcher.h>
#include <unistd.h>
#include <cassert>
#include <pthread.h>
#include <sys/time.h>

using RubberBand::RubberBandStretcher;

namespace StretchPlayer
{
    RubberBandServer::RubberBandServer(uint32_t sample_rate) :
	_running(true),
	_stretcher_feed_block(512),
	_cpu_load(0.0),
	_time_ratio_param(1.0),
	_pitch_scale_param(1.0),
	_reset_param(false)
    {
    _stretcher = std::move(std::unique_ptr<RubberBand::RubberBandStretcher>(
	    new RubberBandStretcher(
            sample_rate,
			2,
			RubberBandStretcher::OptionProcessRealTime | RubberBandStretcher::OptionThreadingAuto
		)
    ));

	const uint32_t MAXBUF = 8196L; // 2x the highest typical value

	_stretcher->setMaxProcessSize(MAXBUF*4);

	_inputs[0] = std::move(std::unique_ptr<ringbuffer_t>(new ringbuffer_t(MAXBUF*4)));
	_inputs[1] = std::move(std::unique_ptr<ringbuffer_t>(new ringbuffer_t(MAXBUF*4)));
	_outputs[0] = std::move(std::unique_ptr<ringbuffer_t>(new ringbuffer_t(MAXBUF*4)));
	_outputs[1] = std::move(std::unique_ptr<ringbuffer_t>(new ringbuffer_t(MAXBUF*4)));

	_proc_time.insert( _proc_time.end(), 64, 0 );
	_idle_time.insert( _idle_time.end(), 64, 0 );
    }

    RubberBandServer::~RubberBandServer()
    {
    }

    void RubberBandServer::start()
    {
	QThread::start();
    }

    void RubberBandServer::shutdown()
    {
	_running = false;
	_wait_cond.wakeOne();
    }

    bool RubberBandServer::is_running()
    {
	return QThread::isRunning();
    }

    void RubberBandServer::wait()
    {
	QThread::wait();
    }

    void RubberBandServer::reset()
    {
	QMutexLocker lk(&_param_mutex);
	_reset_param = true;
	for(size_t k=0 ; k < _proc_time.size() ; ++k) {
	    _proc_time[k] = 0;
	    _idle_time[k] = 0;
	}
	_wait_cond.wakeOne();
    }

    void RubberBandServer::time_ratio(float val)
    {
	QMutexLocker lk(&_param_mutex);
	_time_ratio_param = val;
    }

    float RubberBandServer::time_ratio()
    {
	return _time_ratio_param;
    }

    void RubberBandServer::pitch_scale(float val)
    {
	QMutexLocker lk(&_param_mutex);
	_pitch_scale_param = val;
    }

    float RubberBandServer::pitch_scale()
    {
	return _pitch_scale_param;
    }

    void RubberBandServer::go_idle()
    {
	setPriority(QThread::IdlePriority);
    }

    void RubberBandServer::go_active()
    {
	setPriority(QThread::TimeCriticalPriority);
    }

    void RubberBandServer::set_segment_size(unsigned long nframes)
    {
	while(_reset_param)
	    usleep(100);

	if(nframes == _stretcher_feed_block)
	    return;

	if( nframes <= 512 ) {
	    _stretcher_feed_block = 512;
	    return;
	}
	// Round up to next power of 2
	if( (nframes - 1) & nframes ) {
	    unsigned long p2 = 1;
	    while( p2 < nframes )
		p2 <<= 1;
	    nframes = p2;
	}
	// Max... see constructor.
	if( nframes > (1L<<14) ) {
	    nframes = (1L<<14);
	}
	_stretcher_feed_block = nframes;	

	reset();
	_stretcher->setMaxProcessSize(nframes * 4);
	_inputs[0].reset( new ringbuffer_t(nframes * 4) );
	_inputs[1].reset( new ringbuffer_t(nframes * 4) );
	_outputs[0].reset( new ringbuffer_t(nframes * 4) );
	_outputs[1].reset( new ringbuffer_t(nframes * 4) );

    }

    uint32_t RubberBandServer::feed_block_min() const
    {
	return _stretcher_feed_block;
    }

    uint32_t RubberBandServer::feed_block_max() const
    {
	return 2 * _stretcher_feed_block;
    }

    uint32_t RubberBandServer::latency() const
    {
	return _stretcher->getLatency();
    }

    uint32_t RubberBandServer::available_write()
    {
	if(_reset_param)
	    return 0;
	uint32_t l, r;
	l = _inputs[0]->write_space();
	r = _inputs[1]->write_space();
	return (l < r) ? l : r;
    }

    uint32_t RubberBandServer::written()
    {
	if(_reset_param)
	    return 0;
	uint32_t l, r;
	l = _inputs[0]->read_space();
	r = _inputs[1]->read_space();
	return (l > r) ? l : r;
    }

    uint32_t RubberBandServer::write_audio(float* left, float* right, uint32_t count)
    {
	if(_reset_param)
	    return 0;
	unsigned l, r, max = available_write();
	if( count > max ) count = max;
	l = _inputs[0]->write(left, count);
	r = _inputs[1]->write(right, count);
	_wait_cond.wakeOne();
	// _have_new_data.wakeAll();
	assert( l == r );
	return l;
    }

    uint32_t RubberBandServer::available_read()
    {
	if(_reset_param)
	    return 0;
	unsigned l, r;
	l = _outputs[0]->read_space();
	r = _outputs[1]->read_space();
	return (l<r) ? l : r;
    }

    uint32_t RubberBandServer::read_audio(float* left, float* right, uint32_t count)
    {
	if(_reset_param)
	    return 0;
	unsigned l, r, max = available_read();
	if( count > max ) count = max;
	l = _outputs[0]->read(left, count);
	r = _outputs[1]->read(right, count);
	_wait_cond.wakeOne();
	// _room_for_output.wakeAll();
	assert( l == r );
	return l;
    }

    void RubberBandServer::nudge()
    {
	_wait_cond.wakeOne();
    }

    float RubberBandServer::cpu_load() const
    {
	return _cpu_load;
    }

    void RubberBandServer::_update_cpu_load()
    {
	assert( _proc_time.size() == _idle_time.size() );
	uint32_t proc = 0, idle = 0;
	float ans;
	size_t k;
	for(k=0 ; k<_proc_time.size() ; ++k ) {
	    proc += _proc_time[k];
	    idle += _idle_time[k];
	}
	if( proc && idle ) {
	    ans = float(proc) / float(proc + idle);
	} else {
	    ans = 0.0;
	}
	_cpu_load = ans;
    }

    void RubberBandServer::run()
    {
	uint32_t read_l, read_r, nget;
	uint32_t write_l, write_r, nput;
	uint32_t tmp;
	const unsigned BUFSIZE = (1L<<15);
	float* bufs[2];
	float left[BUFSIZE], right[BUFSIZE];
	float time_ratio, pitch_scale;
	bool reset;
	bool proc_output;
	int cpu_load_pos = 0;
	timeval a, b, c;

	bufs[0] = left;
	bufs[1] = right;

	QMutexLocker lock(&_param_mutex);
	time_ratio = _time_ratio_param;
	pitch_scale = _pitch_scale_param;
	lock.unlock();

	size_t samples_required;
	int samples_available;
	while(_running) {
	    gettimeofday(&a, 0);

	    // Update stretcher parameters
	    lock.relock();
	    time_ratio = _time_ratio_param;
	    pitch_scale = _pitch_scale_param;
	    reset = _reset_param;
	    if(reset) {
		_stretcher->reset();
		_inputs[0]->reset();
		_inputs[1]->reset();
		_outputs[0]->reset();
		_outputs[1]->reset();
	    }
	    _reset_param = false;
	    lock.unlock();
	    _stretcher->setTimeRatio(time_ratio);
	    _stretcher->setPitchScale(pitch_scale);

	    // Get input audio and put them into the stretcher
	    read_l = _inputs[0]->read_space();
	    read_r = _inputs[1]->read_space();
	    nget = (read_l < read_r) ? read_l : read_r;
	    samples_required = _stretcher->getSamplesRequired();
	    samples_available = _stretcher->available();
	    samples_available += available_read();
	    if(nget) {
		if(nget > feed_block_min())
		    nget = feed_block_min();
		if(nget > samples_required)
		    nget = samples_required;
		if(samples_available > feed_block_max())
		    nget = 0;
		if( samples_available && (samples_available < feed_block_min()) && (samples_required == 0) ) {
		    nget = 0;
		}
	    }
	    if(nget) {
		tmp = _inputs[0]->read(left, nget);
		assert( tmp == nget );
		tmp = _inputs[1]->read(right, nget);
		assert( tmp == nget );
	    }
	    _stretcher->process(bufs, nget, false); // Must call even if nget == 0

	    // Take output audio from stretcher and put on output buffers
	    proc_output = false;
	    nput = 1;
	    while(_stretcher->available() > 0 && nput) {
		write_l = _outputs[0]->write_space();
		write_r = _outputs[1]->write_space();
		nput = (write_l < write_r) ? write_l : write_r;
		if(nput) {
		    proc_output = true;
		    if(nput > feed_block_max()) nput = feed_block_max();
		    tmp = _stretcher->retrieve(bufs, nput);
		    _outputs[0]->write(left, tmp);
		    _outputs[1]->write(right, tmp);
		}
	    }

	    // Update statistics
	    gettimeofday(&b, 0);
	    _proc_time[cpu_load_pos] = (b.tv_sec - a.tv_sec) * 1000000 + b.tv_usec - a.tv_usec;
	    if( (nget == 0) && (! proc_output) && _stretcher->getSamplesRequired()) {
		a = b;
		_wait_cond.wait(&_wait_mutex, 100 /* ms */);
		gettimeofday(&b, 0);
		_idle_time[cpu_load_pos] = (b.tv_sec - a.tv_sec) * 1000000 + b.tv_usec - a.tv_usec;
	    } else {
		_idle_time[cpu_load_pos] = 0;
	    }
	    ++cpu_load_pos;
	    if(cpu_load_pos >= _proc_time.size())
		cpu_load_pos = 0;
	    _update_cpu_load();
	}
    }

} // namespace StretchPlayer
