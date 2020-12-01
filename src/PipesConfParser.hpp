#pragma once

#include <string>

#include "PipesConf.hpp"

namespace StretchPlayer
{

class PipesConfParser
{
public:
	PipesConfParser();
	bool parse(const char *filepath);
	const PipesConf & result() const;
private:
	void initParse();
	bool parseTick(char byte);

	PipesConf _pipesConf;
	struct State
	{
		enum class S
		{
			Init,
			IntoGlobalObject,
			KeyStarting,
			KeyValueSeparator,
			ValuePlayback,
			ValueCapture,
			ValueRemote,

			Finished
		} s {S::Init};
		std::string key;
	} _state;
};

} // namespace StretchPlayer

