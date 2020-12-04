#include "PipesConfParser.hpp"

#include <fcntl.h>
#include <unistd.h>

#include "string.h"
#include <iostream> //

//#define err(T) static_cast<int>(T)
//operator int()()

/* only ASCII, without \\
{
	"playback" : "/home/${user}/sound/playback",
	"capture": "~/sound/capture",
	"remote": false
}
*/

const char
	*USER_MARK = "{user}",
	*TRUE = "true",
	*FALSE = "false"
;
const int
	USER_MARK_LEN = strlen(USER_MARK),
	TRUE_LEN = strlen(TRUE),
	FALSE_LEN = strlen(FALSE)
;

bool isWhitespaceSymbol(char byte)
{
	if (byte == ' ' || byte == '\t' || byte == '\r' || byte == '\n')
		return true;
	return false;
}
bool isFilenameSymbol(char byte)
{
	if (byte >= ',' && byte <= '9')
		return true;
	if (byte >= 'A' && byte <= 'Z')
		return true;
	if (byte >= 'a' && byte <= 'z')
		return true;
	return false;
}

const char * ERROR_CODE_DESCRIPTIONS[] = {
	"no error",
	"syntax error",
	"unsupported key used",
	"comma between object key-value pairs expected",
	"unexpected comma not between objects in object",

	"can not open file",
	"file is incomplete",
	"" // for invalid error code
};

namespace StretchPlayer
{

PipesConfParser::PipesConfParser()
{
	//
}

int PipesConfParser::parse(const char *filepath)
{
	int fd = open(filepath, O_RDONLY);
	if (!fd)
		return static_cast<int>(Error::CanNotOpenFile);
	const int bufferSize = 1024;
	char buffer[bufferSize];
	initParse();
	while(int result = read(fd, buffer, bufferSize))
	{
		if (result < 0)
			return false;
		for (char i : buffer)
		{
			if (int err = static_cast<int>(parseTick(i)))
				return err;
			if (_state.s == State::S::Finished)
				break;
		}
	}
	if (_state.s != State::S::Finished)
	{
		return static_cast<int>(Error::IncompleteFile);
	}
	puts("ok");//
	return 0;
}

const char * PipesConfParser::error(int errorCode)
{
	if (errorCode < 0 || errorCode >= static_cast<int>(Error::__Count))
	{
		return ERROR_CODE_DESCRIPTIONS[static_cast<int>(Error::__Count)];
	}
	return ERROR_CODE_DESCRIPTIONS[errorCode];
}

const PipesConf & PipesConfParser::result() const
{
	return _pipesConf;
}

void PipesConfParser::initParse()
{
	_state = State();
}

PipesConfParser::Error PipesConfParser::parseTick(char byte)
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
			if (_state.key == "playback")
				_state.s = State::S::ValuePlaybackStarting;
			else if (_state.key == "capture")
				_state.s = State::S::ValueCaptureStarting;
			else if (_state.key == "remote")
			{
				_state.s = State::S::ValueRemote;
				_state.counter = 0;
			}
			else
				return Error::SystaxError;
		}
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ValuePlaybackStarting)
	{
		if (byte == '"')
		{
			_state.s = State::S::ValuePlayback;
			_state.value.clear();
		}
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ValueCaptureStarting)
	{
		if (byte == '"')
		{
			_state.s = State::S::ValueCapture;
			_state.value.clear();
		}
		else if (isWhitespaceSymbol(byte))
			;
		else
			return Error::SystaxError;
	}
	else if (_state.s == State::S::ValueRemote)
	{
//		if (_state.counter < TRUE_LEN && byte == TRUE[_state.counter++])
//		{
//			_pipesConf.remote.type = PipesConf::Remote::TypePipe;
//		}
	}
	else if (_state.s == State::S::ValuePlayback)
	{
		if (byte == '"')
		{
			_state.s = State::S::ValueFinished;
		}
		else if (isFilenameSymbol(byte))
		{
			_state.value.push_back(byte);
		}
		else if (byte == '~' && _state.value.size() == 0)
		{
			_state.s = State::S::ValuePlaybackTilda;
		}
		else if (byte == '$')
		{
			_state.s = State::S::ValuePlaybackDollar;
			_state.counter = 0;
		}
	}
	else if (_state.s == State::S::ValueCapture)
	{
		if (byte == '"')
		{
			_state.s = State::S::ValueFinished;
		}
		else if (isFilenameSymbol(byte))
		{
			_state.value.push_back(byte);
		}
		else if (byte == '~' && _state.value.size() == 0)
		{
			_state.s = State::S::ValueCaptureTilda;
		}
		else if (byte == '$')
		{
			_state.s = State::S::ValueCaptureDollar;
		}
	}
	else if (_state.s == State::S::ValuePlaybackTilda)
	{
		if (byte == '"')
		{
			// boris here: save value
			_state.s = State::S::ValueFinished;
		}
		else if (byte == '/')
		{
			_state.value.append(getenv("HOME"));
			_state.value.push_back('/');
			_state.s = State::S::ValuePlayback;
		}
		else if (isFilenameSymbol(byte))
		{
			_state.value.push_back('~');
			_state.value.push_back(byte);
		}
		else if (byte == '$')
		{
			_state.value.push_back('~');
			_state.s = State::S::ValuePlaybackDollar;
		}
	}
	else if (_state.s == State::S::ValueCaptureTilda)
	{
		//
	}
	else if (_state.s == State::S::ValuePlaybackDollar)
	{
		if (_state.counter < USER_MARK_LEN && byte == USER_MARK[_state.counter])
			;
		else
			return Error::SystaxError;

		if (++_state.counter == USER_MARK_LEN)
		{
			_state.value.append(getenv("USER"));
			_state.s = State::S::ValuePlayback;
		}
	}
	else if (_state.s == State::S::ValueCaptureDollar)
	{
		//
	}
	else if (_state.s == State::S::ValueFinished)
	{
		if (_state.key == "playback")
		{
			//_pipesConf.playback = _state.value;
			//printf("playback: %s", _state.value);//

			std::cout << "playback: " << _state.value << "\n"; //
		}
		else if (_state.key == "capture")
		{
			//_pipesConf.capture = _state.value;
			//printf("capture: %s", _state.value);//

			std::cout << "capture: " << _state.value << "\n"; //
		}
		else if (_state.key == "remote")
		{
			//
		}
		else
		{
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
	//
	return Error::NoError;
}

} // namespace StretchPlayer
