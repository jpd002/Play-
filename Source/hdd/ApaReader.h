#pragma once

#include "Stream.h"

namespace Hdd
{
	class CApaReader
	{
	public:
		CApaReader(Framework::CStream&);

		uint32 FindPartition(const char*);

	private:
		Framework::CStream& m_stream;
	};
}
