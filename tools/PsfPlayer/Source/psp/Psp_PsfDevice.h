#ifndef _PSP_PSFDEVICE_H_
#define _PSP_PSFDEVICE_H_

#include "Psp_IoDevice.h"
#include "../PsfFs.h"

namespace Psp
{
	class CPsfDevice : public CIoDevice
	{
	public:
		CPsfDevice();
		virtual ~CPsfDevice();

		Framework::CStream* GetFile(const char*, uint32);

		void AppendArchive(const CPsfBase&);

	private:
		CPsfFs m_fileSystem;
	};

	typedef std::shared_ptr<CPsfDevice> PsfDevicePtr;
}

#endif
