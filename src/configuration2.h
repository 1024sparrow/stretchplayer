#pragma once

#include <string>

class Configuration2
{
public:
	enum class Mode
	{
		Undefined = 0,
		Alsa,
		Fake,
		Jack,
	};
	struct Common
	{
		int sampleRate {44100};
		bool mono {false};
		bool mic {false};
		int shift {0};
		int stretch {100};
		int pitch {0};
	};
	struct Alsa : Common
	{
		std::string device {"default"};
		int periodSize {1024};
		int periods {2};
	};
	struct Fake : Common
	{
		std::string fifoPlayback {"~/.stretchplayer-playback.fifo"};
		std::string fifoCapture {"~/.stretchplayer-capture.fifo"};
	};
	struct Jack : Common
	{
		bool noAutoconnect {false};
	};
	Configuration2() = default;
	int parse(int p_argc, char **p_argv, std::string *p_error); // return value: 0 if normal player start needed; 1 - if normal exit required; -1 - if error exit required (writing error description into p_error)

	Mode mode() const {return _mode;}
	const Alsa & alsa() const {return _data.alsa;}
	const Fake & fake() const {return _data.fake;}
	const Jack & jack() const {return _data.jack;}

	std::string toString() const;

private:
	const char *_configPath {nullptr};

	struct
	{
		Alsa alsa;
		Fake fake;
		Jack jack;
	} _data;
	Mode _mode {Mode::Undefined};
};