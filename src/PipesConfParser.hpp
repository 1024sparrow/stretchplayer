#pragma once

#include <string>

#include "PipesConf.hpp"

namespace StretchPlayer
{

class PipesConfParser
{
public:
	PipesConfParser();
	int parse(const char *filepath);
	static const char * error(int errorCode);
	const PipesConf & result() const;
private:
	enum class Error
	{
		NoError = 0,
		SystaxError,

		CanNotOpenFile,
		IncompleteFile,
		__Count
	};
	void initParse();
	Error parseTick(char byte);

	PipesConf _pipesConf;
	struct State
	{
		enum class S
		{
			Init,
			IntoGlobalObject,
			KeyStarting,
			KeyValueSeparator,
			ValuePlaybackStarting,
			ValueCaptureStarting,
			ValueRemote,
			ValuePlayback,
			ValueCapture,
			ValuePlaybackTilda,
			ValueCaptureTilda,
			ValuePlaybackDollar,
			ValueCaptureDollar,
			ValueFinished,

			Finished
		} s {S::Init};
		std::string key;
		std::string value;
		int counter;
	} _state;
};

} // namespace StretchPlayer

