#include "PipesConfParser.hpp"

#include <fcntl.h>
#include <unistd.h>

/* only ASCII, without \\
{
	"playback" : "/home/${user}/sound/playback",
	"capture": "~/sound/capture",
	"remote": false
}
*/

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

namespace StretchPlayer
{

PipesConfParser::PipesConfParser()
{
	//
}

bool PipesConfParser::parse(const char *filepath)
{
	int fd = open(filepath, O_RDONLY);
	if (!fd)
		return false;
	const int bufferSize = 1024;
	char buffer[bufferSize];
	initParse();
	while(int result = read(fd, buffer, bufferSize))
	{
		if (result < 0)
			return false;
		for (char i : buffer)
		{
			if (!parseTick(i))
				return false;
		}
	}
	if (_state.s != State::S::Finished)
	{
		return false;
	}
	return true;
}

const PipesConf & PipesConfParser::result() const
{
	return _pipesConf;
}

void PipesConfParser::initParse()
{
	_state = State();
}

bool PipesConfParser::parseTick(char byte)
{
	if (_state.s == State::S::Init)
	{
		if (isWhitespaceSymbol((byte)))
			;
		else if (byte == '{')
			_state.s = State::S::IntoGlobalObject;
		else
			return false;
	}
	else if (_state.s == State::S::IntoGlobalObject)
	{
		if (isWhitespaceSymbol(byte))
			;
		else if (byte == '"'){
			_state.s = State::S::KeyStarting;
			_state.key.clear();
		}
		else
			return false;
	}
	else if (_state.s == State::S::KeyStarting)
	{
		if (byte == '"')
		{
			_state.s = State::S::KeyValueSeparator;
		}
		else if (isFilenameSymbol(byte))
		{
			_state.key.append(1, byte);
		}
		else
		{
			return false;
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
				_state.s = State::S::ValueRemote;
			else
				return false;
		}
		else
			return false;
	}
	// boris here
	else if (_state.s == State::S::ValuePlaybackStarting)
	{
		//
	}
	else if (_state.s == State::S::ValueCaptureStarting)
	{
		//
	}
	else if (_state.s == State::S::ValueRemote)
	{
		//
	}
	return true;
}

} // namespace StretchPlayer