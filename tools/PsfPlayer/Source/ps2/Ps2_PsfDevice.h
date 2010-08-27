#ifndef _PS2_PSFDEVICE_H_
#define _PS2_PSFDEVICE_H_

#include "iop/Ioman_Device.h"
#include "PsfBase.h"
#include "../PsfFs.h"
#include <list>

namespace PS2
{
	class CPsfDevice : public Iop::Ioman::CDevice
	{
	public:
					                    CPsfDevice();
		virtual		                    ~CPsfDevice();

		void							AppendArchive(const CPsfBase&);
        virtual Framework::CStream*     GetFile(uint32, const char*);

	private:
		CPsfFs							m_fileSystem;

	};
}

#endif
