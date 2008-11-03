#ifndef _PS2_PSFBIOS_H_
#define _PS2_PSFBIOS_H_

#include "../Bios.h"
#include "iop/IopBios.h"
#include "PsfDevice.h"

namespace PS2
{
	class CPsfBios : public CBios
	{
	public:
					CPsfBios(const CPsfDevice::PsfFile&, CMIPS&, uint8*, uint32);
		virtual		~CPsfBios();
	    void		HandleException();
	    void		HandleInterrupt();

	private:
		CIopBios	m_bios;
	};
}

#endif
