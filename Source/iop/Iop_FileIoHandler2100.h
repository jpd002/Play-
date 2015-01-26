#pragma once

#include "Iop_FileIo.h"

namespace Iop
{
	class CFileIoHandler2100 : public CFileIo::CHandler
	{
	public:
						CFileIoHandler2100(CIoman*);

		virtual void	Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

	private:
		struct OPENCOMMAND
		{
			uint32			flags;
			uint32			somePtr1;
			char			fileName[256];
		};

		struct CLOSECOMMAND
		{
			uint32			fd;
		};

		struct READCOMMAND
		{
			uint32			fd;
			uint32			buffer;
			uint32			size;
		};

		struct SEEKCOMMAND
		{
			uint32			fd;
			uint32			offset;
			uint32			whence;
		};
	};
};
