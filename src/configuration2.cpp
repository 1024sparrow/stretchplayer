#include "configuration2.h"
#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h> // exit(), atoi()
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

	_configPath = nullptr;
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
			std::cout << R"(--config
	set alternative config file path (default is "~/.stretchplayer.conf")
--config-add
	copy current config and add options to it
--config-rewrite
	create new config and add options to it
--sampleRate
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
		else if (state)
		{
			if (arg[0] == '-')
			{
				return collectError(p_error, "--config: parameter value not present");
			}
			if (_configPath)
				return collectError(p_error, "only one time config file path can be set");
			_configPath = arg;
			state = 0;
		}
	}
	if (state)
		return collectError(p_error, "--config: parameter value not present");

	if (!_configPath)
		_configPath = "~/.stretchplayer.conf";

	if (int fd = open(_configPath, O_RDONLY))
	{
		// JSON parsing: not implemented yet
	}

	for (int iArg = 0 ; iArg < p_argc ; ++iArg)
	{
		const char *arg = p_argv[iArg];
		Mode cand = Mode::Undefined;
		
		if (!strcmp(arg, "--alsa")) cand = Mode::Alsa;
		else if (!strcmp(arg, "--fake")) cand = Mode::Fake;
		else if (!strcmp(arg, "--jack")) cand = Mode::Jack;
		if (cand != Mode::Undefined)
		{
			if (_mode != Mode::Undefined){
				return collectError(p_error, "only one time mode can be set");
			};
			_mode = cand;
		}
	}
	if (_mode == Mode::Undefined)
	{
		return collectError(p_error, "mode not set");
	}
	state = 0;
	/*
	states:
		0 - normal
		1 - mode expected
	*/
	for (int iArg = 0 ; iArg < p_argc ; ++iArg)
	{
		const char *arg = p_argv[iArg];
		if (!strcmp(arg, "--config"))
			state = -1;
		else if (!strcmp(arg, "--alsa"));
		else if (!strcmp(arg, "--fake"));
		else if (!strcmp(arg, "--jack"));
		else if (!strcmp(arg, "--sampleRate"))
			state = 1;
		else if (!strcmp(arg, "--mono"))
			state = 2;
		else if (!strcmp(arg, "--mic"))
			state = 3;
		else if (!strcmp(arg, "--shift"))
			state = 4;
		else if (!strcmp(arg, "--stretch"))
			state = 5;
		else if (!strcmp(arg, "--pitch"))
			state = 6;
		else if (_mode == Mode::Alsa && !strcmp(arg, "--device"))
			state = 7;
		else if (_mode == Mode::Alsa && !strcmp(arg, "--periodSize"))
			state = 8;
		else if (_mode == Mode::Alsa && !strcmp(arg, "--periods"))
			state = 9;
		else if (_mode == Mode::Fake && !strcmp(arg, "--fifoPlayback"))
			state = 10;
		else if (_mode == Mode::Fake && !strcmp(arg, "--fifoCapture"))
			state = 11;
		else if (_mode == Mode::Jack && !strcmp(arg, "--noAutoconnect"))
			state = 12;
		else
		{
			if (state == 0)
			{
				// свободный аргумент (вне параметров)
				if (arg[0] == '-')
				{
					std::string errorDescr = "unknown key: \"";
					errorDescr += arg;
					errorDescr += "\"";
					return collectError(p_error, errorDescr);
				}
			}
			else if (state == -1)
			{
				state = 0;
			}
			else if (state == 1)
			{
				int tmp = atoi(arg);
				_data.alsa.sampleRate = tmp;
				_data.fake.sampleRate = tmp;
				_data.jack.sampleRate = tmp;
				state = 0;
			}
			else if (state == 2)
			{
				bool tmp;
				if (!strcmp(arg, "true"))
					tmp = true;
				else if (!strcmp(arg, "false"))
					tmp = false;
				else
					return collectError(p_error, "--mono: true|false expected");
				_data.alsa.mono = tmp;
				_data.fake.mono = tmp;
				_data.jack.mono = tmp;
				state = 0;
			}
			else if (state == 3)
			{
				bool tmp;
				if (!strcmp(arg, "true"))
					tmp = true;
				else if (!strcmp(arg, "false"))
					tmp = false;
				else
					return collectError(p_error, "--mic: true|false expected");
				_data.alsa.mic = tmp;
				_data.fake.mic = tmp;
				_data.jack.mic = tmp;
				state = 0;
			}
			else if (state == 4)
			{
				int tmp = atoi(arg);
				_data.alsa.shift = tmp;
				_data.fake.shift = tmp;
				_data.jack.shift = tmp;
				state = 0;
			}
			else if (state == 5)
			{
				int tmp = atoi(arg);
				_data.alsa.stretch = tmp;
				_data.fake.stretch = tmp;
				_data.jack.stretch = tmp;
				state = 0;
			}
			else if (state == 6)
			{
				int tmp = atoi(arg);
				_data.alsa.pitch = tmp;
				_data.fake.pitch = tmp;
				_data.jack.pitch = tmp;
				state = 0;
			}
			else if (state == 7)
			{
				const char *tmp = arg;
				_data.alsa.device = tmp;
				state = 0;
			}
			else if (state == 8)
			{
				int tmp = atoi(arg);
				_data.alsa.periodSize = tmp;
				state = 0;
			}
			else if (state == 9)
			{
				int tmp = atoi(arg);
				_data.alsa.periods = tmp;
				state = 0;
			}
			else if (state == 10)
			{
				const char *tmp = arg;
				_data.fake.fifoPlayback = tmp;
				state = 0;
			}
			else if (state == 11)
			{
				const char *tmp = arg;
				_data.fake.fifoCapture = tmp;
				state = 0;
			}
			else if (state == 12)
			{
				bool tmp;
				if (!strcmp(arg, "true"))
					tmp = true;
				else if (!strcmp(arg, "false"))
					tmp = false;
				else
					return collectError(p_error, "--noAutoconnect: true|false expected");
				_data.jack.noAutoconnect = tmp;
				state = 0;
			}
		}
	}

	return 0;
}

std::string Configuration2::toString() const
{
	std::string retVal;retVal += "mode: ";
	if (_mode == Mode::Alsa)
		retVal += "alsa";
	else if (_mode == Mode::Fake)
		retVal += "fake";
	else if (_mode == Mode::Jack)
		retVal += "jack";
	else
		retVal += "<not set>";
	retVal += "\nconfig file: \"";
	retVal += _configPath;
	retVal += "\"";
	if (_mode == Mode::Undefined)
		return retVal;
	if (_mode == Mode::Alsa)
	{
		retVal += "\n\nCOMMON OPTIONS:\n";
		retVal += "\nsampleRate: ";;
		retVal += std::to_string(_data.alsa.sampleRate);
		retVal += "\nmono: ";;
		retVal += _data.alsa.mono ? "true" : "false";
		retVal += "\nmic: ";;
		retVal += _data.alsa.mic ? "true" : "false";
		retVal += "\nshift: ";;
		retVal += std::to_string(_data.alsa.shift);
		retVal += "\nstretch: ";;
		retVal += std::to_string(_data.alsa.stretch);
		retVal += "\npitch: ";;
		retVal += std::to_string(_data.alsa.pitch);
		retVal += "\n\nMODE SPECIFIC OPTIONS:\n";
		retVal += "\ndevice: ";
		retVal += "\"";
		retVal += _data.alsa.device;
		retVal += "\"";
		retVal += "\nperiodSize: ";;
		retVal += std::to_string(_data.alsa.periodSize);
		retVal += "\nperiods: ";;
		retVal += std::to_string(_data.alsa.periods);
	}
	else if (_mode == Mode::Fake)
	{
		retVal += "\n\nCOMMON OPTIONS:\n";
		retVal += "\nsampleRate: ";;
		retVal += std::to_string(_data.fake.sampleRate);
		retVal += "\nmono: ";;
		retVal += _data.fake.mono ? "true" : "false";
		retVal += "\nmic: ";;
		retVal += _data.fake.mic ? "true" : "false";
		retVal += "\nshift: ";;
		retVal += std::to_string(_data.fake.shift);
		retVal += "\nstretch: ";;
		retVal += std::to_string(_data.fake.stretch);
		retVal += "\npitch: ";;
		retVal += std::to_string(_data.fake.pitch);
		retVal += "\n\nMODE SPECIFIC OPTIONS:\n";
		retVal += "\nfifoPlayback: ";
		retVal += "\"";
		retVal += _data.fake.fifoPlayback;
		retVal += "\"";
		retVal += "\nfifoCapture: ";
		retVal += "\"";
		retVal += _data.fake.fifoCapture;
		retVal += "\"";
	}
	else if (_mode == Mode::Jack)
	{
		retVal += "\n\nCOMMON OPTIONS:\n";
		retVal += "\nsampleRate: ";;
		retVal += std::to_string(_data.jack.sampleRate);
		retVal += "\nmono: ";;
		retVal += _data.jack.mono ? "true" : "false";
		retVal += "\nmic: ";;
		retVal += _data.jack.mic ? "true" : "false";
		retVal += "\nshift: ";;
		retVal += std::to_string(_data.jack.shift);
		retVal += "\nstretch: ";;
		retVal += std::to_string(_data.jack.stretch);
		retVal += "\npitch: ";;
		retVal += std::to_string(_data.jack.pitch);
		retVal += "\n\nMODE SPECIFIC OPTIONS:\n";
		retVal += "\nnoAutoconnect: ";;
		retVal += _data.jack.noAutoconnect ? "true" : "false";
	}
	return retVal;
}
