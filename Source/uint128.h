#pragma once

#include "Types.h"
#include <tuple>

struct uint128
{
	union
	{
		struct
		{
			uint32 nV[4];
		};
		struct
		{
			uint32 nV0;
			uint32 nV1;
			uint32 nV2;
			uint32 nV3;
		};
		struct
		{
			uint64 nD0;
			uint64 nD1;
		};
	};

	inline bool operator<(const uint128& rhs) const
	{
		const auto& lhs = (*this);
		return std::tie(lhs.nD1, lhs.nD0) < std::tie(rhs.nD1, rhs.nD0);
	}

	inline bool operator==(const uint128& rhs) const
	{
		const auto& lhs = (*this);
		return std::tie(lhs.nD1, lhs.nD0) == std::tie(rhs.nD1, rhs.nD0);
	}
};
static_assert(sizeof(uint128) == 0x10);
