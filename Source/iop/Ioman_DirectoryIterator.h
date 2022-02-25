#pragma once

#include <memory>
#include "Ioman_Defs.h"

namespace Iop
{
	namespace Ioman
	{
		class CDirectoryIterator
		{
		public:
			virtual ~CDirectoryIterator() = default;
			virtual void ReadEntry(DIRENTRY*) = 0;
			virtual bool IsDone() = 0;
		};

		typedef std::unique_ptr<CDirectoryIterator> DirectoryIteratorPtr;
	}
}
