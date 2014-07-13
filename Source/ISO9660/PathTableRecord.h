#pragma once

#include <string>
#include "Types.h"
#include "Stream.h"

namespace ISO9660
{
	class CPathTableRecord
	{
	public:
						CPathTableRecord(Framework::CStream*);
						~CPathTableRecord();

		uint8			GetNameLength() const;
		uint32			GetAddress() const;
		uint32			GetParentRecord() const;
		const char*		GetName() const;

	private:
		uint8			m_nameLength;
		uint8			m_exLength;
		uint32			m_location;
		uint16			m_parentDir;
		std::string		m_directory;
	};
}
