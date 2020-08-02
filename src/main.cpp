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

#include "config.h"

#include <stdio.h> // for printf, fgets
#include <stdlib.h> // atoll
#include <string.h>
#include <unistd.h> // read

#include "Configuration.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>

#include "Engine.hpp"

int main(int argc, char* argv[])
{
	StretchPlayer::Configuration config(argc, argv);

	if(config.help() || ( !config.ok() )) {
	config.usage();
	if( !config.ok() ) return -1;
	return 0;
	}

	if( !config.quiet() ) {
	config.copyright();
	}

	std::unique_ptr<StretchPlayer::EngineMessageCallback> _engine_callback;
	StretchPlayer::Engine *_engine = new StretchPlayer::Engine(&config);

	_engine->set_shift(config.shift());
	_engine->set_stretch((float)config.stretch()/100.f);
	_engine->set_pitch(config.pitch());
	if (config.startup_file()) {
		if (!_engine->load_song(config.startup_file(), false)) {
			printf("0can't open\n");
			fflush(stdout);
			return 1;
		}
	}

	if (!config.quiet()) {
		printf("enter a command (enter \"h\" for help).\n");
		fflush(stdout);
	}
	char c;
	ssize_t dataLen;
	char str[1024];
	char *paramString;
	while (true)
	{
		dataLen = read(0, &str, 1024);
		if (dataLen <= 0)
		{
			printf("0file IO error\n");
			fflush(stdout);
			break;
		}
		if (dataLen == 1)
			continue; // no command
		c = str[0];
		paramString = str + 1;
		str[dataLen - 1] = '\0';
		if (c == 'q')
			break;
		else if (c == 'h')
		{
			// TODO: d - use audio filter(for playback only). Parameters: channels (1 - left, 2 - right, 0 - both), path to a filtering program, arguments for the program. It can be useful, for example, for voice replacement.
			printf(R"(
##################################
# Commands from user:
#   q - quit
#   h - show help for console control commands
#   1 - open audio file. After "1" (without "enter") input file path.
#   2 - start playing. Parameter: millisecond of starting
#   3 - start playing. Parameters: millisecond of starting and millisecond of stoping
#   4 - stop playing. Returns stopping millisecond
#   5 - request current playing position. Returns current playing position
#   6 - set playing speed (in percents)
#   7 - set frequency shift (number from -12 to 12)
#   8 - set volume (in percents)
#   9 - set right channel position ahead of left. Parameter: shift (in seconds)
#   b - start recording. Parameters: start position, end position(if not pointed, then to end of file). Recorded fragment will be inserted instead of pointed interval. Limit: 30 minuts (max latency if still not stoped).
#   c - stop recording. Parameter: if apply recorded fragment (1 - apply, 0 - undo changes)
#   d - request if data has not reflected in file changes. Response: "d0" (not changed) or "d1" (changed)
#   e - save to file (WAV). Parameter: filepath to save. Saved mono-file with data from left channel only. It is usable only with --mono key.
#   f - preload file. Use case: you preload next sound chunk to switch to that in future. Parameter: filepath.
#   g - quick apply preloaded file.
#
# Messages for user:
#   0 - error message (text)
#   1 - opened successfully (without arguments)
#   4 - stopping position
#   5 - current playing position (in milliseconds)
#   6 - playing speed. Appears as response for commands 2, 3, and 6.
#   7 - frequency shift (number from -12 to 12). Appears as response for commands 2, 3 and 7.
#   d - if data has not reflected in file changes. Response: "d0" (not changed) or "d1" (changed)
#   e - saved successfully
#
# And also:
#   Как писать в случае замедленного воспроизведения - писать тоже замедленно. Т.е. при нормальной скорости воспроизведения такой записи, она будет ускоренной.
##################################
)");
			fflush(stdout);
		}
		else if (c == '1')
		{
			_engine->load_song(paramString, false);
		}
		else if (c == '2')
		{
			long long ll = atoll(paramString);
			double d = ll/1000.;
			//printf("%f\n", d);
			_engine->locate(d);
			_engine->play();
		}
		else if (c == '3')
		{
			long long ll1, ll2;
			const char *s = strtok(paramString, " ");
			if (s)
				ll1 = atoll(s);
			else
			{
				printf("0error");
				fflush(stdout);
				continue;
			}
			s = strtok(NULL, " ");
			if (s)
				ll2 = atoll(s);
			else
			{
				printf("0error");
				fflush(stdout);
				continue;
			}
			//printf("%lli - %lli\n", ll1, ll2);
			double d1 = ll1/1000.;
			_engine->locate(d1);
			_engine->play();
		}
		else if (c == '4')
		{
			_engine->stop();
		}
		else if (c == '5')
		{
			float position = 1000. * _engine->get_position();
			printf("5%f\n", position);
			fflush(stdout);
		}
		else if (c == '6')
		{
			short i = atoi(paramString);
			float d = i/100.;
			_engine->set_stretch(d);
		}
		else if (c == '7')
		{
			short i = atoi(paramString);
			_engine->set_pitch(i);
		}
		else if (c == '8')
		{
			short i = atoi(paramString);
			float d = i / 100.;
			_engine->set_volume(d);
		}
		else if (c == '9')
		{
			short i = atoi(paramString);
			_engine->set_shift(i);
		}
		else if (c == 'b')
		{
			char *tmpContext, *tmpString;
			if (tmpString = strtok_r(paramString, "	 ", &tmpContext))
			{
				unsigned long recordStartPos = strtoul(tmpString, 0, 10);
				if (recordStartPos >= 0)
				{
					if (tmpString = strtok_r(nullptr, "	 ", &tmpContext))
					{
						unsigned long recordStopPos = strtoul(tmpString, 0, 10);
						if (recordStopPos >= recordStartPos)
						{
							_engine->start_recording(recordStartPos, recordStopPos);
						}
						else
							puts("0incorrect argument: record stop position can not be less then record start position");
					}
					else
					{
						_engine->start_recording(recordStartPos);
					}
				}
				else
				{
					puts("0incorrect argument: negative record start position");
				}
			}
			else
			{
				puts("0please point millisecond of record start position");
			}
		}
		else if (c == 'c')
		{
			if (paramString[0] == '0')
				_engine->stop_recording(false);
			else if (paramString[0] == '1')
				_engine->stop_recording(true);
			else
				printf("Stop recording: incorrect argument \"%s\"\n", paramString);
		}
		else if (c == 'd') // request if changed
		{
			puts(_engine->changed() ? "d1" : "d0");
		}
		else if (c == 'e') // save
		{
			_engine->save(paramString);
		}
		else if (c == 'f')
		{
			_engine->load_song(paramString, true);
		}
		else if (c == 'g')
		{
			_engine->applyPreloaded();
		}
		else
		{
			printf("0not supported command: %c. Passed with argument \"%s\"\n", c, paramString);
		}
	}
	return 0;
}
