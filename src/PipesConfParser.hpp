#pragma once

#include <string>
#include <tuple>

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
		UnsupportedKeyUsed,
		InobjectCommaExpected,
		UnexpectedComma,

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
		static const char * str(S p)
		{
			static const std::tuple<S, const char *> srcData_123[] = {
				{ S::Init, "Init" },
				{ S::IntoGlobalObject, "IntoGlobalObject" },
				{ S::KeyStarting, "KeyStarting" },
				{ S::KeyValueSeparator, "KeyValueSeparator" },
				{ S::ValuePlaybackStarting, "ValuePlaybackStarting" },
				{ S::ValueCaptureStarting, "ValueCaptureStarting" },
				{ S::ValueRemote, "ValueRemote" },
				{ S::ValuePlayback, "ValuePlayback" },
				{ S::ValueCapture, "ValueCapture" },
				{ S::ValuePlaybackTilda, "ValuePlaybackTilda" },
				{ S::ValueCaptureTilda, "ValueCaptureTilda" },
				{ S::ValuePlaybackDollar, "ValuePlaybackDollar" },
				{ S::ValueCaptureDollar, "ValueCaptureDollar" },
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

} // namespace StretchPlayer

