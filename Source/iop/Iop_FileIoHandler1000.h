#pragma once

#include "Iop_FileIo.h"

namespace Iop
{
	class CFileIoHandler1000 : public CFileIo::CHandler
	{
	public:
						CFileIoHandler1000(CIoman*);

		virtual void	Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;
	};
}
