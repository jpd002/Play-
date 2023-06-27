#pragma once

namespace TimeUtils
{
	constexpr static uint64 SecsToCycles(uint64 freq, uint64 secs)
	{
		return secs * freq;
	}

	constexpr uint64 MsecsToCycles(uint64 freq, uint64 msecs)
	{
		return msecs * freq / 1000;
	}

	constexpr uint64 UsecsToCycles(uint64 freq, uint64 usecs)
	{
		return usecs * freq / 1'000'000;
	}
}
