#ifndef _ISO9660_VOLUMEDESCRIPTOR_H_
#define _ISO9660_VOLUMEDESCRIPTOR_H_

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
		uint8		m_nType;
		char		m_sStdId[6];
		char		m_sVolumeId[33];
		uint32		m_nLPathTableAddress;
		uint32		m_nMPathTableAddress;
	};

}

#endif
