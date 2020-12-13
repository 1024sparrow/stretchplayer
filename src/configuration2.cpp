#include "configuration2.h"
#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h> // exit(), atoi()
#include <fcntl.h>
#include <unistd.h>
#include <tuple>

#include <locale.h> // for futher usage: currently only ASCII field values supported

class Configuration2::JsonParser
{
public:
	JsonParser();
	bool parse(const char *p_filepath, bool p_force, std::string *p_error);
private:
	enum class Error
	{
		NoError = 0,
		NotImplemented,
		UnhandledState,
		SystaxError,
		UnsupportedKeyUsed,
		InobjectCommaExpected,
		UnexpectedComma,
		TrueFalseExpected,

		IncorrectMode,

		CanNotOpenFile,
		__Count
	};
	bool collectError(std::string *p_error, const std::string &p_message) const;
	void initParse();
	Error parseTick(char byte);

	bool isWhitespaceSymbol(char byte)
	{
		if (byte == ' ' || byte == '\t' || byte == '\r' || byte == '\n')
			return true;
		return false;
	}
	bool isFilenameSymbol(char byte)
	{
		if (byte == ' ' || byte == '_')
			return true;
		if (byte >= ',' && byte <= '9')
			return true;
		if (byte >= 'A' && byte <= 'Z')
			return true;
		if (byte >= 'a' && byte <= 'z')
			return true;
		return false;
	}

	static const char * ERROR_CODE_DESCRIPTIONS[];
	static const char
		*USER_MARK,
		*TRUE,
		*FALSE
	;
	static const int
		USER_MARK_LEN,
		TRUE_LEN,
		FALSE_LEN
	;

	struct State
	{
		enum class S
		{
			Init,
			IntoGlobalObject,
			KeyStarting,
			KeyValueSeparator,
			ValueModeStarting,
			ValueMode,
			ValueParametersStarting,
			ValueParameters,
			ParametersKey,
			InparamsSeparator,
			InparamsValueStringStarting,
			InparamsValueString,
			InparamsValueBooleanStarting,
			InparamsValueBooleanTrue,
			InparamsValueBooleanFalse,
			InparamsValueInteger,
			InparamsValueFinished,
			ValueFinished,

			Finished
		} s {S::Init};
		static const char * str(S p)
		{
			static const std::tuple<S, const char *> srcData_123[] = {
				{ S::Init, "Init" },
				{ S::IntoGlobalObject, "IntoGlobalObject" },
				{ S::KeyStarting, "KeyStarting" },
				{ S::KeyValueSeparator, "KeyValueSeparator" },
				{ S::ValueModeStarting, "ValueModeStarting" },
				{ S::ValueMode, "ValueMode" },
				{ S::ValueParametersStarting, "ValueParametersStarting" },
				{ S::ValueParameters, "ValueParameters" },
				{ S::ParametersKey, "ParametersKey"},
				{ S::InparamsSeparator, "InparamsSeparator"},
				{ S::InparamsValueStringStarting, "InparamsValueStringStarting"},
				{ S::InparamsValueString, "InparamsValueString"},
				{ S::InparamsValueBooleanStarting, "InparamsValueBooleanStarting"},
				{ S::InparamsValueBooleanTrue, "InparamsValueBooleanTrue"},
				{ S::InparamsValueBooleanFalse, "InparamsValueBooleanFalse"},
				{ S::InparamsValueInteger, "InparamsValueInteger"},
				{ S::InparamsValueFinished, "InparamsValueFinished"},

				{ S::ValueFinished, "ValueFinished" },
				{ S::Finished, "Finished" }
			};
			for (auto o : srcData_123)
			{
				if (p == std::get<0>(o))
					return std::get<1>(o);
			}
			return "<incorrect>";
		}
		std::string key;
		std::string value;
		int counter {0};
	} _state;
};

int Configuration2::parse(int p_argc, char **p_argv, std::string *p_error)
{
	/* boris here:
	1. проходим на предмет запроса справки: тогда показываем справку и выходим
		попутно записываем значение --config - отсюда мы получаем итоговый (_)configPath.
	2. Вычитываем конфигурационный файл в копию структуры
	3. Проходим по аргументам: определяем режим работы
	4. Если режим работы совпадает с режимом в копии структуры, полученной от парсинга файла конфига, то копируем себе приватные поля копии структуры.
	5. Проходим по аргументам: правим отдельные поля приватных структур.

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

	bool usingDefaultConfig = _configPath;
	if (!_configPath)
		_configPath = "~/.stretchplayer.conf";
	JsonParser jsonParser;
	if (!jsonParser.parse(_configPath, !usingDefaultConfig, p_error))
	{
		return collectError(p_error, "can not parse config");
	}

	/*if (int fd = open(_configPath, O_RDONLY))
	{
		// JSON parsing: not implemented yet
	}*/

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
			else
			{
				if (arg[0] == '-')
				{
					std::string errorDescr = "unexpected key: \"";
					errorDescr += arg;
					errorDescr += "\". Expected previous parameter value instead.";
					return collectError(p_error, errorDescr);
				}
				if (state == -1)
				{
					state = 0;
				}
				else if (state == 1)
				{
					int tmp = atoi(arg);
					_data.alsa.sampleRate = tmp;
					_data.fake.sampleRate = tmp;
					_data.jack.sampleRate = tmp;
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
				}
				else if (state == 4)
				{
					int tmp = atoi(arg);
					_data.alsa.shift = tmp;
					_data.fake.shift = tmp;
					_data.jack.shift = tmp;
				}
				else if (state == 5)
				{
					int tmp = atoi(arg);
					_data.alsa.stretch = tmp;
					_data.fake.stretch = tmp;
					_data.jack.stretch = tmp;
				}
				else if (state == 6)
				{
					int tmp = atoi(arg);
					_data.alsa.pitch = tmp;
					_data.fake.pitch = tmp;
					_data.jack.pitch = tmp;
				}
				else if (state == 7)
				{
					const char *tmp = arg;
					_data.alsa.device = tmp;
				}
				else if (state == 8)
				{
					int tmp = atoi(arg);
					_data.alsa.periodSize = tmp;
				}
				else if (state == 9)
				{
					int tmp = atoi(arg);
					_data.alsa.periods = tmp;
				}
				else if (state == 10)
				{
					const char *tmp = arg;
					_data.fake.fifoPlayback = tmp;
				}
				else if (state == 11)
				{
					const char *tmp = arg;
					_data.fake.fifoCapture = tmp;
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
				}
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

int Configuration2::collectError(std::string *p_error, const std::string &p_message) const
{
	if (p_error)
	{
		if (p_error->size() > 0)
			*p_error = ":\n" + *p_error;
		*p_error = p_message + *p_error;
	}
	return -1;
}

const char * Configuration2::JsonParser::ERROR_CODE_DESCRIPTIONS[] {
	"no error",
	"internal error (not implemented)",
	"internal error (unhandled state)",
	"syntax error",
	"unsupported key used",
	"comma between object key-value pairs expected",
	"unexpected comma not between objects in object",
	"true|false expected",

	"there is not such mode",

	"can not open file",
	"" // for invalid error code
};

const char
	*Configuration2::JsonParser::USER_MARK = "{user}",
	*Configuration2::JsonParser::TRUE = "true",
	*Configuration2::JsonParser::FALSE = "false"
;
const int
	Configuration2::JsonParser::USER_MARK_LEN = strlen(USER_MARK),
	Configuration2::JsonParser::TRUE_LEN = strlen(TRUE),
	Configuration2::JsonParser::FALSE_LEN = strlen(FALSE)
;

Configuration2::JsonParser::JsonParser()
{
}

bool Configuration2::JsonParser::parse(const char *p_filepath, bool p_force, std::string *p_error)
{
	int fd = open(p_filepath, O_RDONLY);
	if (!fd)
	{
		if (p_force)
		{
			return true;
		}
		return collectError(p_error, "can not open file");
	}
	const int bufferSize = 1024;
	char buffer[bufferSize];
	initParse();
	while(int result = read(fd, buffer, bufferSize))
	{
		if (result < 0)
		{
			close(fd);
			(void)collectError(p_error, strerror(errno));
			return collectError(p_error, "can read file");
		}
		for (char i : buffer)
		{
			if (int err = static_cast<int>(parseTick(i)))
			{
				close(fd);
				return collectError(p_error, ERROR_CODE_DESCRIPTIONS[err]);
			}
			if (_state.s == State::S::Finished)
				break;
		}
	}
	if (_state.s != State::S::Finished)
	{
		return collectError(p_error, "file is incomplete");
	}
	puts("Pipes configuration file parsed successfully");//
	return true;
}

bool Configuration2::JsonParser::collectError(std::string *p_error, const std::string &p_message) const
{
	if (p_error)
	{
		if (p_error->size() > 0)
			*p_error = ":\n" + *p_error;
		*p_error = p_message + *p_error;
	}
	return false;
}

void Configuration2::JsonParser::initParse()
{
	_state = State();
}

Configuration2::JsonParser::Error Configuration2::JsonParser::parseTick(char byte)
{
	printf("%c\t%s\n", byte, State::str(_state.s));
	if (_state.s == State::S::Init)
	{
		if (isWhitespaceSymbol((byte)))
			;
		else if (byte == '{')
			_state.s = State::S::IntoGlobalObject;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::IntoGlobalObject)
	{
		if (isWhitespaceSymbol(byte))
			;
		else if (byte == '"'){
			_state.s = State::S::KeyStarting;
			_state.key.clear();
		}
		else if (byte == '}'){
			_state.s = State::S::Finished;
		}
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::KeyStarting)
	{
		if (byte == '"')
		{
			_state.s = State::S::KeyValueSeparator;
		}
		else if (isFilenameSymbol(byte))
		{
			_state.key.push_back(byte);
		}
		else
		{
			return Error::SystaxError;
		}
	}
	else if (_state.s == State::S::KeyValueSeparator)
	{
		if (isWhitespaceSymbol(byte))
			;
		else if (byte == ':')
		{
			if (_state.key == "mode")
				_state.s = State::S::ValueModeStarting;
			else if (_state.key == "parameters")
				_state.s = State::S::ValueParametersStarting;
			else
				return Error::UnsupportedKeyUsed;
		}
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ValueModeStarting)
	{
		if (byte == '"')
		{
			_state.s = State::S::ValueMode;
			_state.value.clear();
		}
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ValueMode)
	{
		if (byte == '"')
		{
			_state.s = State::S::ValueFinished;
		}
		else if (isFilenameSymbol(byte))
		{
			_state.value.push_back(byte);
		}
		else
			return Error::SystaxError; // некорректный символ в строке имени режима
	}
	else if (_state.s == State::S::ValueParametersStarting)
	{
		if (byte == '{')
		{
			_state.s = State::S::ValueParameters;
			_state.key.clear();
			_state.value.clear();
		}
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ValueParameters)
	{
		if (byte == '"')
		{
			_state.s = State::S::ParametersKey;
		}
		else if (byte == '}')
		{
			_state.s = State::S::IntoGlobalObject;
		}
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ParametersKey)
	{
		if (byte == '"')
			_state.s = State::S::InparamsSeparator;
		else if (isFilenameSymbol(byte))
		{
			_state.key.push_back(byte);
		}
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::InparamsSeparator)
	{
//		return Error::NotImplemented;
		if (isWhitespaceSymbol(byte))
			;
		else if (byte == ':')
		{
			if (_state.key == "mono")
				_state.s = State::S::InparamsValueBooleanStarting;
			else if (_state.key == "device")
				_state.s = State::S::InparamsValueStringStarting;
			else if (_state.key == "priodSize")
				_state.s = State::S::InparamsValueInteger;
			else
			{
				std::cout << "KEY: " << _state.key << std::endl;
				return Error::UnsupportedKeyUsed;
			}
		}
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::InparamsValueStringStarting)
	{
		if (byte == '"')
		{
			_state.s = State::S::InparamsValueString;
		}
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::InparamsValueString)
	{
		if (byte == '"')
		{
			if (_state.key == "device")
				printf("** write value: _device = \"%s\"\n", _state.value.c_str());

			_state.s = State::S::ValueParameters;
		}
		else if (isFilenameSymbol(byte))
			_state.value.push_back(byte);
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::InparamsValueBooleanStarting)
	{
		if (isWhitespaceSymbol(byte))
			;
		else  if (byte == TRUE[0])
		{
			_state.s = State::S::InparamsValueBooleanTrue;
		}
		else if (byte == FALSE[0])
		{
			_state.s = State::S::InparamsValueBooleanFalse;
		}
		else
			return Error::TrueFalseExpected;
		_state.counter = 1;
	}
	else if (_state.s == State::S::InparamsValueBooleanTrue)
	{
		if (TRUE[_state.counter] != byte)
			return Error::TrueFalseExpected;
		if (_state.counter == TRUE_LEN - 1)
		{
			if (_state.key == "mono")
				puts("** write value: _mono = true");
			_state.s = State::S::InparamsValueFinished;
		}
		++_state.counter;
	}
	else if (_state.s == State::S::InparamsValueBooleanFalse)
	{
		if (FALSE[_state.counter] != byte)
			return Error::TrueFalseExpected;
		if (_state.counter == FALSE_LEN - 1)
		{
			if (_state.key == "mono")
				puts("** write value: _mono = false");
			_state.s = State::S::InparamsValueFinished;
		}
		++_state.counter;
	}
	else if (_state.s == State::S::InparamsValueInteger)
	{
		return Error::NotImplemented;
	}
	else if (_state.s == State::S::InparamsValueFinished)
	{
		if (isWhitespaceSymbol(byte))
			;
		else if (byte == ',')
		{
			_state.key.clear();
			_state.value.clear();
			_state.s = State::S::ValueParameters;
		}
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ValueFinished)
	{
		if (_state.key == "mode")
		{
			if (_state.value == "alsa")
			{
				//
				puts("** write value: _mode = Mode::Alsa");
			}
			else
			{
				return Error::IncorrectMode;
			}
		}
		else
		{
			puts("<EMERGENCY>"); // it's internal epic fail to be here (assert() this). Неподдерживаемые ключи должны были быть пресечены ранее (ещё до считывания значения)
			return Error::UnsupportedKeyUsed;
		}

		if (byte == ',')
			_state.s = State::S::IntoGlobalObject;
		else if (byte == '}')
			_state.s = State::S::Finished;
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::InobjectCommaExpected;
	}
	else
	{
		return Error::UnhandledState;
	}

	return Error::NoError;
}
