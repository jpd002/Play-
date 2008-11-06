#ifndef _BASICUNIONS_H_
#define _BASICUNIONS_H_

#include "Types.h"
#include <boost/static_assert.hpp>

template <typename FullType, typename HalfType>
struct basic_union
{
	BOOST_STATIC_ASSERT(sizeof(FullType) == (2 * sizeof(HalfType)));

	basic_union() {}
	basic_union(const FullType& f) : f(f) {}
	basic_union(const HalfType& h0, const HalfType& h1) : h0(h0), h1(h1) {}

	union
	{
		struct
		{
			HalfType h0;
			HalfType h1;
		};
		FullType f;
	};
};

typedef basic_union<uint32, uint16> UNION32_16;
typedef basic_union<uint64, uint32> UNION64_32;
typedef basic_union<uint64, UNION32_16> UNION64_32_16;

#endif
