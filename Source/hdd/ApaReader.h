#pragma once

#include "Stream.h"
#include "ApaDefs.h"

namespace Hdd
{
	class CApaReader
	{
	public:
		CApaReader(Framework::CStream&);

		bool TryFindPartition(const char*, APA_HEADER&);

	private:
		Framework::CStream& m_stream;
	};
}
