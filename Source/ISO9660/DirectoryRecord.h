#ifndef _ISO9660_DIRECTORYRECORD_H_
#define _ISO9660_DIRECTORYRECORD_H_

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
		bool			IsDirectory();
		uint8			GetLength();
		const char*		GetName();
		uint32			GetPosition();
		uint32			GetDataLength();

	private:
		uint8			m_nLength;
		uint8			m_nExLength;
		uint32			m_nPosition;
		uint32			m_nDataLength;
		uint8			m_nFlags;
		char			m_sName[256];
	};
}

#endif
