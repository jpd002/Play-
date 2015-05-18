#pragma once

#include <string>
#include "Types.h"
#include "Stream.h"

namespace ISO9660
{
	class CPathTableRecord
	{
	public:
						CPathTableRecord(Framework::CStream&);
						~CPathTableRecord();

		uint8			GetNameLength() const;
		uint32			GetAddress() const;
		uint32			GetParentRecord() const;
		const char*		GetName() const;

	private:
		uint8			m_nameLength = 0;
		uint8			m_exLength = 0;
		uint32			m_location = 0;
		uint16			m_parentDir = 0;
		std::string		m_directory;
	};
}
