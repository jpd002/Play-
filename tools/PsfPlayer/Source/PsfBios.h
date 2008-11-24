#ifndef _PS2_PSFBIOS_H_
#define _PS2_PSFBIOS_H_

#include "Bios.h"
#include "iop/IopBios.h"
#include "PsfDevice.h"

namespace PS2
{
	class CPsfBios : public CBios
	{
	public:
								CPsfBios(CMIPS&, uint8*, uint32);
		virtual					~CPsfBios();
	    void					HandleException();
	    void					HandleInterrupt();
		void					CountTicks(uint32);

#ifdef DEBUGGER_INCLUDED
		void					LoadDebugTags(const char*);
		void					SaveDebugTags(const char*);
#endif

		void					AppendArchive(const CPsfBase&);
		void					Start();

	private:
		Iop::CIoman::DevicePtr	m_psfDevice;
		CIopBios				m_bios;
	};
}

#endif
