#pragma once

#include <vector>
#include "Stream.h"
#include "ApaDefs.h"

namespace Hdd
{
	class CApaReader
	{
	public:
		CApaReader(Framework::CStream&);

		std::vector<APA_HEADER> GetPartitions();
		bool TryFindPartition(const char*, APA_HEADER&);

	private:
		Framework::CStream& m_stream;
	};
}
