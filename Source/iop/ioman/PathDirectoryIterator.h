#pragma once

#include "filesystem_def.h"
#include "../Ioman_DirectoryIterator.h"

namespace Iop
{
	namespace Ioman
	{
		class CPathDirectoryIterator : public CDirectoryIterator
		{
		public:
			CPathDirectoryIterator(const fs::path&);

			void ReadEntry(DIRENTRY*) override;
			bool IsDone() override;

		private:
			fs::directory_iterator m_iterator;
		};
	}
}
