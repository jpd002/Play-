#pragma once

#include <string>
#include "Ioman_Device.h"

namespace Iop
{
	namespace Ioman
	{
		class CDirectoryDevice : public CDevice
		{
		public:
			CDirectoryDevice(const char*);
			virtual ~CDirectoryDevice() = default;

			Framework::CStream* GetFile(uint32, const char*) override;
			Directory GetDirectory(const char*) override;

		private:
			std::string m_basePathPreferenceName;
		};
	}
}
