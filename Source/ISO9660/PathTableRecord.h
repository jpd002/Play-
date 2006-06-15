#ifndef _ISO9660_PATHTABLERECORD_H_
#define _ISO9660_PATHTABLERECORD_H_

#include "Types.h"
#include "Stream.h"

namespace ISO9660
{

	class CPathTableRecord
	{
	public:
						CPathTableRecord(Framework::CStream*);
						~CPathTableRecord();

		uint8			GetNameLenght() const;
		uint32			GetAddress() const;
		uint32			GetParentRecord() const;
		const char*		GetName() const;

	private:
		uint8			m_nNameLenght;
		uint8			m_nExLenght;
		uint32			m_nLocation;
		uint16			m_nParentDir;
		char*			m_sDirectory;
	};
}

#endif
