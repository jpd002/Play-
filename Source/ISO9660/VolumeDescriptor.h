#pragma once

#include "BlockProvider.h"

namespace ISO9660
{

	class CVolumeDescriptor
	{
	public:
					CVolumeDescriptor(CBlockProvider*);
					~CVolumeDescriptor();

		uint32		GetLPathTableAddress() const;
		uint32		GetMPathTableAddress() const;

	private:
		uint8		m_type = 0;
		char		m_stdId[6];
		char		m_volumeId[33];
		uint32		m_LPathTableAddress = 0;
		uint32		m_MPathTableAddress = 0;
	};

}
