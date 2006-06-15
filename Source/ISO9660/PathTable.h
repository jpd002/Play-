#ifndef _ISO9660_PATHTABLE_H_
#define _ISO9660_PATHTABLE_H_

#include "PathTableRecord.h"
#include "Types.h"
#include "List.h"

namespace ISO9660
{

	class CPathTable
	{
	public:
											CPathTable(Framework::CStream*, uint32);
											~CPathTable();

		unsigned int						FindRoot();
		unsigned int						FindDirectory(const char*, unsigned int);
		uint32								GetDirectoryAddress(unsigned int);

	private:
		Framework::CList<CPathTableRecord>	m_Record;

	};

}

#endif
