#pragma once

#include <string>
#include "PtrStream.h"
#include "../Ioman_Device.h"
#include "NamcoSys147NANDReader.h"

namespace Iop
{
	namespace Namco
	{
		class CNamcoNANDDevice : public Ioman::CDevice
		{
		public:
			CNamcoNANDDevice(std::unique_ptr<Framework::CStream>);
			virtual ~CNamcoNANDDevice() = default;

			Framework::CStream* GetFile(uint32, const char*) override;
			Ioman::DirectoryIteratorPtr GetDirectory(const char*) override;

		private:
			std::unique_ptr<Framework::CStream> m_nandDumpStream;
			::Namco::CSys147NANDReader m_nandReader;
		};
	}
}
