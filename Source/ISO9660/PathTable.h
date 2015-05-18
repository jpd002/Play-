#pragma once

#include "BlockProvider.h"
#include "PathTableRecord.h"
#include "Types.h"
#include <map>

namespace ISO9660
{

	class CPathTable
	{
	public:
											CPathTable(CBlockProvider*, uint32);
											~CPathTable();

		unsigned int						FindRoot() const;
		unsigned int						FindDirectory(const char*, unsigned int) const;
		uint32								GetDirectoryAddress(unsigned int) const;

	private:
		typedef std::map<size_t, CPathTableRecord> RecordMapType;

		RecordMapType						m_records;
	};

}
