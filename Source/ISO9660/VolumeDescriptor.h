#pragma once

#include "Types.h"
#include "Stream.h"

namespace ISO9660
{

	class CVolumeDescriptor
	{
	public:
					CVolumeDescriptor(Framework::CStream*);
					~CVolumeDescriptor();

		uint32		GetLPathTableAddress() const;
		uint32		GetMPathTableAddress() const;

	private:
		uint8		m_type;
		char		m_stdId[6];
		char		m_volumeId[33];
		uint32		m_LPathTableAddress;
		uint32		m_MPathTableAddress;
	};

}
