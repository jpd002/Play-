#ifndef _PSFDEVICE_H_
#define _PSFDEVICE_H_

#include "iop/Ioman_Device.h"
#include "../PsfBase.h"

namespace Ps2
{
	class CPsfDevice : public Iop::Ioman::CDevice
	{
	public:
					CPsfDevice(CPsfBase*);
		virtual		~CPsfDevice();

	private:

	};
}

#endif
