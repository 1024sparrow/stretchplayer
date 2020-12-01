#pragma once

namespace StretchPlayer
{

struct PipesConf
{
	struct Remote
	{
		enum Type
		{
			TypePipe, // i.e. locally working only (through named FIFO pipes)
			//TypeSocket // some public network interface; not supported yet
		} type {TypePipe};
		operator bool() const
		{
			if (type == TypePipe)
			{
				return false;
			}
			return true;
		}
		union
		{
			//
		} data;
	};
	const char *playback {nullptr};
	const char *capture {nullptr};
	Remote remote;
};

} // namespace StretchPlayer
