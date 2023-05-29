#pragma once

#include "Psp_IoDevice.h"
#include "../PsfFs.h"

namespace Psp
{
	class CPsfDevice : public CIoDevice
	{
	public:
		Framework::CStream* GetFile(const char*, uint32) override;

		void AppendArchive(const CPsfBase&);

	private:
		CPsfFs m_fileSystem;
	};

	typedef std::shared_ptr<CPsfDevice> PsfDevicePtr;
}
