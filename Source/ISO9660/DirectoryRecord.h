#pragma once

#include "Types.h"
#include "Stream.h"

namespace ISO9660
{
	class CDirectoryRecord
	{
	public:
		CDirectoryRecord();
		CDirectoryRecord(Framework::CStream*);
		~CDirectoryRecord();

		bool IsDirectory() const;
		uint8 GetLength() const;
		const char* GetName() const;
		uint32 GetPosition() const;
		uint32 GetDataLength() const;

	private:
		uint8 m_length = 0;
		uint8 m_exLength = 0;
		uint32 m_position = 0;
		uint32 m_dataLength = 0;
		uint8 m_flags = 0;
		char m_name[256];
	};
}
