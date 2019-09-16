#pragma once

#include "Iop_Ioman.h"

namespace Iop
{
	namespace Ioman
	{
		class CScopedFile
		{
		public:
			CScopedFile(uint32, CIoman&);
			CScopedFile(const CScopedFile&) = delete;
			virtual ~CScopedFile();

			operator uint32();
			CScopedFile& operator=(const CScopedFile&) = delete;

		private:
			uint32 m_handle;
			CIoman& m_ioman;
		};
	}
}
