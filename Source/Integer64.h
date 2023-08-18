#ifndef _INTEGER64_H_
#define _INTEGER64_H_

#include "Types.h"

struct INTEGER64
{
	union
	{
		uint64 q;
		struct
		{
			uint32 d0;
			uint32 d1;
		};
		uint32 d[2];
	};
};

#endif
