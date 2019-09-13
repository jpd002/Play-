#pragma once

#include "Iop_FileIo.h"

namespace Iop
{
	class CFileIoHandler1000 : public CFileIo::CHandler
	{
	public:
		CFileIoHandler1000(CIoman*);

		bool Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;
	};
}
