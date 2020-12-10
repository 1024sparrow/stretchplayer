#include "configuration2.h"
#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h> // exit()
#include <fcntl.h>
#include <unistd.h>

inline int collectError(std::string *p_error, const std::string &p_message)
{
	if (p_error)
	{
		if (p_error->size() > 0)
			*p_error += ":\n";
		*p_error += p_message;
	}
	return -1;
}

int Configuration2::parse(int p_argc, char **p_argv, std::string *p_error)
{
	/* boris here:
	проходим на предмет запроса справки: тогда показываем справку и выходим
		попутно записываем значение --config - отсюда мы получаем итоговый (_)configPath.
	проходим по нашему дереву:
		считываем из JSON общие параметры, режим и параметры, специфичные для режима.
	
	проходим по аргументам:
		считываем общие параметры, режим и параметры, специфичные для режима. Выводим сообщение об ошибке, если:
			* режим указан несколько раз
			* режим не указан, но указаны параметры, которых нет в числе общих параметров
			* режим указан, но не все указанные параметры есть в числе (общих и специфичных для указанного режима) параметров
	*/

	const char *configPath = nullptr;
	int state;
	/*
	states:
		0 - normal
		1 - config path expected
	*/
	state = 0;
	for (int iArg = 0 ; iArg < p_argc ; ++iArg)
	{
		const char *arg = p_argv[iArg];
		if (!strcmp(arg, "--help"))
		{
			std::cout << R"(--sampleRate
	sample rate to use for ALSA (default: 44100)
--mono
	merge all sound channels into the one: make it mono (default: false)
--mic
	use microphone for sound catching (default: false)
--shift
	right channel ahead of left (in seconds). Automaticaly make it mono. (default: 0)
--stretch
	playing speed (in percents) (default: 100)
--pitch
	frequency shift (number from -12 to 12) (default: 0)
--alsa
	use ALSA for audio
	Options:
	--device
		device to use for ALSA (default: "default")
	--periodSize
		period size to use for ALSA (default: 1024)
	--periods
		periods per buffer for ALSA (default: 2)
--fake
	use FIFO-s to write playback-data and read capture-data
	Options:
	--fifoPlayback
		filepath to the FIFO to write playback into (default: "~/.stretchplayer-playback.fifo")
	--fifoCapture
		filepath to the FIFO to read capture from (default: "~/.stretchplayer-capture.fifo")
--jack
	use JACK for audio
	Options:
	--noAutoconnect
		disable auto-connection ot ouputs (for JACK) (default: false))" << std::endl;
			exit(0);
		}
		else if (!strcmp(arg, "--config"))
		{
			state = 1;
		}
		else if (!strcmp(arg, "--mode"))
		{
			state = 2;
		}
		else if (state)
		{
			if (arg[0] == '-')
			{
				return collectError(p_error, std::string{state == 1 ? "\"--config\"" : "\"--mode\""} + ": parameter value not present");
			}
			configPath = arg;
		}
	}
	if (state)
		return collectError(p_error, std::string{state == 1 ? "\"--config\"" : "\"--mode\""} + ": parameter value not present");

	if (int fd = open(configPath, O_RDONLY))
	{
		// JSON parsing: not implemented yet
	}

	/*
	states:
		0 - normal
		1 - mode expected
	*/
	state = 0;
	for (int iArg = 0 ; iArg < p_argc ; ++iArg)
	{
		const char *arg = p_argv[iArg];
		if (!strcmp(arg, "--mode"))
		{
			state = 1;
		}
		else if (state)
		{
			if (arg[0] == '-')
			{
				//
			}
			else if (state == 1)
			{
				if (_mode != Mode::Undefined){
					return collectError(p_error, "only one time mode can be set");
				};
				if (!strcmp(arg, "alsa")) _mode = Mode::Alsa;
				else if (!strcmp(arg, "fake")) _mode = Mode::Fake;
				else if (!strcmp(arg, "jack")) _mode = Mode::Jack;
				else return collectError(p_error, "unsupported mode");
			}
		}
		else
		{
			// boris here
		}
	}

	return 0;
}

/*void Configuration2::printHelp()
{
}*/
