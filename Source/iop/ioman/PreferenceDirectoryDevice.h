#pragma once

#include "DirectoryDevice.h"
#include "../../AppConfig.h"

namespace Iop
{
	namespace Ioman
	{
		class CPreferenceDirectoryDevice : public CDirectoryDevice
		{
		public:
			CPreferenceDirectoryDevice(const char* basePathPreferenceName)
			    : m_basePathPreferenceName(basePathPreferenceName)
			{
			}

		protected:
			fs::path GetBasePath() override
			{
				return CAppConfig::GetInstance().GetPreferencePath(m_basePathPreferenceName.c_str());
			}

		private:
			std::string m_basePathPreferenceName;
		};
	}
}
