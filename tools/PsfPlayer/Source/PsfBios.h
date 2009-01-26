#ifndef _PS2_PSFBIOS_H_
#define _PS2_PSFBIOS_H_

#include "iop/Iop_BiosBase.h"
#include "iop/IopBios.h"
#include "PsfDevice.h"

namespace PS2
{
	class CPsfBios : public Iop::CBiosBase
	{
	public:
								CPsfBios(CMIPS&, uint8*, uint32);
		virtual					~CPsfBios();
	    void					HandleException();
	    void					HandleInterrupt();
		void					CountTicks(uint32);

        void                    SaveState(CZipArchiveWriter&);
        void                    LoadState(CZipArchiveReader&);

        bool                    IsIdle();

#ifdef DEBUGGER_INCLUDED
		void					LoadDebugTags(Framework::Xml::CNode*);
		void					SaveDebugTags(Framework::Xml::CNode*);
		MipsModuleList			GetModuleList();
#endif

		void					AppendArchive(const CPsfBase&);
		void					Start();

	private:
		Iop::CIoman::DevicePtr	m_psfDevice;
		CIopBios				m_bios;
	};
}

#endif
