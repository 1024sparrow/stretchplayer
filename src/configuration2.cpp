// generated by generate_config.js
// source file is config.js

#include "configuration2.h"
#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h> // exit(), atoi()
#include <fcntl.h>
#include <unistd.h>
#include <tuple>
#include <set>

class Configuration2::JsonParser
{
public:
	JsonParser(Configuration2 *p_conf);
	// p_force - error, if file not exists (or can not read)
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
		DuplicatingKey,

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
			InparamsValueIntegerStarting,
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
				{ S::InparamsValueIntegerStarting, "InparamsValueIntegerStarting"},
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
		std::set<std::string> keysAlreadyUsed;
		std::set<std::string> inparamsKeysAlreadyUsed;
		struct
		{
			int value;
			bool negative;
		} intValue;
		int counter {0};
	} _state;
	Configuration2 *_conf;
};

int Configuration2::parse(int p_argc, char **p_argv, const char *p_helpPrefix, const char *p_helpPostfix, std::string *p_error)
{
	/* Порядок разбора аргументов командной строки:

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

	_configPath.clear();
	int needToGenerateConfig = 0; // 1 - generateConfig, 2 - showParameters
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
			if (p_helpPrefix)
			{
				std::cout << p_helpPrefix << std::endl;
			}
			std::cout << R"(--config
	set alternative config file path (default is "~/.stretchplayer.conf")
	${...} and ~ at the begin resolves to appropriate environment variable values
--config-gen
	generate config-file content (print it to stdout) and normal exit
--showOptionsInsteadOfApplying
	show parameters, resulting of config file (if exists) and command-line arguments, and exit normally
--quiet <true|false>
	suppress most output to console (default: false)
--sampleRate <number>
	sample rate to use for ALSA (default: 44100)
--mono <true|false>
	merge all sound channels into the one: make it mono (default: false)
--mic <true|false>
	use microphone for sound catching (default: false)
--shift <number>
	right channel ahead of left (in seconds). Automaticaly make it mono. (default: 0)
--stretch <number>
	playing speed (in percents) (default: 100)
--pitch <number>
	frequency shift (number from -12 to 12) (default: 0)
--alsa
	use ALSA for audio
	Options:
	--device <string>
		device to use for ALSA (default: "default")
	--periodSize <number>
		period size to use for ALSA (default: 1024)
	--periods <number>
		periods per buffer for ALSA (default: 2)
--fake
	use FIFO-s to write playback-data and read capture-data
	Options:
	--fifoPlayback <string>
		filepath to the FIFO to write playback into (default: "~/.stretchplayer-playback.fifo")
	--fifoCapture <string>
		filepath to the FIFO to read capture from (default: "~/.stretchplayer-capture.fifo")
--jack
	use JACK for audio
	Options:
	--noAutoconnect <true|false>
		disable auto-connection ot ouputs (for JACK) (default: false))" << std::endl;
			if (p_helpPostfix)
			{
				std::cout << p_helpPostfix << std::endl;
			}
			exit(0);
		}
		else if (!strcmp(arg, "--config"))
		{
			state = 1;
		}
		else if (!strcmp(arg, "--config-gen"))
		{
			needToGenerateConfig = 1;
		}
		else if (!strcmp(arg, "--showOptionsInsteadOfApplying"))
		{
			needToGenerateConfig = 2;
		}
		else if (state)
		{
			if (arg[0] == '-')
			{
				return collectError(p_error, "--config: parameter value not present");
			}
			if (_configPath.size())
				return collectError(p_error, "only one time config file path can be set");
			_configPath = resolveEnvVarsAndTilda(arg);
			state = 0;
		}
	}
	if (state)
		return collectError(p_error, "--config: parameter value not present");

	{ // prevent argument duplication
		std::set<std::string> usedArguments;
		for (int iArg = 0 ; iArg < p_argc ; ++iArg)
		{
			const char *arg = p_argv[iArg];
			if (arg[0] == '-')
			{
				if (usedArguments.find(arg) != usedArguments.end())
					return collectError(p_error, std::string(arg) + ": duplicating argument");
				usedArguments.insert(arg);
			}
		}
	}

	state = 0;
	for (int iArg = 0 ; iArg < p_argc ; ++iArg)
	{
		const char *arg = p_argv[iArg];
		Mode cand = Mode::Undefined;
		
		if (!strcmp(arg, "--alsa")) cand = Mode::Alsa;
		else if (!strcmp(arg, "--fake")) cand = Mode::Fake;
		else if (!strcmp(arg, "--jack")) cand = Mode::Jack;
		else
			continue;
		if (state == 0)
		{
			_mode = cand;
			state = 1;
		}
		else if (state == 1)
		{
			return collectError(p_error, "only one time mode can be set");
		}
	}

	bool usingDefaultConfig = _configPath.size();
	if (!usingDefaultConfig)
		_configPath = "~/.stretchplayer.conf";
	_configPath = resolveEnvVarsAndTilda(_configPath);

	Configuration2 confCopy = *this;
	JsonParser jsonParser(&confCopy);
	if (!jsonParser.parse(_configPath.c_str(), !usingDefaultConfig, p_error))
	{
		return collectError(p_error, "can not parse config");
	}
	if (_mode == Mode::Undefined && confCopy._mode != Mode::Undefined)
	{
		_mode = confCopy._mode;
	}
	if (confCopy._mode == Mode::Undefined || confCopy._mode == _mode)
	{
		_data = confCopy._data;
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
		else if (!strcmp(arg, "--config-gen"));
		else if (!strcmp(arg, "--showOptionsInsteadOfApplying"));
		else if (!strcmp(arg, "--alsa"));
		else if (!strcmp(arg, "--fake"));
		else if (!strcmp(arg, "--jack"));
		else if (!strcmp(arg, "--quiet"))
			state = 1;
		else if (!strcmp(arg, "--sampleRate"))
			state = 2;
		else if (!strcmp(arg, "--mono"))
			state = 3;
		else if (!strcmp(arg, "--mic"))
			state = 4;
		else if (!strcmp(arg, "--shift"))
			state = 5;
		else if (!strcmp(arg, "--stretch"))
			state = 6;
		else if (!strcmp(arg, "--pitch"))
			state = 7;
		else if (_mode == Mode::Alsa && !strcmp(arg, "--device"))
			state = 8;
		else if (_mode == Mode::Alsa && !strcmp(arg, "--periodSize"))
			state = 9;
		else if (_mode == Mode::Alsa && !strcmp(arg, "--periods"))
			state = 10;
		else if (_mode == Mode::Fake && !strcmp(arg, "--fifoPlayback"))
			state = 11;
		else if (_mode == Mode::Fake && !strcmp(arg, "--fifoCapture"))
			state = 12;
		else if (_mode == Mode::Jack && !strcmp(arg, "--noAutoconnect"))
			state = 13;
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
				_argv.push_back(arg);
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
					bool tmp;
					if (!strcmp(arg, "true"))
						tmp = true;
					else if (!strcmp(arg, "false"))
						tmp = false;
					else
						return collectError(p_error, "--quiet: true|false expected");
					_data.alsa.quiet = tmp;
					_data.fake.quiet = tmp;
					_data.jack.quiet = tmp;
				}
				else if (state == 2)
				{
					int tmp = atoi(arg);
					_data.alsa.sampleRate = tmp;
					_data.fake.sampleRate = tmp;
					_data.jack.sampleRate = tmp;
				}
				else if (state == 3)
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
				else if (state == 4)
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
				else if (state == 5)
				{
					int tmp = atoi(arg);
					_data.alsa.shift = tmp;
					_data.fake.shift = tmp;
					_data.jack.shift = tmp;
				}
				else if (state == 6)
				{
					int tmp = atoi(arg);
					_data.alsa.stretch = tmp;
					_data.fake.stretch = tmp;
					_data.jack.stretch = tmp;
				}
				else if (state == 7)
				{
					int tmp = atoi(arg);
					_data.alsa.pitch = tmp;
					_data.fake.pitch = tmp;
					_data.jack.pitch = tmp;
				}
				else if (state == 8)
				{
					std::string tmp = resolveEnvVarsAndTilda(arg);
					_data.alsa.device = tmp;
				}
				else if (state == 9)
				{
					int tmp = atoi(arg);
					_data.alsa.periodSize = tmp;
				}
				else if (state == 10)
				{
					int tmp = atoi(arg);
					_data.alsa.periods = tmp;
				}
				else if (state == 11)
				{
					std::string tmp = resolveEnvVarsAndTilda(arg);
					_data.fake.fifoPlayback = tmp;
				}
				else if (state == 12)
				{
					std::string tmp = resolveEnvVarsAndTilda(arg);
					_data.fake.fifoCapture = tmp;
				}
				else if (state == 13)
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

	if (needToGenerateConfig)
	{
		if (needToGenerateConfig == 1)
			std::cout << generateConf() << std::endl;
		else if (needToGenerateConfig == 2)
			std::cout << toString() << std::endl;
		exit(0);
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
		retVal += "\nquiet: ";;
		retVal += _data.alsa.quiet ? "true" : "false";
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
		retVal += "\nquiet: ";;
		retVal += _data.fake.quiet ? "true" : "false";
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
		retVal += "\nquiet: ";;
		retVal += _data.jack.quiet ? "true" : "false";
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

/*
 * resolve "~/qwe/${USER}/rty" to "/home/user/qwe/user/rty"
 * "${...}" resolves to appropriate environment variable value
 * "~" resolves to home directory path only if it is placed in the begin
 */
std::string Configuration2::resolveEnvVarsAndTilda(const std::string &p)
{
	int state = 0, substate = 0;
	std::string envName, retVal;
	for (char ch : p)
	{
		if (state == 0)
		{
			if (ch == '~')
			{
				retVal += getenv("HOME");
				state = 1;
			}
			else if (ch == '$')
			{
				state = 2;
				substate = 0;
			}
			else
			{
				retVal.push_back(ch);
				state = 1;
			}
		}
		else if (state == 1)
		{
			if (ch == '$')
			{
				state = 2;
				substate = 0;
			}
			else
			{
				retVal.push_back(ch);
			}
		}
		else if (state == 2)
		{
			if (substate == 0)
			{
				if (ch == '{')
				{
					substate = 1;
					envName.clear();
				}
				else
				{
					retVal.push_back('$');
					retVal.push_back(ch);
				}
			}
			else if (substate == 1)
			{
				if (ch == '}')
				{
					retVal += getenv(envName.c_str());
					state = 1;
				}
				else
				{
					envName.push_back(ch);
				}
			}
		}
	}
	return retVal;
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
	"duplicating key",

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

Configuration2::JsonParser::JsonParser(Configuration2 *p_conf)
	: _conf(p_conf)
{
}

bool Configuration2::JsonParser::parse(const char *p_filepath, bool p_force, std::string *p_error)
{
	int fd = open(p_filepath, O_RDONLY);
	if (fd <= 0)
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
	int lineNum = 1, colNum = 1;
	while(int result = read(fd, buffer, bufferSize))
	{
		if (result < 0)
		{
			close(fd);
			(void)collectError(p_error, strerror(errno));
			return collectError(p_error, "can not read file");
		}
		for (char i : buffer)
		{
			if (int err = static_cast<int>(parseTick(i))) // mode detecting
			{
				close(fd);
				sprintf(
					buffer,
					"line %i, column %i: %s.",
					lineNum,
					colNum,
					ERROR_CODE_DESCRIPTIONS[err]
				);
				return collectError(p_error, buffer);
			}
			if (_state.s == State::S::Finished)
				break;
			if (i == '\n')
			{
				++lineNum;
				colNum = 1;
			}
			else
			{
				++colNum;
			}
		}
	}
	if (_state.s != State::S::Finished)
	{
		close(fd);
		return collectError(p_error, "file is incomplete");
	}

	lseek(fd, 0, SEEK_SET);
	lineNum = 1, colNum = 1;
	_state = State();
	while(int result = read(fd, buffer, bufferSize))
	{
		if (result < 0)
		{
			close(fd);
			(void)collectError(p_error, strerror(errno));
			return collectError(p_error, "can not read file");
		}
		for (char i : buffer)
		{
			if (int err = static_cast<int>(parseTick(i))) // parameters saving
			{
				close(fd);
				sprintf(
					buffer,
					"line %i, column %i: %s.",
					lineNum,
					colNum,
					ERROR_CODE_DESCRIPTIONS[err]
				);
				return collectError(p_error, buffer);
			}
			if (_state.s == State::S::Finished)
				break;
			if (i == '\n')
			{
				++lineNum;
				colNum = 1;
			}
			else
			{
				++colNum;
			}
		}
	}
	close(fd);
	if (_state.s != State::S::Finished)
	{
		return collectError(p_error, "file is incomplete");
	}
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
	//printf("%c\t%s\n", byte, State::str(_state.s));

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

			if (_state.keysAlreadyUsed.find(_state.key) == _state.keysAlreadyUsed.end())
				_state.keysAlreadyUsed.insert(_state.key);
			else
				return Error::DuplicatingKey;
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
		if (isWhitespaceSymbol(byte))
			;
		else if (byte == ':')
		{
			if (_state.key == "quiet")
			{
				_state.s = State::S::InparamsValueBooleanStarting;
			}
			else if (_state.key == "sampleRate")
			{
				_state.intValue = {0, false};
				_state.s = State::S::InparamsValueIntegerStarting;
			}
			else if (_state.key == "mono")
			{
				_state.s = State::S::InparamsValueBooleanStarting;
			}
			else if (_state.key == "mic")
			{
				_state.s = State::S::InparamsValueBooleanStarting;
			}
			else if (_state.key == "shift")
			{
				_state.intValue = {0, false};
				_state.s = State::S::InparamsValueIntegerStarting;
			}
			else if (_state.key == "stretch")
			{
				_state.intValue = {0, false};
				_state.s = State::S::InparamsValueIntegerStarting;
			}
			else if (_state.key == "pitch")
			{
				_state.intValue = {0, false};
				_state.s = State::S::InparamsValueIntegerStarting;
			}
			else if (_conf->_mode == Mode::Alsa && _state.key == "device")
			{
				_state.s = State::S::InparamsValueStringStarting;
			}
			else if (_conf->_mode == Mode::Alsa && _state.key == "periodSize")
			{
				_state.intValue = {0, false};
				_state.s = State::S::InparamsValueIntegerStarting;
			}
			else if (_conf->_mode == Mode::Alsa && _state.key == "periods")
			{
				_state.intValue = {0, false};
				_state.s = State::S::InparamsValueIntegerStarting;
			}
			else if (_conf->_mode == Mode::Fake && _state.key == "fifoPlayback")
			{
				_state.s = State::S::InparamsValueStringStarting;
			}
			else if (_conf->_mode == Mode::Fake && _state.key == "fifoCapture")
			{
				_state.s = State::S::InparamsValueStringStarting;
			}
			else if (_conf->_mode == Mode::Jack && _state.key == "noAutoconnect")
			{
				_state.s = State::S::InparamsValueBooleanStarting;
			}
			else
			{
				return Error::UnsupportedKeyUsed;
			}

			if (_state.inparamsKeysAlreadyUsed.find(_state.key) == _state.inparamsKeysAlreadyUsed.end())
				_state.inparamsKeysAlreadyUsed.insert(_state.key);
			else
				return Error::DuplicatingKey;
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
			if (_conf->_mode == Mode::Alsa && _state.key == "device")
			{
				_conf->_data.alsa.device = resolveEnvVarsAndTilda(_state.value);
			}
			else if (_conf->_mode == Mode::Fake && _state.key == "fifoPlayback")
			{
				_conf->_data.fake.fifoPlayback = resolveEnvVarsAndTilda(_state.value);
			}
			else if (_conf->_mode == Mode::Fake && _state.key == "fifoCapture")
			{
				_conf->_data.fake.fifoCapture = resolveEnvVarsAndTilda(_state.value);
			}
			_state.s = State::S::InparamsValueFinished;
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
			bool newVal = true;
			if (_state.key == "quiet")
			{
				_conf->_data.alsa.quiet = newVal;
				_conf->_data.fake.quiet = newVal;
				_conf->_data.jack.quiet = newVal;
			}
			else if (_state.key == "mono")
			{
				_conf->_data.alsa.mono = newVal;
				_conf->_data.fake.mono = newVal;
				_conf->_data.jack.mono = newVal;
			}
			else if (_state.key == "mic")
			{
				_conf->_data.alsa.mic = newVal;
				_conf->_data.fake.mic = newVal;
				_conf->_data.jack.mic = newVal;
			}
			else if (_conf->_mode == Mode::Jack && _state.key == "noAutoconnect")
			{
				_conf->_data.jack.noAutoconnect = newVal;
			}
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
			bool newVal = false;
			if (_state.key == "quiet")
			{
				_conf->_data.alsa.quiet = newVal;
				_conf->_data.fake.quiet = newVal;
				_conf->_data.jack.quiet = newVal;
			}
			else if (_state.key == "mono")
			{
				_conf->_data.alsa.mono = newVal;
				_conf->_data.fake.mono = newVal;
				_conf->_data.jack.mono = newVal;
			}
			else if (_state.key == "mic")
			{
				_conf->_data.alsa.mic = newVal;
				_conf->_data.fake.mic = newVal;
				_conf->_data.jack.mic = newVal;
			}
			else if (_conf->_mode == Mode::Jack && _state.key == "noAutoconnect")
			{
				_conf->_data.jack.noAutoconnect = newVal;
			}
			_state.s = State::S::InparamsValueFinished;
		}
		++_state.counter;
	}
	else if (_state.s == State::S::InparamsValueIntegerStarting)
	{
		if (byte == '-')
		{
			_state.intValue.negative = true;
			_state.s = State::S::InparamsValueInteger;
		}
		else if (byte >= '0' && byte <= '9')
		{
			_state.intValue.value = byte - '0';
			_state.s = State::S::InparamsValueInteger;
		}
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::InparamsValueInteger)
	{
		if (byte >= '0' && byte <= '9')
			_state.intValue.value = _state.intValue.value * 10 + byte - '0';
		else if (byte == ',' || byte == '}' || isWhitespaceSymbol(byte))
		{
			int newVal = _state.intValue.value * (_state.intValue.negative ? -1 : 1);
			if (_state.key == "sampleRate")
			{
				_conf->_data.alsa.sampleRate = newVal;
				_conf->_data.fake.sampleRate = newVal;
				_conf->_data.jack.sampleRate = newVal;
			}
			else if (_state.key == "shift")
			{
				_conf->_data.alsa.shift = newVal;
				_conf->_data.fake.shift = newVal;
				_conf->_data.jack.shift = newVal;
			}
			else if (_state.key == "stretch")
			{
				_conf->_data.alsa.stretch = newVal;
				_conf->_data.fake.stretch = newVal;
				_conf->_data.jack.stretch = newVal;
			}
			else if (_state.key == "pitch")
			{
				_conf->_data.alsa.pitch = newVal;
				_conf->_data.fake.pitch = newVal;
				_conf->_data.jack.pitch = newVal;
			}
			else if (_conf->_mode == Mode::Alsa && _state.key == "periodSize")
			{
				_conf->_data.alsa.periodSize = newVal;
			}
			else if (_conf->_mode == Mode::Alsa && _state.key == "periods")
			{
				_conf->_data.alsa.periods = newVal;
			}
			_state.key.clear();
			_state.value.clear();
			if (byte == ',')
			{
				_state.s = State::S::ValueParameters;
			}
			if (byte == '}')
			{
				_state.s = State::S::Finished;
			}
		}
		else
			return Error::SystaxError;
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
		else if (byte == '}')
		{
			_state.key = "parameters";
			_state.s = State::S::ValueFinished;
		}
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ValueFinished)
	{
		if (_state.key == "mode")
		{
			if (_state.value == "alsa")
				_conf->_mode = Mode::Alsa;
			else if (_state.value == "fake")
				_conf->_mode = Mode::Fake;
			else if (_state.value == "jack")
				_conf->_mode = Mode::Jack;
			else
				return Error::IncorrectMode;
		}
		else if (_state.key == "parameters")
			;
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

std::string Configuration2::generateConf() const
{
	std::string retVal = "{";

	if (_mode == Mode::Alsa)
	{
		retVal += R"(
	"mode": "alsa",
	"parameters": {)";
	}
	else if (_mode == Mode::Fake)
	{
		retVal += R"(
	"mode": "fake",
	"parameters": {)";
	}
	else if (_mode == Mode::Jack)
	{
		retVal += R"(
	"mode": "jack",
	"parameters": {)";
	}
	else
	{
		retVal += R"(
	"parameters": {)";
	}

	if (_mode == Mode::Alsa)
	{
		retVal += R"(
		"quiet": )";
		if (_data.alsa.quiet)
			retVal += "true";
		else
			retVal += "false";
		retVal.push_back(',');
		retVal += R"(
		"sampleRate": )";
		retVal += std::to_string(_data.alsa.sampleRate);
		retVal.push_back(',');
		retVal += R"(
		"mono": )";
		if (_data.alsa.mono)
			retVal += "true";
		else
			retVal += "false";
		retVal.push_back(',');
		retVal += R"(
		"mic": )";
		if (_data.alsa.mic)
			retVal += "true";
		else
			retVal += "false";
		retVal.push_back(',');
		retVal += R"(
		"shift": )";
		retVal += std::to_string(_data.alsa.shift);
		retVal.push_back(',');
		retVal += R"(
		"stretch": )";
		retVal += std::to_string(_data.alsa.stretch);
		retVal.push_back(',');
		retVal += R"(
		"pitch": )";
		retVal += std::to_string(_data.alsa.pitch);
		retVal.push_back(',');
		retVal += R"(
		"device": ")";
		retVal += _data.alsa.device;
		retVal += "\"";
		retVal.push_back(',');
		retVal += R"(
		"periodSize": )";
		retVal += std::to_string(_data.alsa.periodSize);
		retVal.push_back(',');
		retVal += R"(
		"periods": )";
		retVal += std::to_string(_data.alsa.periods);
	}
	else if (_mode == Mode::Fake)
	{
		retVal += R"(
		"quiet": )";
		if (_data.fake.quiet)
			retVal += "true";
		else
			retVal += "false";
		retVal.push_back(',');
		retVal += R"(
		"sampleRate": )";
		retVal += std::to_string(_data.fake.sampleRate);
		retVal.push_back(',');
		retVal += R"(
		"mono": )";
		if (_data.fake.mono)
			retVal += "true";
		else
			retVal += "false";
		retVal.push_back(',');
		retVal += R"(
		"mic": )";
		if (_data.fake.mic)
			retVal += "true";
		else
			retVal += "false";
		retVal.push_back(',');
		retVal += R"(
		"shift": )";
		retVal += std::to_string(_data.fake.shift);
		retVal.push_back(',');
		retVal += R"(
		"stretch": )";
		retVal += std::to_string(_data.fake.stretch);
		retVal.push_back(',');
		retVal += R"(
		"pitch": )";
		retVal += std::to_string(_data.fake.pitch);
		retVal.push_back(',');
		retVal += R"(
		"fifoPlayback": ")";
		retVal += _data.fake.fifoPlayback;
		retVal += "\"";
		retVal.push_back(',');
		retVal += R"(
		"fifoCapture": ")";
		retVal += _data.fake.fifoCapture;
		retVal += "\"";
	}
	else if (_mode == Mode::Jack)
	{
		retVal += R"(
		"quiet": )";
		if (_data.jack.quiet)
			retVal += "true";
		else
			retVal += "false";
		retVal.push_back(',');
		retVal += R"(
		"sampleRate": )";
		retVal += std::to_string(_data.jack.sampleRate);
		retVal.push_back(',');
		retVal += R"(
		"mono": )";
		if (_data.jack.mono)
			retVal += "true";
		else
			retVal += "false";
		retVal.push_back(',');
		retVal += R"(
		"mic": )";
		if (_data.jack.mic)
			retVal += "true";
		else
			retVal += "false";
		retVal.push_back(',');
		retVal += R"(
		"shift": )";
		retVal += std::to_string(_data.jack.shift);
		retVal.push_back(',');
		retVal += R"(
		"stretch": )";
		retVal += std::to_string(_data.jack.stretch);
		retVal.push_back(',');
		retVal += R"(
		"pitch": )";
		retVal += std::to_string(_data.jack.pitch);
		retVal.push_back(',');
		retVal += R"(
		"noAutoconnect": )";
		if (_data.jack.noAutoconnect)
			retVal += "true";
		else
			retVal += "false";
	}
	// else { ... } // set common only parameters. Not supported work without an active mode.

	return retVal + R"(
	}
})";
}
