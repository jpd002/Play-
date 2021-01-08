
#pragma once

#include "DirectoryDevice.h"

namespace Iop
{
	namespace Ioman
	{
		class CPathDirectoryDevice : public CDirectoryDevice
		{
		public:
			CPathDirectoryDevice(const fs::path& basePath)
			    : m_basePath(basePath)
			{
			}

		protected:
			fs::path GetBasePath() override
			{
				return m_basePath;
			}

		private:
			fs::path m_basePath;
		};
	}
}
