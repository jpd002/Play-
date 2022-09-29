#pragma once

#include <string>
#include "PtrStream.h"
#include "../Ioman_Device.h"
#include "../../saves/McDumpReader.h"

namespace Iop
{
	namespace Ioman
	{
		class CMcDumpDevice : public CDevice
		{
		public:
			typedef std::vector<uint8> DumpContent;

			CMcDumpDevice(DumpContent);
			virtual ~CMcDumpDevice() = default;

			Framework::CStream* GetFile(uint32, const char*) override;
			DirectoryIteratorPtr GetDirectory(const char*) override;

		private:
			DumpContent m_content;
			Framework::CPtrStream m_contentReader;
			CMcDumpReader m_dumpReader;
		};
	}
}
